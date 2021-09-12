#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export DOCKER_NAME_TAG="ubuntu:20.04"
export CONTAINER_NAME=ci_native_fuzz
export PACKAGES="clang llvm python3 libevent-dev bsdmainutils libboost-dev libboost-system-dev libboost-filesystem-dev libboost-test-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer,address,undefined,integer CC='clang -ftrivial-auto-var-init=pattern' CXX='clang++ -ftrivial-auto-var-init=pattern'"
export CCACHE_SIZE=200M
