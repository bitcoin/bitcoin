#!/usr/bin/env bash
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
export LC_ALL=C
set -e -o pipefail

# shellcheck source=setup.sh
source "$(dirname "${BASH_SOURCE[0]}")/setup.sh"

# Set environment variables to point the NATIVE toolchain to the right
# includes/libs
NATIVE_GCC="$(store_path gcc-toolchain)"

# Set native toolchain
build_CC="${NATIVE_GCC}/bin/gcc -isystem ${NATIVE_GCC}/include"
build_CXX="${NATIVE_GCC}/bin/g++ -isystem ${NATIVE_GCC}/include/c++ -isystem ${NATIVE_GCC}/include"

case "$HOST" in
    *darwin*) export LIBRARY_PATH="${NATIVE_GCC}/lib" ;; # Required for native packages
    *mingw*) export LIBRARY_PATH="${NATIVE_GCC}/lib" ;;
    *)
        NATIVE_GCC_STATIC="$(store_path gcc-toolchain static)"
        export LIBRARY_PATH="${NATIVE_GCC}/lib:${NATIVE_GCC_STATIC}/lib"
        ;;
esac

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
    *linux*)
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

# Determine the correct value for -Wl,--dynamic-linker for the current $HOST
case "$HOST" in
    *linux*)
        glibc_dynamic_linker=$(
            case "$HOST" in
                x86_64-linux-gnu)      echo /lib64/ld-linux-x86-64.so.2 ;;
                arm-linux-gnueabihf)   echo /lib/ld-linux-armhf.so.3 ;;
                aarch64-linux-gnu)     echo /lib/ld-linux-aarch64.so.1 ;;
                riscv64-linux-gnu)     echo /lib/ld-linux-riscv64-lp64d.so.1 ;;
                powerpc64-linux-gnu)   echo /lib64/ld64.so.1;;
                powerpc64le-linux-gnu) echo /lib64/ld64.so.2;;
                *)                     exit 1 ;;
            esac
        )
        ;;
esac

####################
# Depends Building #
####################

# Build the depends tree, overriding variables that assume multilib gcc
make -C depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${SDK_PATH+SDK_PATH="$SDK_PATH"} \
                                   ${build_CC+build_CC="$build_CC"} \
                                   ${build_CXX+build_CXX="$build_CXX"} \
                                   x86_64_linux_CC=x86_64-linux-gnu-gcc \
                                   x86_64_linux_CXX=x86_64-linux-gnu-g++ \
                                   x86_64_linux_AR=x86_64-linux-gnu-gcc-ar \
                                   x86_64_linux_RANLIB=x86_64-linux-gnu-gcc-ranlib \
                                   x86_64_linux_NM=x86_64-linux-gnu-gcc-nm \
                                   x86_64_linux_STRIP=x86_64-linux-gnu-strip

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

case "$HOST" in
    arm-linux-gnueabihf) HOST_CXXFLAGS="${HOST_CXXFLAGS} -Wno-psabi" ;;
esac

# LDFLAGS
case "$HOST" in
    *linux*)  HOST_LDFLAGS="-Wl,--as-needed -Wl,--dynamic-linker=$glibc_dynamic_linker -Wl,-O2" ;;
    *mingw*)  HOST_LDFLAGS="-Wl,--no-insert-timestamp" ;;
esac

# EXE FLAGS
case "$HOST" in
    *linux*)  CMAKE_EXE_LINKER_FLAGS="-DCMAKE_EXE_LINKER_FLAGS=${HOST_LDFLAGS} -static-libstdc++ -static-libgcc" ;;
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
          ${CONFIGFLAGS} \
          ${CMAKE_EXE_LINKER_FLAGS+"$CMAKE_EXE_LINKER_FLAGS"}

    # Build Bitcoin Core
    cmake --build build -j "$JOBS"

    mkdir -p "$OUTDIR"

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

    # Perform basic security checks on installed executables.
    echo "Checking binary security on installed executables..."
    python3 "${DISTSRC}/contrib/guix/security-check.py" "${INSTALLPATH}/bin/"* "${INSTALLPATH}/libexec/"*
    # Check that executables only contain allowed version symbols.
    echo "Running symbol and dynamic library checks on installed executables..."
    python3 "${DISTSRC}/contrib/guix/symbol-check.py" "${INSTALLPATH}/bin/"* "${INSTALLPATH}/libexec/"*
)  # $DISTSRC

# shellcheck source=package.sh
source "$(dirname "${BASH_SOURCE[0]}")/package.sh"
