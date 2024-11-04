#!/usr/bin/env bash
#
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_multiprocess
export PACKAGES="cmake python3 llvm clang"
export DEP_OPTS="DEBUG=1 MULTIPROCESS=1"
export GOAL="install"
export BITCOIN_CONFIG="--with-boost-process --enable-debug CC=clang CXX=clang++" # Use clang to avoid OOM
export TEST_RUNNER_ENV="BITCOIND=dash-node"