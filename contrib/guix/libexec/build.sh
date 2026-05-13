#!/usr/bin/env bash
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
export LC_ALL=C
set -o errexit -o pipefail

# shellcheck source=setup.sh
source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"

# Set environment variables to point the NATIVE toolchain to the right
# includes/libs
NATIVE_GCC="$(store_path gcc-toolchain)"

# Set native toolchain
build_CC="${NATIVE_GCC}/bin/gcc -isystem ${NATIVE_GCC}/include"
build_CXX="${NATIVE_GCC}/bin/g++ -isystem ${NATIVE_GCC}/include/c++ -isystem ${NATIVE_GCC}/include"

# Required for native packages
export LIBRARY_PATH="${NATIVE_GCC}/lib"

# Set environment variables to point the CROSS toolchain to the right
# includes/libs for $HOST
case "$HOST" in
    *mingw*)
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
        ;;
    *darwin*)
        # The CROSS toolchain for darwin uses the SDK and ignores environment variables.
        # See depends/hosts/darwin.mk for more details.
        ;;
    *)
        exit 1 ;;
esac

# Sanity check CROSS_*_PATH directories
IFS=':' read -ra PATHS <<< "${CROSS_C_INCLUDE_PATH}:${CROSS_CPLUS_INCLUDE_PATH}:${CROSS_LIBRARY_PATH}"
for p in "${PATHS[@]}"; do
    if [ -n "$p" ] && [ ! -d "$p" ]; then
        echo "'$p' doesn't exist or isn't a directory... Aborting..."
        exit 1
    fi
done

####################
# Depends Building #
####################

# Build the depends tree
make -C depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${SDK_PATH+SDK_PATH="$SDK_PATH"} \
                                   ${build_CC+build_CC="$build_CC"} \
                                   ${build_CXX+build_CXX="$build_CXX"}

case "$HOST" in
    *darwin*)
        # Unset now that Qt is built
        unset LIBRARY_PATH
        ;;
esac

###########################
# Binary Tarball Building #
###########################

# CONFIGFLAGS
CONFIGFLAGS="-DREDUCE_EXPORTS=ON -DBUILD_BENCH=OFF -DBUILD_GUI_TESTS=OFF -DBUILD_FUZZ_BINARY=OFF -DCMAKE_SKIP_RPATH=TRUE"

# CFLAGS
HOST_CFLAGS="-O2 -g"
HOST_CFLAGS+=$(find /gnu/store -maxdepth 1 -mindepth 1 -type d -exec echo -n " -ffile-prefix-map={}=/usr" \;)
HOST_CFLAGS+=" -fdebug-prefix-map=${DISTSRC}/src=."
case "$HOST" in
    *mingw*)  HOST_CFLAGS+=" -fno-ident" ;;
    *darwin*) unset HOST_CFLAGS ;;
esac

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"

# LDFLAGS
case "$HOST" in
    *mingw*)  HOST_LDFLAGS="-Wl,--no-insert-timestamp" ;;
esac

mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    # Configure this DISTSRC for $HOST
    # shellcheck disable=SC2086
    env CFLAGS="${HOST_CFLAGS}" CXXFLAGS="${HOST_CXXFLAGS}" LDFLAGS="${HOST_LDFLAGS}" \
    cmake -S . -B build \
          --toolchain "${BASEPREFIX}/${HOST}/toolchain.cmake" \
          -DWITH_CCACHE=OFF \
          -Werror=dev \
          ${CONFIGFLAGS}

    # Build Bitcoin Core
    cmake --build build -j "$JOBS"

    # Make the os-specific installers
    case "$HOST" in
        *mingw*)
            cmake --build build -j "$JOBS" -t deploy
            mv build/bitcoin-win64-setup.exe "${OUTDIR}/${DISTNAME}-win64-setup-unsigned.exe"
            ;;
    esac

    # Setup the directory where our Bitcoin Core build for HOST will be
    # installed. This directory will also later serve as the input for our
    # binary tarballs.
    mkdir -p "${INSTALLPATH}"
    # Install built Bitcoin Core to $INSTALLPATH
    case "$HOST" in
        *darwin*)
            cmake --install build --strip --prefix "${INSTALLPATH}"
            ;;
        *)
            cmake --install build --prefix "${INSTALLPATH}"
            ;;
    esac
)  # $DISTSRC

# shellcheck source=package.sh
source "$(dirname "${BASH_SOURCE[0]}")/package.sh"
