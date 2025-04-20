#!/usr/bin/env bash
#
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_multiprocess
export HOST=x86_64-pc-linux-gnu
export PACKAGES="cmake python3 llvm clang"
export DEP_OPTS="MULTIPROCESS=1 CC=clang-18 CXX=clang++-18"
export RUN_TIDY=true
export GOAL="install"
export TEST_RUNNER_EXTRA="--v2transport"
export BITCOIN_CONFIG="--with-boost-process --enable-debug CC=clang-18 CXX=clang++-18" # Use clang to avoid OOM
# Additional flags for RUN_TIDY
export BITCOIN_CONFIG="${BITCOIN_CONFIG} --disable-hardening CFLAGS='-O0 -g0' CXXFLAGS='-O0 -g0 -Wno-error=documentation'"
export BITCOIND=dash-node  # Used in functional tests
