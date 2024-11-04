{ toolchain ? "",
  rev       ? "08ef0f28e3a41424b92ba1d203de64257a9fca6a",
  sha256  ? "1mql1gp86bk6pfsrp0lcww6hw5civi6f8542d4nh356506jdxmcy",
  nixpkgs ? builtins.fetchTarball {
    name   = "nixpkgs-${rev}";
    url    = "https://github.com/nixos/nixpkgs/archive/${rev}.tar.gz";
    sha256 = sha256;
  },
}:

with import nixpkgs {};

let
  # For the documentation tools we use an older Nixpkgs since the
  # newer versions seem to be not working great...
  oldNixpkgsSrc = fetchFromGitHub {
                    owner  = "NixOS";
                    repo   = "nixpkgs";
                    rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
                    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
                  };
  oldNixpkgs    = import oldNixpkgsSrc {};
  docs          = import ./nix/docs.nix { nixpkgs = oldNixpkgsSrc; };
  benchmarks    = import ./nix/benchmarks.nix { inherit nixpkgs; };
  tc            =
    if toolchain == ""         then { stdenv = stdenv; cc = stdenv.cc; } else
    if toolchain == "gnu-6"    then { stdenv = gcc6Stdenv; cc = gcc6; } else
    if toolchain == "gnu-7"    then { stdenv = gcc7Stdenv; cc = gcc7; } else
    if toolchain == "gnu-8"    then { stdenv = gcc8Stdenv; cc = gcc8; } else
    if toolchain == "gnu-9"    then { stdenv = gcc9Stdenv; cc = gcc9; } else
    if toolchain == "gnu-10"   then { stdenv = gcc10Stdenv; cc = gcc10; } else
    if toolchain == "gnu-11"   then { stdenv = gcc11Stdenv; cc = gcc11; } else
    if toolchain == "llvm-39"  then { stdenv = llvmPackages_39.libcxxStdenv; cc = llvmPackages_39.libcxxClang; } else
    if toolchain == "llvm-4"   then { stdenv = llvmPackages_4.libcxxStdenv; cc = llvmPackages_4.libcxxClang; } else
    if toolchain == "llvm-5"   then { stdenv = llvmPackages_5.libcxxStdenv; cc = llvmPackages_5.libcxxClang; } else
    if toolchain == "llvm-6"   then { stdenv = llvmPackages_6.libcxxStdenv; cc = llvmPackages_6.libcxxClang; } else
    if toolchain == "llvm-7"   then { stdenv = llvmPackages_7.libcxxStdenv; cc = llvmPackages_7.libcxxClang; } else
    if toolchain == "llvm-8"   then { stdenv = llvmPackages_8.libcxxStdenv; cc = llvmPackages_8.libcxxClang; } else
    if toolchain == "llvm-9"   then { stdenv = llvmPackages_9.stdenv;  cc = llvmPackages_9.clang; } else
    if toolchain == "llvm-10"  then { stdenv = llvmPackages_10.stdenv; cc = llvmPackages_10.clang; } else
    if toolchain == "llvm-11"  then { stdenv = llvmPackages_11.stdenv; cc = llvmPackages_11.clang; } else
    if toolchain == "llvm-12"  then { stdenv = llvmPackages_12.stdenv; cc = llvmPackages_12.clang; } else
    if toolchain == "llvm-13"  then { stdenv = llvmPackages_13.stdenv; cc = llvmPackages_13.clang; } else
    abort "unknown toolchain";

in
tc.stdenv.mkDerivation rec {
  name = "immer-env";
  buildInputs = [
    tc.cc
    git
    catch2
    cmake
    pkgconfig
    ninja
    gdb
    lldb
    ccache
    boost
    boehmgc
    fmt
    valgrind
    benchmarks.c_rrb
    benchmarks.steady
    benchmarks.chunkedseq
    benchmarks.immutable_cpp
    benchmarks.hash_trie
    oldNixpkgs.doxygen
    (oldNixpkgs.python.withPackages (ps: [
      ps.sphinx
      docs.breathe
      docs.recommonmark
    ]))
  ];
  hardeningDisable = [ "fortify" ];
}
