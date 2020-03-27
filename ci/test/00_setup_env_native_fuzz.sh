#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_fuzz
export PACKAGES="clang-8 llvm-8 python3 libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer,address,undefined CC=clang-8 CXX=clang++-8"
# Use clang-8, instead of default clang on bionic, which is clang-6 and does not come with libfuzzer on aarch64
