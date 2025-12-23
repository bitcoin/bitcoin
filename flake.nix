{
  description = "bitcoind for benchmarking";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-darwin"
      ];

      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);

      pkgsFor = system: import nixpkgs { inherit system; };

      mkBitcoinCore =
        system:
        let
          pkgs = pkgsFor system;
          inherit (pkgs) lib;

          pname = "bitcoin-core";
          version = self.shortRev or "dirty";

          CFlags = toString [
            "-O2"
            "-g"
          ];
          CXXFlags = "${CFlags} -fno-omit-frame-pointer";

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
            pkgs.python3
          ];

          buildInputs = [
            pkgs.boost188.dev
            pkgs.libevent.dev
          ];

          cmakeFlags = [
            "-DBUILD_BENCH=OFF"
            "-DBUILD_BITCOIN_BIN=OFF"
            "-DBUILD_CLI=OFF"
            "-DBUILD_DAEMON=ON"
            "-DBUILD_FUZZ_BINARY=OFF"
            "-DBUILD_GUI_TESTS=OFF"
            "-DBUILD_TESTS=OFF"
            "-DBUILD_TX=OFF"
            "-DBUILD_UTIL=OFF"
            "-DBUILD_WALLET_TOOL=OFF"
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            "-DCMAKE_SKIP_RPATH=ON"
            "-DENABLE_EXTERNAL_SIGNER=OFF"
            "-DENABLE_IPC=OFF"
            "-DENABLE_WALLET=OFF"
            "-DREDUCE_EXPORTS=ON"
            "-DWITH_ZMQ=OFF"
          ];
        in
        pkgs.stdenv.mkDerivation {
          inherit
            pname
            version
            nativeBuildInputs
            buildInputs
            cmakeFlags
            ;

          preConfigure = ''
            cmakeFlagsArray+=(
              "-DAPPEND_CFLAGS=${CFlags}"
              "-DAPPEND_CXXFLAGS=${CXXFlags}"
              "-DAPPEND_LDFLAGS=-Wl,--as-needed -Wl,-O2"
            )
          '';

          src = builtins.path {
            path = ./.;
            name = "source";
          };

          env = {
            CMAKE_GENERATOR = "Ninja";
            LC_ALL = "C";
            LIBRARY_PATH = "";
            CPATH = "";
            C_INCLUDE_PATH = "";
            CPLUS_INCLUDE_PATH = "";
            OBJC_INCLUDE_PATH = "";
            OBJCPLUS_INCLUDE_PATH = "";
          };

          dontStrip = true;

          meta = {
            description = "bitcoind for benchmarking";
            homepage = "https://bitcoincore.org/";
            license = lib.licenses.mit;
          };
        };
    in
    {
      packages = forAllSystems (system: {
        default = mkBitcoinCore system;
      });

      formatter = forAllSystems (system: (pkgsFor system).nixfmt-tree);

      devShells = forAllSystems (
        system:
        let
          pkgs = pkgsFor system;
          inherit (pkgs) stdenv;

          # Override the default cargo-flamegraph with a custom fork including bitcoin highlighting
          cargo-flamegraph = pkgs.rustPlatform.buildRustPackage rec {
            pname = "flamegraph";
            version = "bitcoin-core";

            src = pkgs.fetchFromGitHub {
              owner = "willcl-ark";
              repo = "flamegraph";
              rev = "bitcoin-core";
              sha256 = "sha256-tQbr3MYfAiOxeT12V9au5KQK5X5JeGuV6p8GR/Sgen4=";
            };

            doCheck = false;
            cargoHash = "sha256-QWPqTyTFSZNJNayNqLmsQSu0rX26XBKfdLROZ9tRjrg=";

            nativeBuildInputs = pkgs.lib.optionals stdenv.hostPlatform.isLinux [ pkgs.makeWrapper ];
            buildInputs = pkgs.lib.optionals stdenv.hostPlatform.isDarwin [
              pkgs.darwin.apple_sdk.frameworks.Security
            ];

            postFixup = pkgs.lib.optionalString stdenv.hostPlatform.isLinux ''
              wrapProgram $out/bin/cargo-flamegraph \
                --set-default PERF ${pkgs.perf}/bin/perf
              wrapProgram $out/bin/flamegraph \
                --set-default PERF ${pkgs.perf}/bin/perf
            '';
          };
        in
        {
          default = pkgs.mkShell {
            buildInputs = [
              # Benchmarking
              cargo-flamegraph
              pkgs.flamegraph
              pkgs.hyperfine
              pkgs.jq
              pkgs.just
              pkgs.perf
              pkgs.perf-tools
              pkgs.python312
              pkgs.python312Packages.matplotlib
              pkgs.util-linux

              # Binary patching
              pkgs.patchelf
            ];
          };
        }
      );
    };
}
