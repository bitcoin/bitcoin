{ pkgs ? import <nixpkgs> {}
, crossPkgs ? import <nixpkgs> {}
, enableLibcxx ? false # Whether to use libc++ toolchain and libraries instead of libstdc++
, minimal ? false # Whether to create minimal shell without extra tools (faster when cross compiling)
}:

let
  lib  = pkgs.lib;
  llvm = crossPkgs.llvmPackages_20;
  capnproto = crossPkgs.capnproto.override (lib.optionalAttrs enableLibcxx { clangStdenv = llvm.libcxxStdenv; });
  clang = if enableLibcxx then llvm.libcxxClang else llvm.clang;
  clang-tools = llvm.clang-tools.override { inherit enableLibcxx; };
in crossPkgs.mkShell {
  buildInputs = [
    capnproto
  ];
  nativeBuildInputs = with pkgs; [
    cmake
    include-what-you-use
    ninja
  ] ++ lib.optionals (!minimal) [
    clang
    clang-tools
  ];

  # Tell IWYU where its libc++ mapping lives
  IWYU_MAPPING_FILE = if enableLibcxx then "${llvm.libcxx.dev}/include/c++/v1/libcxx.imp" else null;
}
