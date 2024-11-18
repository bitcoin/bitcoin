#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export PACKAGES="clang-18 llvm-18 libclang-rt-18-dev libc++abi-18-dev libc++-18-dev python3-zmq"
export DEP_OPTS="CC=clang-18 CXX='clang++-18 -stdlib=libc++'"
export TEST_RUNNER_EXTRA="--extended --exclude feature_pruning,feature_dbcrash,wallet_multiwallet.py" # Temporarily suppress ASan heap-use-after-free (see issue #14163)
export TEST_RUNNER_EXTRA="${TEST_RUNNER_EXTRA} --timeout-factor=4"  # Increase timeout because sanitizers slow down
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-gui=no --with-sanitizers=thread CC=clang-18 CXX=clang++-18 CXXFLAGS='-g' --with-boost-process"
export CPPFLAGS="-DARENA_DEBUG -DDEBUG_LOCKORDER -DDEBUG_LOCKCONTENTION"
export PYZMQ=true
