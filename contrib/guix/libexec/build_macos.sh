#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
export LC_ALL=C
set -o errexit -o pipefail

# shellcheck source=setup.sh
source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"

# Set toolchain
CLANG_TOOLCHAIN="$(store_path clang-toolchain)"
LIBCXX="$(store_path libcxx)"
build_CC="${CLANG_TOOLCHAIN}/bin/clang \
    -isystem ${CLANG_TOOLCHAIN}/include"
build_CXX="${CLANG_TOOLCHAIN}/bin/clang++ \
    -stdlib=libc++ \
    -isystem ${LIBCXX}/include/c++/v1 \
    -isystem ${CLANG_TOOLCHAIN}/include"
build_LDFLAGS="-fuse-ld=lld -rtlib=compiler-rt -unwindlib=libunwind -L${LIBCXX}/lib -Wl,-rpath,${LIBCXX}/lib"
build_AR="${CLANG_TOOLCHAIN}/bin/llvm-ar"
build_RANLIB="${CLANG_TOOLCHAIN}/bin/llvm-ranlib"
build_OBJDUMP="${CLANG_TOOLCHAIN}/bin/llvm-objdump"
build_NM="${CLANG_TOOLCHAIN}/bin/llvm-nm"
build_STRIP="${CLANG_TOOLCHAIN}/bin/llvm-strip"

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
          -Werror=dev \
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
