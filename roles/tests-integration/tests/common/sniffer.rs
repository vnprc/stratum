use async_channel::{Receiver, Sender};
use codec_sv2::{
    framing_sv2::framing::Frame, HandshakeRole, Initiator, Responder, StandardEitherFrame,
};
use key_utils::{Secp256k1PublicKey, Secp256k1SecretKey};
use network_helpers_sv2::noise_connection_tokio::Connection;
use roles_logic_sv2::{
    parsers::{
        AnyMessage, CommonMessages,
        JobDeclaration::{
            AllocateMiningJobToken, AllocateMiningJobTokenSuccess, DeclareMiningJob,
            DeclareMiningJobError, DeclareMiningJobSuccess, IdentifyTransactions,
            IdentifyTransactionsSuccess, ProvideMissingTransactions,
            ProvideMissingTransactionsSuccess, SubmitSolution,
        },
        TemplateDistribution,
        TemplateDistribution::CoinbaseOutputDataSize,
    },
    utils::Mutex,
};
use std::{collections::VecDeque, convert::TryInto, net::SocketAddr, sync::Arc};
use tokio::{
    net::{TcpListener, TcpStream},
    select,
};
type MessageFrame = StandardEitherFrame<AnyMessage<'static>>;
type MsgType = u8;

#[derive(Debug, PartialEq)]
enum SnifferError {
    DownstreamClosed,
    UpstreamClosed,
}

/// Allows to intercept messages sent between two roles.
///
/// The downstream (or client) role connects to the [`Sniffer`] `listening_address` and the
/// [`Sniffer`] connects to the `upstream` server. This way, the Sniffer can intercept messages sent
/// between the downstream and upstream roles. The downstream will send its messages to the
/// [`Sniffer`] which will save those in the `downstream_messages` aggregator and forward them to
/// the upstream role. When a response is received it is saved in `upstream_messages` and
/// forwarded to the downstream role. Both `downstream_messages` and `upstream_messages` can be
/// accessed as FIFO queues.
///
/// It is useful for testing purposes, as it allows to assert that the roles have sent specific
/// messages in a specific order and to inspect the messages details.
#[derive(Debug, Clone)]
pub struct Sniffer {
    listening_address: SocketAddr,
    upstream_address: SocketAddr,
    downstream_messages: MessagesAggregator,
    upstream_messages: MessagesAggregator,
}

impl Sniffer {
    /// Creates a new sniffer that listens on the given listening address and connects to the given
    /// upstream address.
    pub async fn new(listening_address: SocketAddr, upstream_address: SocketAddr) -> Self {
        Self {
            listening_address,
            upstream_address,
            downstream_messages: MessagesAggregator::new(),
            upstream_messages: MessagesAggregator::new(),
        }
    }

    /// Starts the sniffer.
    ///
    /// The sniffer should be started after the upstream role have been initialized and is ready to
    /// accept messages and before the downstream role starts sending messages.
    pub async fn start(self) {
        let (downstream_receiver, downstream_sender) =
            Self::create_downstream(Self::wait_for_client(self.listening_address).await)
                .await
                .expect("Failed to create downstream");
        let (upstream_receiver, upstream_sender) = Self::create_upstream(
            TcpStream::connect(self.upstream_address)
                .await
                .expect("Failed to connect to upstream"),
        )
        .await
        .expect("Failed to create upstream");
        let downstream_messages = self.downstream_messages.clone();
        let upstream_messages = self.upstream_messages.clone();
        let _ = select! {
            r = Self::recv_from_down_send_to_up(downstream_receiver, upstream_sender, downstream_messages) => r,
            r = Self::recv_from_up_send_to_down(upstream_receiver, downstream_sender, upstream_messages) => r,
        };
    }

    /// Returns the oldest message sent by downstream.
    ///
    /// The queue is FIFO and once a message is returned it is removed from the queue.
    ///
    /// This can be used to assert that the downstream sent:
    /// - specific message types
    /// - specific message fields
    pub fn next_downstream_message(&self) -> Option<(MsgType, AnyMessage<'static>)> {
        self.downstream_messages.next_message()
    }

    /// Returns the oldest message sent by upstream.
    ///
    /// The queue is FIFO and once a message is returned it is removed from the queue.
    ///
    /// This can be used to assert that the upstream sent:
    /// - specific message types
    /// - specific message fields
    pub fn next_upstream_message(&self) -> Option<(MsgType, AnyMessage<'static>)> {
        self.upstream_messages.next_message()
    }

    async fn create_downstream(
        stream: TcpStream,
    ) -> Option<(Receiver<MessageFrame>, Sender<MessageFrame>)> {
        let pub_key = "9auqWEzQDVyd2oe1JVGFLMLHZtCo2FFqZwtKA5gd9xbuEu7PH72"
            .to_string()
            .parse::<Secp256k1PublicKey>()
            .unwrap()
            .into_bytes();
        let prv_key = "mkDLTBBRxdBv998612qipDYoTK3YUrqLe8uWw7gu3iXbSrn2n"
            .to_string()
            .parse::<Secp256k1SecretKey>()
            .unwrap()
            .into_bytes();
        let responder =
            Responder::from_authority_kp(&pub_key, &prv_key, std::time::Duration::from_secs(10000))
                .unwrap();
        if let Ok((receiver_from_client, send_to_client, _, _)) =
            Connection::new::<'static, AnyMessage<'static>>(
                stream,
                HandshakeRole::Responder(responder),
            )
            .await
        {
            Some((receiver_from_client, send_to_client))
        } else {
            None
        }
    }

    async fn create_upstream(
        stream: TcpStream,
    ) -> Option<(Receiver<MessageFrame>, Sender<MessageFrame>)> {
        let initiator = Initiator::without_pk().expect("This fn call can not fail");
        if let Ok((receiver_from_client, send_to_client, _, _)) =
            Connection::new::<'static, AnyMessage<'static>>(
                stream,
                HandshakeRole::Initiator(initiator),
            )
            .await
        {
            Some((receiver_from_client, send_to_client))
        } else {
            None
        }
    }

    async fn recv_from_down_send_to_up(
        recv: Receiver<MessageFrame>,
        send: Sender<MessageFrame>,
        downstream_messages: MessagesAggregator,
    ) -> Result<(), SnifferError> {
        while let Ok(mut frame) = recv.recv().await {
            let (msg_type, msg) = Self::message_from_frame(&mut frame);
            downstream_messages.add_message(msg_type, msg);
            if send.send(frame).await.is_err() {
                return Err(SnifferError::UpstreamClosed);
            };
        }
        Err(SnifferError::DownstreamClosed)
    }

    async fn recv_from_up_send_to_down(
        recv: Receiver<MessageFrame>,
        send: Sender<MessageFrame>,
        upstream_messages: MessagesAggregator,
    ) -> Result<(), SnifferError> {
        while let Ok(mut frame) = recv.recv().await {
            let (msg_type, msg) = Self::message_from_frame(&mut frame);
            upstream_messages.add_message(msg_type, msg);
            if send.send(frame).await.is_err() {
                return Err(SnifferError::DownstreamClosed);
            };
        }
        Err(SnifferError::UpstreamClosed)
    }

    fn message_from_frame(frame: &mut MessageFrame) -> (MsgType, AnyMessage<'static>) {
        match frame {
            Frame::Sv2(frame) => {
                if let Some(header) = frame.get_header() {
                    let message_type = header.msg_type();
                    let mut payload = frame.payload().to_vec();
                    let message: Result<AnyMessage<'_>, _> =
                        (message_type, payload.as_mut_slice()).try_into();
                    match message {
                        Ok(message) => {
                            let message = Self::into_static(message);
                            (message_type, message)
                        }
                        _ => {
                            println!(
                                "Received frame with invalid payload or message type: {frame:?}"
                            );
                            panic!();
                        }
                    }
                } else {
                    println!("Received frame with invalid header: {frame:?}");
                    panic!();
                }
            }
            Frame::HandShake(f) => {
                println!("Received unexpected handshake frame: {f:?}");
                panic!();
            }
        }
    }

    fn into_static(m: AnyMessage<'_>) -> AnyMessage<'static> {
        match m {
            AnyMessage::Mining(m) => AnyMessage::Mining(m.into_static()),
            AnyMessage::Common(m) => match m {
                CommonMessages::ChannelEndpointChanged(m) => {
                    AnyMessage::Common(CommonMessages::ChannelEndpointChanged(m.into_static()))
                }
                CommonMessages::SetupConnection(m) => {
                    AnyMessage::Common(CommonMessages::SetupConnection(m.into_static()))
                }
                CommonMessages::SetupConnectionError(m) => {
                    AnyMessage::Common(CommonMessages::SetupConnectionError(m.into_static()))
                }
                CommonMessages::SetupConnectionSuccess(m) => {
                    AnyMessage::Common(CommonMessages::SetupConnectionSuccess(m.into_static()))
                }
            },
            AnyMessage::JobDeclaration(m) => match m {
                AllocateMiningJobToken(m) => {
                    AnyMessage::JobDeclaration(AllocateMiningJobToken(m.into_static()))
                }
                AllocateMiningJobTokenSuccess(m) => {
                    AnyMessage::JobDeclaration(AllocateMiningJobTokenSuccess(m.into_static()))
                }
                DeclareMiningJob(m) => {
                    AnyMessage::JobDeclaration(DeclareMiningJob(m.into_static()))
                }
                DeclareMiningJobError(m) => {
                    AnyMessage::JobDeclaration(DeclareMiningJobError(m.into_static()))
                }
                DeclareMiningJobSuccess(m) => {
                    AnyMessage::JobDeclaration(DeclareMiningJobSuccess(m.into_static()))
                }
                IdentifyTransactions(m) => {
                    AnyMessage::JobDeclaration(IdentifyTransactions(m.into_static()))
                }
                IdentifyTransactionsSuccess(m) => {
                    AnyMessage::JobDeclaration(IdentifyTransactionsSuccess(m.into_static()))
                }
                ProvideMissingTransactions(m) => {
                    AnyMessage::JobDeclaration(ProvideMissingTransactions(m.into_static()))
                }
                ProvideMissingTransactionsSuccess(m) => {
                    AnyMessage::JobDeclaration(ProvideMissingTransactionsSuccess(m.into_static()))
                }
                SubmitSolution(m) => AnyMessage::JobDeclaration(SubmitSolution(m.into_static())),
            },
            AnyMessage::TemplateDistribution(m) => match m {
                CoinbaseOutputDataSize(m) => {
                    AnyMessage::TemplateDistribution(CoinbaseOutputDataSize(m.into_static()))
                }
                TemplateDistribution::NewTemplate(m) => AnyMessage::TemplateDistribution(
                    TemplateDistribution::NewTemplate(m.into_static()),
                ),
                TemplateDistribution::RequestTransactionData(m) => {
                    AnyMessage::TemplateDistribution(TemplateDistribution::RequestTransactionData(
                        m.into_static(),
                    ))
                }
                TemplateDistribution::RequestTransactionDataError(m) => {
                    AnyMessage::TemplateDistribution(
                        TemplateDistribution::RequestTransactionDataError(m.into_static()),
                    )
                }
                TemplateDistribution::RequestTransactionDataSuccess(m) => {
                    AnyMessage::TemplateDistribution(
                        TemplateDistribution::RequestTransactionDataSuccess(m.into_static()),
                    )
                }
                TemplateDistribution::SetNewPrevHash(m) => AnyMessage::TemplateDistribution(
                    TemplateDistribution::SetNewPrevHash(m.into_static()),
                ),
                TemplateDistribution::SubmitSolution(m) => AnyMessage::TemplateDistribution(
                    TemplateDistribution::SubmitSolution(m.into_static()),
                ),
            },
        }
    }

    async fn wait_for_client(client: SocketAddr) -> TcpStream {
        let listner = TcpListener::bind(client)
            .await
            .expect("Impossible to listen on given address");
        if let Ok((stream, _)) = listner.accept().await {
            stream
        } else {
            panic!("Impossible to accept dowsntream connection")
        }
    }
}

// Utility macro to assert that the downstream and upstream roles have sent specific messages.
//
// This macro can be called in two ways:
// 1. If you want to assert the message without any of its properties, you can invoke the macro
//   with the message group, the nested message group, the message, and the expected message:
//   `assert_message!(TemplateDistribution, TemplateDistribution, $msg, $expected_message_variant);`.
//
// 2. If you want to assert the message with its properties, you can invoke the macro with the
//  message group, the nested message group, the message, the expected message, and the expected
//  properties and values:
//  `assert_message!(TemplateDistribution, TemplateDistribution, $msg, $expected_message_variant,
//  $expected_property, $expected_property_value, ...);`.
//  Note that you can provide any number of properties and values.
//
//  In both cases, the `$message_group` could be any variant of `PoolMessages::$message_group` and
//  the `$nested_message_group` could be any variant of
//  `PoolMessages::$message_group($nested_message_group)`.
//
//  If you dont want to provide the `$message_group` and `$nested_message_group` arguments, you can
//  utilize `assert_common_message!`, `assert_tp_message!`, `assert_mining_message!`, and
//  `assert_jd_message!` macros. All those macros are just wrappers around `assert_message!` macro
//  with predefined `$message_group` and `$nested_message_group` arguments. They also can be called
//  in two ways, with or without properties validation.
#[macro_export]
macro_rules! assert_message {
  ($message_group:ident, $nested_message_group:ident, $msg:expr, $expected_message_variant:ident,
   $($expected_property:ident, $expected_property_value:expr),*) => { match $msg {
	  Some((_, message)) => {
		match message {
		  PoolMessages::$message_group($nested_message_group::$expected_message_variant(
			  $expected_message_variant {
				$($expected_property,)*
				  ..
			  },
		  )) => {
			$(
			  assert_eq!($expected_property.clone(), $expected_property_value);
			)*
		  }
		  _ => {
			panic!(
			  "Sent wrong message: {:?}",
			  message
			);
		  }
		}
	  }
	  _ => panic!("No message received"),
		}
  };
  ($message_group:ident, $nested_message_group:ident, $msg:expr, $expected_message_variant:ident) => {
	match $msg {
	  Some((_, message)) => {
		match message {
		  PoolMessages::$message_group($nested_message_group::$expected_message_variant(_)) => {}
		  _ => {
			panic!(
			  "Sent wrong message: {:?}",
			  message
			);
		  }
		}
	  }
	  _ => panic!("No message received"),
		}
  };
}

// Assert that the message is a common message and that it has the expected properties and values.
#[macro_export]
macro_rules! assert_common_message {
  ($msg:expr, $expected_message_variant:ident, $($expected_property:ident, $expected_property_value:expr),*) => {
	assert_message!(Common, CommonMessages, $msg, $expected_message_variant, $($expected_property, $expected_property_value),*);
  };
  ($msg:expr, $expected_message_variant:ident) => {
	assert_message!(Common, CommonMessages, $msg, $expected_message_variant);
  };
}

// Assert that the message is a template distribution message and that it has the expected
// properties and values.
#[macro_export]
macro_rules! assert_tp_message {
  ($msg:expr, $expected_message_variant:ident, $($expected_property:ident, $expected_property_value:expr),*) => {
	assert_message!(TemplateDistribution, TemplateDistribution, $msg, $expected_message_variant, $($expected_property, $expected_property_value),*);
  };
  ($msg:expr, $expected_message_variant:ident) => {
	assert_message!(TemplateDistribution, TemplateDistribution, $msg, $expected_message_variant);
  };
}

// Assert that the message is a mining message and that it has the expected properties and values.
#[macro_export]
macro_rules! assert_mining_message {
  ($msg:expr, $expected_message_variant:ident, $($expected_property:ident, $expected_property_value:expr),*) => {
	assert_message!(Mining, Mining, $msg, $expected_message_variant, $($expected_property, $expected_property_value),*);
  };
  ($msg:expr, $expected_message_variant:ident) => {
	assert_message!(Mining, Mining, $msg, $expected_message_variant);
  };
}

// Assert that the message is a job declaration message and that it has the expected properties and
// values.
#[macro_export]
macro_rules! assert_jd_message {
  ($msg:expr, $expected_message_variant:ident, $($expected_property:ident, $expected_property_value:expr),*) => {
	assert_message!(JobDeclaration, JobDeclaration, $msg, $expected_message_variant, $($expected_property, $expected_property_value),*);
  };
  ($msg:expr, $expected_message_variant:ident) => {
	assert_message!(JobDeclaration, JobDeclaration, $msg, $expected_message_variant);
  };
}

// This implementation is used in order to check if a test has handled all messages sent by the
// downstream and upstream roles. If not, the test will panic.
//
// This is useful to ensure that the test has checked all exchanged messages between the roles.
impl Drop for Sniffer {
    fn drop(&mut self) {
        // Don't print backtrace on panic
        std::panic::set_hook(Box::new(|_| {
            println!();
        }));
        if !self.downstream_messages.is_empty() {
            println!(
                "You didn't handle all downstream messages: {:?}",
                self.downstream_messages
            );
            panic!();
        }
        if !self.upstream_messages.is_empty() {
            println!(
                "You didn't handle all upstream messages: {:?}",
                self.upstream_messages
            );
            panic!();
        }
    }
}

#[derive(Debug, Clone)]
struct MessagesAggregator {
    messages: Arc<Mutex<VecDeque<(MsgType, AnyMessage<'static>)>>>,
}

impl MessagesAggregator {
    fn new() -> Self {
        Self {
            messages: Arc::new(Mutex::new(VecDeque::new())),
        }
    }

    // Adds a message to the end of the queue.
    fn add_message(&self, msg_type: MsgType, message: AnyMessage<'static>) {
        self.messages
            .safe_lock(|messages| messages.push_back((msg_type, message)))
            .unwrap();
    }

    fn is_empty(&self) -> bool {
        self.messages
            .safe_lock(|messages| messages.is_empty())
            .unwrap()
    }

    // The aggregator queues messages in FIFO order, so this function returns the oldest message in
    // the queue.
    //
    // The returned message is removed from the queue.
    fn next_message(&self) -> Option<(MsgType, AnyMessage<'static>)> {
        let is_state = self
            .messages
            .safe_lock(|messages| {
                let mut cloned = messages.clone();
                if let Some((msg_type, msg)) = cloned.pop_front() {
                    *messages = cloned;
                    Some((msg_type, msg))
                } else {
                    None
                }
            })
            .unwrap();
        is_state
    }
}
