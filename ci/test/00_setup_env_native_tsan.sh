#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export PACKAGES="clang llvm python3-zmq qtbase5-dev qttools5-dev-tools libevent-dev bsdmainutils libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev libqrencode-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
export TEST_RUNNER_EXTRA="--extended --exclude feature_pruning,feature_dbcrash,wallet_multiwallet.py" # Temporarily suppress ASan heap-use-after-free (see issue #14163)
export RUN_BENCH=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --enable-reduce-exports --enable-crash-hooks --with-sanitizers=thread"
export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG -DARENA_DEBUG"
export PYZMQ=true

# xenial comes with old clang versions that can not parse the sanitizer suppressions files
# Remove unparseable lines as a hacky workaround
sed -i '/^implicit-/d' "${BASE_ROOT_DIR}/test/sanitizer_suppressions/ubsan"
