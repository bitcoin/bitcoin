#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_fuzz
export PACKAGES="clang llvm python3 libevent-dev bsdmainutils libboost-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
export CPPFLAGS="-DDEBUG_LOCKORDER -DARENA_DEBUG"
export PYZMQ=true
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --enable-fuzz --with-sanitizers=fuzzer,address,undefined,integer CC='clang-19 -ftrivial-auto-var-init=pattern' CXX='clang++-19 -ftrivial-auto-var-init=pattern'"
