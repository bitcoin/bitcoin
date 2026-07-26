#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
export LC_ALL=C.UTF-8
set -o errexit -o pipefail

# shellcheck source=setup.sh
source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"

# Set environment variables to point the NATIVE toolchain to the right
# includes/libs
NATIVE_GCC="$(store_path gcc-toolchain)"

# Set native toolchain
build_CC="${NATIVE_GCC}/bin/gcc -isystem ${NATIVE_GCC}/include"
build_CXX="${NATIVE_GCC}/bin/g++ -isystem ${NATIVE_GCC}/include/c++ -isystem ${NATIVE_GCC}/include"

# Set environment variables to point the CROSS toolchain to the right
# includes/libs for $HOST
# Determine output paths to use in CROSS_* environment variables
CROSS_GLIBC="$(store_path "mingw-w64-x86_64-winpthreads")"
CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
CROSS_GCC_LIB_STORE="$(store_path "gcc-cross-${HOST}" lib)"
CROSS_GCC_LIBS=( "${CROSS_GCC_LIB_STORE}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

# The search path ordering is generally:
#    1. gcc-related search paths
#    2. libc-related search paths
#    2. kernel-header-related search paths (not applicable to mingw-w64 hosts)
export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GCC_LIB}/include-fixed:${CROSS_GLIBC}/include"
export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
export CROSS_LIBRARY_PATH="${CROSS_GCC_LIB_STORE}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib"

check_cross_paths "${CROSS_C_INCLUDE_PATH}:${CROSS_CPLUS_INCLUDE_PATH}:${CROSS_LIBRARY_PATH}"

# Build the depends tree
make -C depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${build_CC+build_CC="$build_CC"} \
                                   ${build_CXX+build_CXX="$build_CXX"}

# CFLAGS
HOST_CFLAGS="-O2 -g"
HOST_CFLAGS+=$(find /gnu/store -maxdepth 1 -mindepth 1 -type d -exec echo -n " -ffile-prefix-map={}=/usr" \;)
HOST_CFLAGS+=" -fdebug-prefix-map=${DISTSRC}/src=."
HOST_CFLAGS+=" -fno-ident"

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"

# LDFLAGS
HOST_LDFLAGS="-Wl,--no-insert-timestamp"

mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    # Configure this DISTSRC for $HOST
    env CFLAGS="${HOST_CFLAGS}" CXXFLAGS="${HOST_CXXFLAGS}" LDFLAGS="${HOST_LDFLAGS}" \
    cmake -S . -B build \
          --toolchain "${BASEPREFIX}/${HOST}/toolchain.cmake" \
          -Werror=dev \
          -DBUILD_BENCH=OFF \
          -DBUILD_BITCOIN_BIN=OFF \
          -DBUILD_CLI=OFF \
          -DBUILD_DAEMON=OFF \
          -DBUILD_FUZZ_BINARY=OFF \
          -DBUILD_GUI_TESTS=OFF \
          -DBUILD_TESTS=OFF \
          -DBUILD_TX=OFF \
          -DBUILD_UTIL=OFF \
          -DBUILD_WALLET_TOOL=OFF \
          -DCMAKE_INSTALL_PREFIX="${INSTALLPATH}" \
          -DREDUCE_EXPORTS=ON \
          -DWITH_CCACHE=OFF

    # Build Bitcoin Core
    cmake --build build -j "$JOBS" --target bitcoin-qt

    # Install built Bitcoin Core
    cmake --install build --component bitcoin-qt
)

# shellcheck source=package.sh
source "$(dirname "${BASH_SOURCE[0]}")/package.sh"
