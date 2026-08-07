#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o nounset -o pipefail -o xtrace

[ "${CI_CONFIG+x}" ] && source "$CI_CONFIG"

: "${CI_DIR:=build}"
if ! [ -v BUILD_TARGETS ]; then
  BUILD_TARGETS=(all tests mpexamples)
fi

[ -n "${CI_CLEAN-}" ] && rm -rf "${CI_DIR}"

cmake --version
cmake_ver=$(cmake --version | awk '/version/{print $3; exit}')
ver_ge() { [ "$(printf '%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]; }

src_dir=$PWD

# If cross-compiling, build native mpgen first so it can be used as a code
# generator when cmake invokes it from add_custom_command (which does not go
# through CMAKE_CROSSCOMPILING_EMULATOR, unlike add_test executables).
if [ -n "${MPGEN_PRE_BUILD-}" ]; then
  native_dir="${src_dir}/${CI_DIR}-native"
  [ -n "${CI_CLEAN-}" ] && rm -rf "$native_dir"
  mkdir -p "$native_dir"
  # Unset cross-compilation env vars so cmake uses the native compiler and
  # does a native (not cross) build.  Key vars to clear:
  #   CXX/CC/AR/RANLIB/LD - set to cross-compiler by the cross shell
  #   cmakeFlags - nix cross shell injects -DCMAKE_SYSTEM_NAME=Windows etc.
  # Pass NATIVE_CAPNPROTO_PREFIX as CMAKE_PREFIX_PATH so cmake finds the
  # native Cap'n Proto rather than the cross-compiled one.
  native_cmake_args=()
  if [ -n "${NATIVE_CAPNPROTO_PREFIX-}" ]; then
    # Build a cmake prefix path with native capnproto and its dependencies
    # (openssl, zlib) so find_package and find_dependency succeed.
    native_prefix="${NATIVE_CAPNPROTO_PREFIX}"
    [ -n "${NATIVE_OPENSSL_DEV-}" ] && native_prefix="${native_prefix};${NATIVE_OPENSSL_DEV}"
    [ -n "${NATIVE_OPENSSL_LIB-}" ] && native_prefix="${native_prefix};${NATIVE_OPENSSL_LIB}"
    [ -n "${NATIVE_ZLIB_DEV-}" ]    && native_prefix="${native_prefix};${NATIVE_ZLIB_DEV}"
    [ -n "${NATIVE_ZLIB_LIB-}" ]    && native_prefix="${native_prefix};${NATIVE_ZLIB_LIB}"
    native_cmake_args+=(
      "-DCMAKE_PREFIX_PATH=${native_prefix}"
      "-DCapnProto_DIR=${NATIVE_CAPNPROTO_PREFIX}/lib/cmake/CapnProto"
    )
  fi
  # -static-libstdc++ / -static-libgcc: the native mpgen must run inside the
  # cross nix shell where the native libstdc++.so may not be in LD_LIBRARY_PATH.
  native_cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc")
  (cd "$native_dir" && env -u CXX -u CC -u AR -u RANLIB -u LD -u cmakeFlags cmake "$src_dir" "${native_cmake_args[@]+${native_cmake_args[@]}}" && cmake --build . -t mpgen)
  CMAKE_ARGS+=("-DMPGEN_EXECUTABLE=${native_dir}/mpgen")

  # Override capnp tool executables: the cross capnproto cmake config sets
  # CAPNP_EXECUTABLE to capnp.exe (Windows binary), which can't run on Linux.
  # Use the native capnp/capnpc-c++ binaries from pkgs.capnproto in nativeBuildInputs.
  _capnp=$(command -v capnp 2>/dev/null || true)
  _capnpc=$(command -v capnpc-c++ 2>/dev/null || true)
  [ -n "$_capnp" ]   && CMAKE_ARGS+=("-DCAPNP_EXECUTABLE=$_capnp")
  [ -n "$_capnpc" ]  && CMAKE_ARGS+=("-DCAPNPC_CXX_EXECUTABLE=$_capnpc")
fi

mkdir -p "$CI_DIR"
cd "$CI_DIR"
cmake "$src_dir" "${CMAKE_ARGS[@]+"${CMAKE_ARGS[@]}"}"
if ver_ge "$cmake_ver" "3.15"; then
  cmake --build . -t "${BUILD_TARGETS[@]}" -- "${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"}"
else
  # Older versions of cmake can only build one target at a time with --target,
  # and do not support -t shortcut
  for t in "${BUILD_TARGETS[@]}"; do
    cmake --build . --target "$t" -- "${BUILD_ARGS[@]+"${BUILD_ARGS[@]}"}"
  done
fi
# When cross-compiling for Windows, copy GCC and MCF thread runtime DLLs
# alongside the test executables so wine can find them (wine DLL loading
# checks the executable's directory first, before any search-path logic).
if [ -n "${WIN_RUNTIME_DLLS-}" ]; then
  IFS=: read -ra _dll_dirs <<< "$WIN_RUNTIME_DLLS"
  for _dir in "${_dll_dirs[@]}"; do
    find "$_dir" -maxdepth 1 -name "*.dll" -exec cp -n {} test/ \;
  done
fi
ctest --output-on-failure
