# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

{ pkgs ? import (builtins.fetchTarball {
    # Pin, to keep the versions of the toolchain aligned with the versions used by Guix.
    url = "https://github.com/NixOS/nixpkgs/archive/531670d871c0e29724a02f3cbcac170adc65b58c.tar.gz";
  }) {} }:

let
  host = builtins.getEnv "HOST";
  crossPkgs = if host == "x86_64-w64-mingw32ucrt"
    then pkgs.pkgsCross.ucrt64
    else if host == "x86_64-w64-mingw32"
      then pkgs.pkgsCross.mingwW64
      else throw "Unsupported HOST: ${host}";
  toolchain = crossPkgs.stdenv.cc.targetPrefix;
  pthreads = crossPkgs.windows.pthreads;
in

pkgs.mkShellNoCC {
  packages = [
    crossPkgs.gcc14
    pkgs.nsis
  ];

  shellHook = ''
    export NIX_CFLAGS_COMPILE="-isystem ${pthreads}/include $NIX_CFLAGS_COMPILE"
    export NIX_LDFLAGS="-L${pthreads}/lib $NIX_LDFLAGS"
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
