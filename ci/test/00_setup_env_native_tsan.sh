#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export PACKAGES="clang-8 llvm-8 python3-zmq qtbase5-dev qttools5-dev-tools libevent-dev bsdmainutils libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev libqrencode-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
export TEST_RUNNER_EXTRA="--extended --exclude feature_pruning,feature_dbcrash,wallet_multiwallet.py" # Temporarily suppress ASan heap-use-after-free (see issue #14163)
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --enable-reduce-exports --enable-crash-hooks --enable-suppress-external-warnings --with-sanitizers=thread"
export BITCOIN_CONFIG="${BITCOIN_CONFIG} CC=clang-15 CXX=clang++-15 CXXFLAGS=-Werror=thread-safety"
export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG -DARENA_DEBUG"
export PYZMQ=true
export RUN_SYMBOL_TESTS=false
