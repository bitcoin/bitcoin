#!/usr/bin/env bash
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.
export LC_ALL=C.UTF-8
set -o errexit -o pipefail

# Environment variables for determinism
export TAR_OPTIONS="--no-same-owner --owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ=UTC

# Although Guix _does_ set umask when building its own packages (in our case,
# this is all packages in manifest.scm), it does not set it for `guix
# shell`. It does make sense for at least `guix shell --container`
# to set umask, so if that change gets merged upstream and we bump the
# time-machine to a commit which includes the aforementioned change, we can
# remove this line.
#
# This line should be placed before any commands which creates files.
umask 0022

if [ -n "$V" ]; then
    # Print both unexpanded (-v) and expanded (-x) forms of commands as they are
    # read from this file.
    set -vx
    # Set VERBOSE for CMake-based builds
    export VERBOSE="$V"
fi

# Check that required environment variables are set
cat << EOF
Required environment variables as seen inside the container:
    DIST_ARCHIVE_BASE: ${DIST_ARCHIVE_BASE:?not set}
    DISTNAME: ${DISTNAME:?not set}
    HOST: ${HOST:?not set}
    SOURCE_DATE_EPOCH: ${SOURCE_DATE_EPOCH:?not set}
    JOBS: ${JOBS:?not set}
    DISTSRC: ${DISTSRC:?not set}
    OUTDIR: ${OUTDIR:?not set}
EOF

export ACTUAL_OUTDIR="${OUTDIR}"
export OUTDIR="${DISTSRC}/output"
export INSTALLPATH="${DISTSRC}/installed/${DISTNAME}"

#####################
# Environment Setup #
#####################

# The depends folder also serves as a base-prefix for depends packages for
# $HOSTs after successfully building.
export BASEPREFIX="${PWD}/depends"

# Given a package name and an output name, return the path of that output in our
# current guix environment
store_path() {
    grep --extended-regexp "/[^-]{32}-${1}-[^-]+${2:+-${2}}" "${GUIX_ENVIRONMENT}/manifest" \
        | head --lines=1 \
        | sed --expression='s|\x29*$||' \
              --expression='s|^[[:space:]]*"||' \
              --expression='s|"[[:space:]]*$||'
}

# Sanity check CROSS_*_PATH directories
check_cross_paths() {
    local p paths
    IFS=':' read -ra paths <<< "$1"
    for p in "${paths[@]}"; do
        if [ -n "$p" ] && [ ! -d "$p" ]; then
            echo "'$p' doesn't exist or isn't a directory... Aborting..." >&2
            return 1
        fi
    done
}

# Given a hostname, determine the correct value for -Wl,--dynamic-linker.
glibc_dynamic_linker() {
    case "$1" in
        x86_64-linux-gnu)      echo /lib64/ld-linux-x86-64.so.2 ;;
        arm-linux-gnueabihf)   echo /lib/ld-linux-armhf.so.3 ;;
        aarch64-linux-gnu)     echo /lib/ld-linux-aarch64.so.1 ;;
        riscv64-linux-gnu)     echo /lib/ld-linux-riscv64-lp64d.so.1 ;;
        powerpc64-linux-gnu)   echo /lib64/ld64.so.1 ;;
        powerpc64le-linux-gnu) echo /lib64/ld64.so.2 ;;
        *)                     exit 1 ;;
    esac
}

gcc_toolchain() {
    # Set environment variables to point the NATIVE toolchain to the right
    # includes/libs
    local NATIVE_GCC NATIVE_GCC_STATIC CROSS_GLIBC CROSS_GLIBC_STATIC CROSS_KERNEL CROSS_GCC CROSS_GCC_LIB_STORE CROSS_GCC_LIBS CROSS_GCC_LIB

    NATIVE_GCC="$(store_path gcc-toolchain)"

    # Set native toolchain
    export build_CC="${NATIVE_GCC}/bin/gcc -isystem ${NATIVE_GCC}/include"
    export build_CXX="${NATIVE_GCC}/bin/g++ -isystem ${NATIVE_GCC}/include/c++ -isystem ${NATIVE_GCC}/include"

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
}

llvm_toolchain() {
    local CLANG_TOOLCHAIN LIB_CXX

    CLANG_TOOLCHAIN="$(store_path clang-toolchain)"
    LIB_CXX="$(store_path libcxx)"

    export build_CC="${CLANG_TOOLCHAIN}/bin/clang -isystem ${CLANG_TOOLCHAIN}/include"
    export build_CXX="${CLANG_TOOLCHAIN}/bin/clang++ -stdlib=libc++ -isystem ${LIB_CXX}/include/c++/v1 -isystem ${CLANG_TOOLCHAIN}/include"
    export build_LDFLAGS="-fuse-ld=lld -rtlib=compiler-rt -unwindlib=libunwind -L${LIB_CXX}/lib -Wl,-rpath,${LIB_CXX}/lib"
    export build_AR="${CLANG_TOOLCHAIN}/bin/llvm-ar"
    export build_RANLIB="${CLANG_TOOLCHAIN}/bin/llvm-ranlib"
    export build_OBJDUMP="${CLANG_TOOLCHAIN}/bin/llvm-objdump"
    export build_NM="${CLANG_TOOLCHAIN}/bin/llvm-nm"
    export build_STRIP="${CLANG_TOOLCHAIN}/bin/llvm-strip"
}

mingw_w64_toolchain() {
    # Set environment variables to point the NATIVE toolchain to the right
    # includes/libs
    local NATIVE_GCC CROSS_GLIBC CROSS_GCC CROSS_GCC_LIB_STORE CROSS_GCC_LIBS CROSS_GCC_LIB

    NATIVE_GCC="$(store_path gcc-toolchain)"

    # Set native toolchain
    export build_CC="${NATIVE_GCC}/bin/gcc -isystem ${NATIVE_GCC}/include"
    export build_CXX="${NATIVE_GCC}/bin/g++ -isystem ${NATIVE_GCC}/include/c++ -isystem ${NATIVE_GCC}/include"

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
}

# Disable Guix ld auto-rpath behavior
export GUIX_LD_WRAPPER_DISABLE_RPATH=yes

# Make /usr/bin if it doesn't exist
[ -e /usr/bin ] || mkdir -p /usr/bin

# Symlink env to a conventional path
[ -e /usr/bin/env ]  || ln -s --no-dereference "$(command -v env)"  /usr/bin/env

###########################
# Source Tarball Building #
###########################

GIT_ARCHIVE="${DIST_ARCHIVE_BASE}/${DISTNAME}.tar.gz"

# Create the source tarball if not already there
if [ ! -e "$GIT_ARCHIVE" ]; then
    mkdir -p "$(dirname "$GIT_ARCHIVE")"
    git archive --prefix="${DISTNAME}/" --output="$GIT_ARCHIVE" HEAD
fi

mkdir -p "$OUTDIR"

unset LIBRARY_PATH
unset CPATH
unset C_INCLUDE_PATH
unset CPLUS_INCLUDE_PATH
unset OBJC_INCLUDE_PATH
unset OBJCPLUS_INCLUDE_PATH
