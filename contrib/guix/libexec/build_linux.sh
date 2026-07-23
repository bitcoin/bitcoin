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

NATIVE_GCC_STATIC="$(store_path gcc-toolchain static)"
export LIBRARY_PATH="${NATIVE_GCC}/lib:${NATIVE_GCC_STATIC}/lib"

# Set environment variables to point the CROSS toolchain to the right
# includes/libs for $HOST
CROSS_GLIBC="$(store_path "glibc-cross-${HOST}")"
CROSS_GLIBC_STATIC="$(store_path "glibc-cross-${HOST}" static)"
CROSS_KERNEL="$(store_path "linux-libre-headers-cross-${HOST}")"
CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
CROSS_GCC_LIB_STORE="$(store_path "gcc-cross-${HOST}" lib)"
CROSS_GCC_LIBS=( "${CROSS_GCC_LIB_STORE}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GCC_LIB}/include-fixed:${CROSS_GLIBC}/include:${CROSS_KERNEL}/include"
export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
export CROSS_LIBRARY_PATH="${CROSS_GCC_LIB_STORE}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib:${CROSS_GLIBC_STATIC}/lib"

check_cross_paths "${CROSS_C_INCLUDE_PATH}:${CROSS_CPLUS_INCLUDE_PATH}:${CROSS_LIBRARY_PATH}"

# Build the depends tree, overriding variables that assume multilib gcc
make -C depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${build_CC+build_CC="$build_CC"} \
                                   ${build_CXX+build_CXX="$build_CXX"} \
                                   x86_64_linux_CC=x86_64-linux-gnu-gcc \
                                   x86_64_linux_CXX=x86_64-linux-gnu-g++ \
                                   x86_64_linux_AR=x86_64-linux-gnu-gcc-ar \
                                   x86_64_linux_RANLIB=x86_64-linux-gnu-gcc-ranlib \
                                   x86_64_linux_NM=x86_64-linux-gnu-gcc-nm \
                                   x86_64_linux_STRIP=x86_64-linux-gnu-strip \
                                   NO_QT=1

# CFLAGS
HOST_CFLAGS="-O2 -g"
HOST_CFLAGS+=$(find /gnu/store -maxdepth 1 -mindepth 1 -type d -exec echo -n " -ffile-prefix-map={}=/usr" \;)
HOST_CFLAGS+=" -fdebug-prefix-map=${DISTSRC}/src=."

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"

case "$HOST" in
    arm-linux-gnueabihf) HOST_CXXFLAGS="${HOST_CXXFLAGS} -Wno-psabi" ;;
esac

# LDFLAGS
HOST_LDFLAGS="-Wl,--as-needed -Wl,--dynamic-linker=$(glibc_dynamic_linker "$HOST") -Wl,-O2"

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
          -DBUILD_FUZZ_BINARY=OFF \
          -DBUILD_GUI=OFF \
          -DCMAKE_INSTALL_PREFIX="${INSTALLPATH}" \
          -DCMAKE_SKIP_RPATH=TRUE \
          -DREDUCE_EXPORTS=ON \
          -DCMAKE_EXE_LINKER_FLAGS="${HOST_LDFLAGS} -static-libstdc++ -static-libgcc" \
          -DWITH_CCACHE=OFF

    # Build Bitcoin Core
    cmake --build build -j "$JOBS"

    # Install built Bitcoin Core
    cmake --install build
)

rm -rf "$DISTSRC"/build

exit 0
