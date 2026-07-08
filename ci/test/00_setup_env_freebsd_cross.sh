#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_freebsd_cross
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"
export APT_LLVM_V="22"
export HOST=x86_64-unknown-freebsd
export FREEBSD_VERSION=15.1
export FREEBSD_SDK_BASENAME="freebsd-${HOST}-${FREEBSD_VERSION}"
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} lld-${APT_LLVM_V}"
export SYSROOT="--sysroot=${DEPENDS_DIR}/SDKs/${FREEBSD_SDK_BASENAME}"
export DEP_OPTS="build_CC=clang build_CXX=clang++ \
 CC='clang --target=${HOST} ${SYSROOT}' \
 CXX='clang++ --target=${HOST} ${SYSROOT} -stdlib=libc++' \
 LDFLAGS='-Wc,-fuse-ld=lld -fuse-ld=lld' \
 AR=llvm-ar-${APT_LLVM_V} \
 NM=llvm-nm-${APT_LLVM_V} \
 OBJCOPY=llvm-objcopy-${APT_LLVM_V} \
 OBJDUMP=llvm-objdump-${APT_LLVM_V} \
 RANLIB=llvm-ranlib-${APT_LLVM_V} \
 STRIP=llvm-strip-${APT_LLVM_V}"
export GOAL="install"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DCMAKE_LINKER_TYPE=LLD \
 -DREDUCE_EXPORTS=ON \
 -DWITH_USDT=OFF \
"
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
