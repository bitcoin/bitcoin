# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

{ pkgs ? import (builtins.fetchTarball {
    # Pin, to keep the versions of the toolchain aligned with the versions used by Guix.
    url = "https://github.com/NixOS/nixpkgs/archive/531670d871c0e29724a02f3cbcac170adc65b58c.tar.gz";
  }) {} }:

let
  host = builtins.getEnv "HOST";
  baseCrossPkgs = if host == "x86_64-w64-mingw32ucrt"
    then pkgs.pkgsCross.ucrt64
    else if host == "x86_64-w64-mingw32"
      then pkgs.pkgsCross.mingwW64
      else throw "Unsupported HOST: ${host}";
  crossPkgs = baseCrossPkgs.extend (_: _: {
    threads = {
      model = "posix";
      package = null;
    };
  });
  toolchain = crossPkgs.stdenv.cc.targetPrefix;
  pthreads = crossPkgs.windows.pthreads;
  crossGcc = crossPkgs.buildPackages.gcc14;
  gcc = crossGcc.override {
    cc = crossGcc.cc.overrideAttrs (old: {
      # Keep target pthread headers out of GCC's native build tools.
      env = old.env // {
        EXTRA_FLAGS_FOR_TARGET =
          "${old.env.EXTRA_FLAGS_FOR_TARGET} -idirafter ${pthreads}/include -B${pthreads}/lib";
        EXTRA_LDFLAGS_FOR_TARGET =
          "${old.env.EXTRA_LDFLAGS_FOR_TARGET} -Wl,-L${pthreads}/lib";
      };
    });
    extraPackages = [ pthreads ];
  };
in

pkgs.mkShellNoCC {
  packages = [ gcc pkgs.nsis ];

  shellHook = ''
    export CC=$(command -v ${toolchain}gcc)
    export CXX=$(command -v ${toolchain}g++)
    export LD=$(command -v ${toolchain}ld)
    export AR=$(command -v ${toolchain}ar)
    export AS=$(command -v ${toolchain}as)
    export RANLIB=$(command -v ${toolchain}ranlib)
    export NM=$(command -v ${toolchain}nm)
    export STRIP=$(command -v ${toolchain}strip)
    export OBJCOPY=$(command -v ${toolchain}objcopy)
    export OBJDUMP=$(command -v ${toolchain}objdump)
    export READELF=$(command -v ${toolchain}readelf)
    export SIZE=$(command -v ${toolchain}size)
    export WINDRES=$(command -v ${toolchain}windres)
    export RC=$(command -v ${toolchain}windres)
  '';
}
