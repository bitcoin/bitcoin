#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
export APT_LLVM_V="21"
LIBCXX_DIR="/cxx_build/"
export MSAN_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O1 -fno-optimize-sibling-calls"
LIBCXX_FLAGS="-nostdinc++ -nostdlib++ -isystem ${LIBCXX_DIR}include/c++/v1 -L${LIBCXX_DIR}lib -Wl,-rpath,${LIBCXX_DIR}lib -lc++ -lc++abi -lpthread -Wno-unused-command-line-argument"
export MSAN_AND_LIBCXX_FLAGS="${MSAN_FLAGS} ${LIBCXX_FLAGS}"

export CONTAINER_NAME="ci_native_msan"
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} llvm-${APT_LLVM_V}-dev libclang-${APT_LLVM_V}-dev libclang-rt-${APT_LLVM_V}-dev python3-pip"
export PIP_PACKAGES="--break-system-packages pycapnp"
export DEP_OPTS="DEBUG=1 NO_QT=1 CC=clang CXX=clang++ CFLAGS='${MSAN_FLAGS}' CXXFLAGS='${MSAN_AND_LIBCXX_FLAGS}'"
export GOAL="install"
export CI_LIMIT_STACK_SIZE=1
# Setting CMAKE_{C,CXX}_FLAGS_DEBUG flags to an empty string ensures that the flags set in MSAN_FLAGS remain unaltered.
# _FORTIFY_SOURCE is not compatible with MSAN.
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_C_FLAGS_DEBUG='' \
 -DCMAKE_CXX_FLAGS_DEBUG='' \
 -DSANITIZERS=memory \
 -DAPPEND_CPPFLAGS='-U_FORTIFY_SOURCE' \
"
export USE_INSTRUMENTED_LIBCPP="MemoryWithOrigins"
