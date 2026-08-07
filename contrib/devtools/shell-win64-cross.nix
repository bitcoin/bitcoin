{ pkgs ? import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/refs/heads/nixos-26.05.tar.gz";
  }) {} }:

let
  host = builtins.getEnv "HOST";
  crossPkgs = if host == "x86_64-w64-mingw32ucrt"
    then pkgs.pkgsCross.ucrt64
    else if host == "x86_64-w64-mingw32"
      then pkgs.pkgsCross.mingwW64
      else throw "Unsupported HOST: ${host}";
  toolchain = "x86_64-w64-mingw32";
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
    export CC=${toolchain}-gcc
    export CXX=${toolchain}-g++
    export LD=${toolchain}-ld
    export AR=${toolchain}-ar
    export AS=${toolchain}-as
    export RANLIB=${toolchain}-ranlib
    export NM=${toolchain}-nm
    export STRIP=${toolchain}-strip
    export OBJCOPY=${toolchain}-objcopy
    export OBJDUMP=${toolchain}-objdump
    export READELF=${toolchain}-readelf
    export SIZE=${toolchain}-size
    export WINDRES=${toolchain}-windres
    export RC=${toolchain}-windres
  '';
}
