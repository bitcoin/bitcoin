#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export CONTAINER_NAME=ci_native_fuzz
export PACKAGES="clang-18 llvm-18 libclang-rt-18-dev libevent-dev libboost-dev libsqlite3-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export CI_CONTAINER_CAP="--cap-add SYS_PTRACE"  # If run with (ASan + LSan), the container needs access to ptrace (https://github.com/google/sanitizers/issues/764)
export BITCOIN_CONFIG="\
 -DBUILD_FOR_FUZZING=ON \
 -DSANITIZERS=fuzzer,address,undefined,float-divide-by-zero,integer \
 -DCMAKE_C_COMPILER=clang-18 \
 -DCMAKE_CXX_COMPILER=clang++-18 \
 -DCMAKE_C_FLAGS='-ftrivial-auto-var-init=pattern' \
 -DCMAKE_CXX_FLAGS='-ftrivial-auto-var-init=pattern' \
"
export LLVM_SYMBOLIZER_PATH="/usr/bin/llvm-symbolizer-18"
