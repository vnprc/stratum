#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

/// Identifier for the extension_type field in the SV2 frame, indicating no
/// extensions.
static const uint16_t EXTENSION_TYPE_NO_EXTENSION = 0;

/// Size of the SV2 frame header in bytes.
static const uintptr_t SV2_FRAME_HEADER_SIZE = 6;

static const uintptr_t SV2_FRAME_HEADER_LEN_OFFSET = 3;

static const uintptr_t SV2_FRAME_HEADER_LEN_END = 3;

/// Maximum size of an SV2 frame chunk in bytes.
static const uintptr_t SV2_FRAME_CHUNK_SIZE = 65535;

/// Size of the MAC for supported AEAD encryption algorithm (ChaChaPoly).
static const uintptr_t AEAD_MAC_LEN = 16;

/// Size of the encrypted SV2 frame header, including the MAC.
static const uintptr_t ENCRYPTED_SV2_FRAME_HEADER_SIZE = (SV2_FRAME_HEADER_SIZE + AEAD_MAC_LEN);

/// Size of the Noise protocol frame header in bytes.
static const uintptr_t NOISE_FRAME_HEADER_SIZE = 2;

static const uintptr_t NOISE_FRAME_HEADER_LEN_OFFSET = 0;

/// Size in bytes of the encoded elliptic curve point using ElligatorSwift
/// encoding. This encoding produces a 64-byte representation of the
/// X-coordinate of a secp256k1 curve point.
static const uintptr_t ELLSWIFT_ENCODING_SIZE = 64;

static const uintptr_t RESPONDER_EXPECTED_HANDSHAKE_MESSAGE_SIZE = ELLSWIFT_ENCODING_SIZE;

static const uintptr_t MAC = 16;

/// Size in bytes of the encrypted ElligatorSwift encoded data, which includes
/// the original ElligatorSwift encoded data and a MAC for integrity
/// verification.
static const uintptr_t ENCRYPTED_ELLSWIFT_ENCODING_SIZE = (ELLSWIFT_ENCODING_SIZE + MAC);

/// Size in bytes of the SIGNATURE_NOISE_MESSAGE, which contains information and
/// a signature for the handshake initiator, formatted according to the Noise
/// Protocol specifications.
static const uintptr_t SIGNATURE_NOISE_MESSAGE_SIZE = 74;

/// Size in bytes of the encrypted signature noise message, which includes the
/// SIGNATURE_NOISE_MESSAGE and a MAC for integrity verification.
static const uintptr_t ENCRYPTED_SIGNATURE_NOISE_MESSAGE_SIZE = (SIGNATURE_NOISE_MESSAGE_SIZE + MAC);

/// Size in bytes of the handshake message expected by the initiator,
/// encompassing:
/// - ElligatorSwift encoded public key
/// - Encrypted ElligatorSwift encoding
/// - Encrypted SIGNATURE_NOISE_MESSAGE
static const uintptr_t INITIATOR_EXPECTED_HANDSHAKE_MESSAGE_SIZE = ((ELLSWIFT_ENCODING_SIZE + ENCRYPTED_ELLSWIFT_ENCODING_SIZE) + ENCRYPTED_SIGNATURE_NOISE_MESSAGE_SIZE);

static const uint8_t SV2_MINING_PROTOCOL_DISCRIMINANT = 0;

static const uint8_t SV2_JOB_DECLARATION_PROTOCOL_DISCRIMINANT = 1;

static const uint8_t SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT = 2;

static const uint8_t MESSAGE_TYPE_SETUP_CONNECTION = 0;

static const uint8_t MESSAGE_TYPE_SETUP_CONNECTION_SUCCESS = 1;

static const uint8_t MESSAGE_TYPE_SETUP_CONNECTION_ERROR = 2;

static const uint8_t MESSAGE_TYPE_CHANNEL_ENDPOINT_CHANGED = 3;

static const uint8_t MESSAGE_TYPE_OPEN_STANDARD_MINING_CHANNEL = 16;

static const uint8_t MESSAGE_TYPE_OPEN_STANDARD_MINING_CHANNEL_SUCCESS = 17;

static const uint8_t MESSAGE_TYPE_OPEN_MINING_CHANNEL_ERROR = 18;

static const uint8_t MESSAGE_TYPE_OPEN_EXTENDED_MINING_CHANNEL = 19;

static const uint8_t MESSAGE_TYPE_OPEN_EXTENDED_MINING_CHANNEL_SUCCES = 20;

static const uint8_t MESSAGE_TYPE_NEW_MINING_JOB = 21;

static const uint8_t MESSAGE_TYPE_UPDATE_CHANNEL = 22;

static const uint8_t MESSAGE_TYPE_UPDATE_CHANNEL_ERROR = 23;

static const uint8_t MESSAGE_TYPE_CLOSE_CHANNEL = 24;

static const uint8_t MESSAGE_TYPE_SET_EXTRANONCE_PREFIX = 25;

static const uint8_t MESSAGE_TYPE_SUBMIT_SHARES_STANDARD = 26;

static const uint8_t MESSAGE_TYPE_SUBMIT_SHARES_EXTENDED = 27;

static const uint8_t MESSAGE_TYPE_SUBMIT_SHARES_SUCCESS = 28;

static const uint8_t MESSAGE_TYPE_SUBMIT_SHARES_ERROR = 29;

static const uint8_t MESSAGE_TYPE_NEW_EXTENDED_MINING_JOB = 31;

static const uint8_t MESSAGE_TYPE_MINING_SET_NEW_PREV_HASH = 32;

static const uint8_t MESSAGE_TYPE_SET_TARGET = 33;

static const uint8_t MESSAGE_TYPE_SET_CUSTOM_MINING_JOB = 34;

static const uint8_t MESSAGE_TYPE_SET_CUSTOM_MINING_JOB_SUCCESS = 35;

static const uint8_t MESSAGE_TYPE_SET_CUSTOM_MINING_JOB_ERROR = 36;

static const uint8_t MESSAGE_TYPE_RECONNECT = 37;

static const uint8_t MESSAGE_TYPE_SET_GROUP_CHANNEL = 38;

static const uint8_t MESSAGE_TYPE_ALLOCATE_MINING_JOB_TOKEN = 80;

static const uint8_t MESSAGE_TYPE_ALLOCATE_MINING_JOB_TOKEN_SUCCESS = 81;

static const uint8_t MESSAGE_TYPE_IDENTIFY_TRANSACTIONS = 83;

static const uint8_t MESSAGE_TYPE_IDENTIFY_TRANSACTIONS_SUCCESS = 84;

static const uint8_t MESSAGE_TYPE_PROVIDE_MISSING_TRANSACTIONS = 85;

static const uint8_t MESSAGE_TYPE_PROVIDE_MISSING_TRANSACTIONS_SUCCESS = 86;

static const uint8_t MESSAGE_TYPE_DECLARE_MINING_JOB = 87;

static const uint8_t MESSAGE_TYPE_DECLARE_MINING_JOB_SUCCESS = 88;

static const uint8_t MESSAGE_TYPE_DECLARE_MINING_JOB_ERROR = 89;

static const uint8_t MESSAGE_TYPE_SUBMIT_SOLUTION_JD = 96;

static const uint8_t MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE = 112;

static const uint8_t MESSAGE_TYPE_NEW_TEMPLATE = 113;

static const uint8_t MESSAGE_TYPE_SET_NEW_PREV_HASH = 114;

static const uint8_t MESSAGE_TYPE_REQUEST_TRANSACTION_DATA = 115;

static const uint8_t MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_SUCCESS = 116;

static const uint8_t MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_ERROR = 117;

static const uint8_t MESSAGE_TYPE_SUBMIT_SOLUTION = 118;

static const bool CHANNEL_BIT_SETUP_CONNECTION = false;

static const bool CHANNEL_BIT_SETUP_CONNECTION_SUCCESS = false;

static const bool CHANNEL_BIT_SETUP_CONNECTION_ERROR = false;

static const bool CHANNEL_BIT_CHANNEL_ENDPOINT_CHANGED = true;

static const bool CHANNEL_BIT_COINBASE_OUTPUT_DATA_SIZE = false;

static const bool CHANNEL_BIT_NEW_TEMPLATE = false;

static const bool CHANNEL_BIT_SET_NEW_PREV_HASH = false;

static const bool CHANNEL_BIT_REQUEST_TRANSACTION_DATA = false;

static const bool CHANNEL_BIT_REQUEST_TRANSACTION_DATA_SUCCESS = false;

static const bool CHANNEL_BIT_REQUEST_TRANSACTION_DATA_ERROR = false;

static const bool CHANNEL_BIT_SUBMIT_SOLUTION = false;

static const bool CHANNEL_BIT_ALLOCATE_MINING_JOB_TOKEN = false;

static const bool CHANNEL_BIT_ALLOCATE_MINING_JOB_TOKEN_SUCCESS = false;

static const bool CHANNEL_BIT_DECLARE_MINING_JOB = false;

static const bool CHANNEL_BIT_DECLARE_MINING_JOB_SUCCESS = false;

static const bool CHANNEL_BIT_DECLARE_MINING_JOB_ERROR = false;

static const bool CHANNEL_BIT_IDENTIFY_TRANSACTIONS = false;

static const bool CHANNEL_BIT_IDENTIFY_TRANSACTIONS_SUCCESS = false;

static const bool CHANNEL_BIT_PROVIDE_MISSING_TRANSACTIONS = false;

static const bool CHANNEL_BIT_PROVIDE_MISSING_TRANSACTIONS_SUCCESS = false;

static const bool CHANNEL_BIT_SUBMIT_SOLUTION_JD = true;

static const bool CHANNEL_BIT_CLOSE_CHANNEL = true;

static const bool CHANNEL_BIT_NEW_EXTENDED_MINING_JOB = true;

static const bool CHANNEL_BIT_NEW_MINING_JOB = true;

static const bool CHANNEL_BIT_OPEN_EXTENDED_MINING_CHANNEL = false;

static const bool CHANNEL_BIT_OPEN_EXTENDED_MINING_CHANNEL_SUCCES = false;

static const bool CHANNEL_BIT_OPEN_MINING_CHANNEL_ERROR = false;

static const bool CHANNEL_BIT_OPEN_STANDARD_MINING_CHANNEL = false;

static const bool CHANNEL_BIT_OPEN_STANDARD_MINING_CHANNEL_SUCCESS = false;

static const bool CHANNEL_BIT_RECONNECT = false;

static const bool CHANNEL_BIT_SET_CUSTOM_MINING_JOB = false;

static const bool CHANNEL_BIT_SET_CUSTOM_MINING_JOB_ERROR = false;

static const bool CHANNEL_BIT_SET_CUSTOM_MINING_JOB_SUCCESS = false;

static const bool CHANNEL_BIT_SET_EXTRANONCE_PREFIX = true;

static const bool CHANNEL_BIT_SET_GROUP_CHANNEL = false;

static const bool CHANNEL_BIT_MINING_SET_NEW_PREV_HASH = true;

static const bool CHANNEL_BIT_SET_TARGET = true;

static const bool CHANNEL_BIT_SUBMIT_SHARES_ERROR = true;

static const bool CHANNEL_BIT_SUBMIT_SHARES_EXTENDED = true;

static const bool CHANNEL_BIT_SUBMIT_SHARES_STANDARD = true;

static const bool CHANNEL_BIT_SUBMIT_SHARES_SUCCESS = true;

static const bool CHANNEL_BIT_UPDATE_CHANNEL = true;

static const bool CHANNEL_BIT_UPDATE_CHANNEL_ERROR = true;
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct CVec {
  uint8_t *data;
  uintptr_t len;
  uintptr_t capacity;
};

struct CVec2 {
  CVec *data;
  uintptr_t len;
  uintptr_t capacity;
};

struct U24 {
  uint32_t _0;
};

extern "C" {

/// Given a C allocated buffer return a rust allocated CVec
///
/// # Safety
///
CVec cvec_from_buffer(const uint8_t *data, uintptr_t len);

/// # Safety
///
CVec2 init_cvec2();

/// The caller is reponsible for NOT adding duplicate cvecs to the cvec2 structure,
/// as this can lead to double free errors when the message is dropped.
/// # Safety
///
void cvec2_push(CVec2 *cvec2, CVec cvec);

void _c_export_u24(U24 _a);

void _c_export_cvec(CVec _a);

void _c_export_cvec2(CVec2 _a);

} // extern "C"
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

/// MiningProtocol = [`SV2_MINING_PROTOCOL_DISCRIMINANT`],
/// JobDeclarationProtocol = [`SV2_JOB_DECLARATION_PROTOCOL_DISCRIMINANT`],
/// TemplateDistributionProtocol = [`SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT`],
enum class Protocol : uint8_t {
  MiningProtocol = SV2_MINING_PROTOCOL_DISCRIMINANT,
  JobDeclarationProtocol = SV2_JOB_DECLARATION_PROTOCOL_DISCRIMINANT,
  TemplateDistributionProtocol = SV2_TEMPLATE_DISTR_PROTOCOL_DISCRIMINANT,
};

/// ## ChannelEndpointChanged (Server -> Client)
/// When a channel’s upstream or downstream endpoint changes and that channel had previously
/// sent messages with [channel_msg] bitset of unknown extension_type, the intermediate proxy
/// MUST send a [`ChannelEndpointChanged`] message. Upon receipt thereof, any extension state
/// (including version negotiation and the presence of support for a given extension) MUST be
/// reset and version/presence negotiation must begin again.
///
struct ChannelEndpointChanged {
  /// The channel which has changed endpoint.
  uint32_t channel_id;
};

/// ## SetupConnection.Success (Server -> Client)
/// Response to [`SetupConnection`] message if the server accepts the connection. The client is
/// required to verify the set of feature flags that the server supports and act accordingly.
struct SetupConnectionSuccess {
  /// Selected version proposed by the connecting node that the upstream
  /// node supports. This version will be used on the connection for the rest
  /// of its life.
  uint16_t used_version;
  /// Flags indicating optional protocol features the server supports. Each
  /// protocol from [`Protocol`] field has its own values/flags.
  uint32_t flags;
};

struct CSetupConnection {
  Protocol protocol;
  uint16_t min_version;
  uint16_t max_version;
  uint32_t flags;
  CVec endpoint_host;
  uint16_t endpoint_port;
  CVec vendor;
  CVec hardware_version;
  CVec firmware;
  CVec device_id;
};

struct CSetupConnectionError {
  uint32_t flags;
  CVec error_code;
};

extern "C" {

void _c_export_channel_endpoint_changed(ChannelEndpointChanged _a);

void _c_export_setup_conn_succ(SetupConnectionSuccess _a);

void free_setup_connection(CSetupConnection s);

void free_setup_connection_error(CSetupConnectionError s);

} // extern "C"
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

/// ## CoinbaseOutputDataSize (Client -> Server)
/// Ultimately, the pool is responsible for adding coinbase transaction outputs for payouts and
/// other uses, and thus the Template Provider will need to consider this additional block size
/// when selecting transactions for inclusion in a block (to not create an invalid, oversized block).
/// Thus, this message is used to indicate that some additional space in the block/coinbase
/// transaction be reserved for the pool’s use (while always assuming the pool will use the entirety
/// of available coinbase space).
/// The Job Declarator MUST discover the maximum serialized size of the additional outputs which
/// will be added by the pool(s) it intends to use this work. It then MUST communicate the
/// maximum such size to the Template Provider via this message. The Template Provider MUST
/// NOT provide NewWork messages which would represent consensus-invalid blocks once this
/// additional size — along with a maximally-sized (100 byte) coinbase field — is added. Further,
/// the Template Provider MUST consider the maximum additional bytes required in the output
/// count variable-length integer in the coinbase transaction when complying with the size limits.
struct CoinbaseOutputDataSize {
  /// The maximum additional serialized bytes which the pool will add in
  /// coinbase transaction outputs.
  uint32_t coinbase_output_max_additional_size;
};

/// ## RequestTransactionData (Client -> Server)
/// A request sent by the Job Declarator to the Template Provider which requests the set of
/// transaction data for all transactions (excluding the coinbase transaction) included in a block, as
/// well as any additional data which may be required by the Pool to validate the work.
struct RequestTransactionData {
  /// The template_id corresponding to a NewTemplate message.
  uint64_t template_id;
};

struct CNewTemplate {
  uint64_t template_id;
  bool future_template;
  uint32_t version;
  uint32_t coinbase_tx_version;
  CVec coinbase_prefix;
  uint32_t coinbase_tx_input_sequence;
  uint64_t coinbase_tx_value_remaining;
  uint32_t coinbase_tx_outputs_count;
  CVec coinbase_tx_outputs;
  uint32_t coinbase_tx_locktime;
  CVec2 merkle_path;
};

struct CRequestTransactionDataSuccess {
  uint64_t template_id;
  CVec excess_data;
  CVec2 transaction_list;
};

struct CRequestTransactionDataError {
  uint64_t template_id;
  CVec error_code;
};

struct CSetNewPrevHash {
  uint64_t template_id;
  CVec prev_hash;
  uint32_t header_timestamp;
  uint32_t n_bits;
  CVec target;
};

struct CSubmitSolution {
  uint64_t template_id;
  uint32_t version;
  uint32_t header_timestamp;
  uint32_t header_nonce;
  CVec coinbase_tx;
};

extern "C" {

void _c_export_coinbase_out(CoinbaseOutputDataSize _a);

void _c_export_req_tx_data(RequestTransactionData _a);

void free_new_template(CNewTemplate s);

void free_request_tx_data_success(CRequestTransactionDataSuccess s);

void free_request_tx_data_error(CRequestTransactionDataError s);

void free_set_new_prev_hash(CSetNewPrevHash s);

void free_submit_solution(CSubmitSolution s);

} // extern "C"
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

/// C-compatible enumeration of possible errors in the `codec_sv2` module.
///
/// This enum mirrors the [`Error`] enum but is designed to be used in C code through FFI. It
/// represents the same set of errors as [`Error`], making them accessible to C programs.
struct CError {
  enum class Tag {
    /// AEAD (`snow`) error in the Noise protocol.
    AeadError,
    /// Binary Sv2 data format error.
    BinarySv2Error,
    /// Framing Sv2 error.
    FramingError,
    /// Framing Sv2 error.
    FramingSv2Error,
    /// Invalid step for initiator in the Noise protocol.
    InvalidStepForInitiator,
    /// Invalid step for responder in the Noise protocol.
    InvalidStepForResponder,
    /// Missing bytes in the Noise protocol.
    MissingBytes,
    /// Sv2 Noise protocol error.
    NoiseSv2Error,
    /// Noise protocol is not in the expected handshake state.
    NotInHandShakeState,
    /// Unexpected state in the Noise protocol.
    UnexpectedNoiseState,
  };

  struct MissingBytes_Body {
    uintptr_t _0;
  };

  Tag tag;
  union {
    MissingBytes_Body missing_bytes;
  };
};

extern "C" {

/// Force `cbindgen` to create a header for [`CError`].
///
/// It ensures that [`CError`] is included in the generated C header file. This function is not meant
/// to be called and will panic if called. Its only purpose is to make [`CError`] visible to
/// `cbindgen`.
CError export_cerror();

} // extern "C"
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

struct DecoderWrapper;

struct EncoderWrapper;

struct CSv2Message {
  enum class Tag {
    CoinbaseOutputDataSize,
    NewTemplate,
    RequestTransactionData,
    RequestTransactionDataError,
    RequestTransactionDataSuccess,
    SetNewPrevHash,
    SubmitSolution,
    ChannelEndpointChanged,
    SetupConnection,
    SetupConnectionError,
    SetupConnectionSuccess,
  };

  struct CoinbaseOutputDataSize_Body {
    CoinbaseOutputDataSize _0;
  };

  struct NewTemplate_Body {
    CNewTemplate _0;
  };

  struct RequestTransactionData_Body {
    RequestTransactionData _0;
  };

  struct RequestTransactionDataError_Body {
    CRequestTransactionDataError _0;
  };

  struct RequestTransactionDataSuccess_Body {
    CRequestTransactionDataSuccess _0;
  };

  struct SetNewPrevHash_Body {
    CSetNewPrevHash _0;
  };

  struct SubmitSolution_Body {
    CSubmitSolution _0;
  };

  struct ChannelEndpointChanged_Body {
    ChannelEndpointChanged _0;
  };

  struct SetupConnection_Body {
    CSetupConnection _0;
  };

  struct SetupConnectionError_Body {
    CSetupConnectionError _0;
  };

  struct SetupConnectionSuccess_Body {
    SetupConnectionSuccess _0;
  };

  Tag tag;
  union {
    CoinbaseOutputDataSize_Body coinbase_output_data_size;
    NewTemplate_Body new_template;
    RequestTransactionData_Body request_transaction_data;
    RequestTransactionDataError_Body request_transaction_data_error;
    RequestTransactionDataSuccess_Body request_transaction_data_success;
    SetNewPrevHash_Body set_new_prev_hash;
    SubmitSolution_Body submit_solution;
    ChannelEndpointChanged_Body channel_endpoint_changed;
    SetupConnection_Body setup_connection;
    SetupConnectionError_Body setup_connection_error;
    SetupConnectionSuccess_Body setup_connection_success;
  };
};

struct Sv2Error {
  enum class Tag {
    BinaryError,
    CodecError,
    EncoderBusy,
    InvalidSv2Frame,
    MissingBytes,
    PayloadTooBig,
    Unknown,
  };

  struct BinaryError_Body {
    CError _0;
  };

  struct CodecError_Body {
    CError _0;
  };

  struct PayloadTooBig_Body {
    CVec _0;
  };

  Tag tag;
  union {
    BinaryError_Body binary_error;
    CodecError_Body codec_error;
    PayloadTooBig_Body payload_too_big;
  };
};

template<typename T, typename E>
struct CResult {
  enum class Tag {
    Ok,
    Err,
  };

  struct Ok_Body {
    T _0;
  };

  struct Err_Body {
    E _0;
  };

  Tag tag;
  union {
    Ok_Body ok;
    Err_Body err;
  };
};

extern "C" {

void drop_sv2_message(CSv2Message s);

/// This function does nothing unless there is some heap allocated data owned by the C side that
/// needs to be dropped (specifically a `CVec`). In this case, `free_vec` is used in order to drop
/// that memory.
void drop_sv2_error(Sv2Error s);

bool is_ok(const CResult<CSv2Message, Sv2Error> *cresult);

EncoderWrapper *new_encoder();

void flush_encoder(EncoderWrapper *encoder);

void free_decoder(DecoderWrapper *decoder);

/// # Safety
///
CResult<CVec, Sv2Error> encode(CSv2Message *message, EncoderWrapper *encoder);

DecoderWrapper *new_decoder();

CVec get_writable(DecoderWrapper *decoder);

CResult<CSv2Message, Sv2Error> next_frame(DecoderWrapper *decoder);

} // extern "C"
