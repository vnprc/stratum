{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = github:nixos/nixpkgs/nixos-24.05;
    flake-utils.url = github:numtide/flake-utils;
  };

  outputs = { self, nixpkgs, flake-utils }:
  flake-utils.lib.eachDefaultSystem (system:
  let
      pkgs = import nixpkgs { inherit system; };

      run-local-pool = pkgs.writeScriptBin "run-local-pool"
        ''
        cargo run -- -c roles/pool/config-examples/pool-config-local-tp-example.toml
	'';

      run-job-server = pkgs.writeScriptBin "run-job-server"
        ''
        cargo run -- -c roles/jd-server/config-examples/jds-config-local-example.toml
	'';

      run-job-client = pkgs.writeScriptBin "run-job-client"
        ''
        cargo run -- -c roles/jd-client/config-examples/jds-config-local-example.toml
	'';

      run-translator-proxy = pkgs.writeScriptBin "run-translator-proxy"
        ''
        cargo run -- -c roles/translator/config-examples/tproxy-config-local-jdc-example.toml
	'';
      run-bitcoind = pkgs.writeScriptBin "run-bitcoind"
        ''
	'';
  in {
    devShells.default = pkgs.mkShell {
        buildInputs = with pkgs; [ rustc cargo bitcoind ];
	shellHook = ''
          ${run-bitcoind}/bin/run-bitcoind
	  ${run-local-pool}/bin/run-local-pool
	  ${run-job-server}/bin/run-job-server
	  ${run-job-client}/bin/run-job-client
	  ${run-translator-proxy}/bin/run-translator-proxy
	'';
    };
  });
}
