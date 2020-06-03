#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export PACKAGES="clang llvm libc++abi-dev libc++-dev python3-zmq"
export DEP_OPTS="CC=clang-15 CXX='clang++-15 -stdlib=libc++"
export TEST_RUNNER_EXTRA="--extended --exclude feature_pruning,feature_dbcrash,wallet_multiwallet.py" # Temporarily suppress ASan heap-use-after-free (see issue #14163)
export TEST_RUNNER_EXTRA="${TEST_RUNNER_EXTRA} --timeout-factor=4"  # Increase timeout because sanitizers slow down
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --disable-wallet --with-gui=no --with-sanitizers=thread CC=clang-15 CXX=clang++-15"
export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG -DARENA_DEBUG"
export PYZMQ=true
export RUN_SYMBOL_TESTS=false
