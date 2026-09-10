#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
export LC_ALL=C.UTF-8
set -o errexit -o pipefail

# shellcheck source=setup.sh
source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"

# Setup toolchain
llvm_toolchain

if [ -n "${BASE_CACHE_GUI:-}" ]; then
    BASE_CACHE="$BASE_CACHE_GUI"
fi

# Build the depends tree
make -C depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${SDK_PATH+SDK_PATH="$SDK_PATH"} \
                                   ${build_CC+build_CC="$build_CC"} \
                                   ${build_CXX+build_CXX="$build_CXX"} \
                                   ${build_LDFLAGS+build_LDFLAGS="$build_LDFLAGS"} \
                                   ${build_AR+build_AR="$build_AR"} \
                                   ${build_RANLIB+build_RANLIB="$build_RANLIB"} \
                                   ${build_OBJDUMP+build_OBJDUMP="$build_OBJDUMP"} \
                                   ${build_NM+build_NM="$build_NM"} \
                                   ${build_STRIP+build_STRIP="$build_STRIP"}

mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    # Configure this DISTSRC for $HOST
    env cmake -S . -B build \
          --toolchain "${BASEPREFIX}/${HOST}/toolchain.cmake" \
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
          -DCMAKE_SKIP_RPATH=TRUE \
          -DREDUCE_EXPORTS=ON \
          -DWITH_CCACHE=OFF \
          -Werror=dev

    # Build Bitcoin Core
    cmake --build build -j "$JOBS" --target bitcoin-gui bitcoin-qt

    # Install built Bitcoin Core
    cmake --install build --strip --component bitcoin-gui
    cmake --install build --strip --component bitcoin-qt
)

# shellcheck source=package.sh
source "$(dirname "${BASH_SOURCE[0]}")/package.sh"
