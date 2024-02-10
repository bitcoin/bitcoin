#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# TODO: use config `asan` instead `ubsan` for `undefined` sanitizer to unify with bitcoin
export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_ubsan
export PACKAGES="clang-16 llvm-16 python3-zmq qtbase5-dev qttools5-dev-tools libevent-dev bsdmainutils libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev libqrencode-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --enable-reduce-exports --enable-crash-hooks --with-sanitizers=undefined CC=clang-16 CXX=clang++-16"
export PYZMQ=true
export RUN_SYMBOL_TESTS=false
