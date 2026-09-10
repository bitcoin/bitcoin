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

if [ -n "${BASE_CACHE_BUILD:-}" ]; then
    BASE_CACHE="$BASE_CACHE_BUILD"
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
                                   ${build_STRIP+build_STRIP="$build_STRIP"} \
                                   NO_QT=1

mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    # Configure this DISTSRC for $HOST
    env cmake -S . -B build \
          --toolchain "${BASEPREFIX}/${HOST}/toolchain.cmake" \
          -DBUILD_BENCH=OFF \
          -DBUILD_FUZZ_BINARY=OFF \
          -DBUILD_GUI=OFF \
          -DBUILD_GUI_TESTS=OFF \
          -DCMAKE_INSTALL_PREFIX="${INSTALLPATH}" \
          -DCMAKE_SKIP_RPATH=TRUE \
          -DREDUCE_EXPORTS=ON \
          -DWITH_CCACHE=OFF

    # Build Bitcoin Core
    cmake --build build -j "$JOBS"

    # Install built Bitcoin Core
    cmake --install build --strip
)

rm -rf "$DISTSRC"/build

exit 0
