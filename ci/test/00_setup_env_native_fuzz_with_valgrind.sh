#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export CONTAINER_NAME=ci_native_fuzz_valgrind
export PACKAGES="clang-16 llvm-16 libclang-rt-16-dev libevent-dev libboost-dev libsqlite3-dev valgrind"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export FUZZ_TESTS_CONFIG="--valgrind"
export GOAL="install"
export BITCOIN_CONFIG="\
 -DBUILD_FOR_FUZZING=ON \
 -DSANITIZERS=fuzzer \
 -DCMAKE_C_COMPILER=clang-16 \
 -DCMAKE_CXX_COMPILER=clang++-16 \
"
export LLVM_SYMBOLIZER_PATH="/usr/bin/llvm-symbolizer-16"
