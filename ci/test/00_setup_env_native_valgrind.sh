#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_valgrind
export PACKAGES="valgrind clang llvm python3-zmq libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev"
export USE_VALGRIND=1
export NO_DEPENDS=1
export TEST_RUNNER_EXTRA="p2p_segwit.py"  # Only run one test for now. TODO enable all and bump timeouts
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-incompatible-bdb --with-gui=no CC=clang CXX=clang++"  # TODO enable GUI
