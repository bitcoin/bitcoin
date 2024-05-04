#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-pc-linux-gnu
export CONTAINER_NAME=ci_i686_multiprocess
export CI_IMAGE_NAME_TAG="docker.io/amd64/ubuntu:24.04"
export PACKAGES="llvm clang g++-multilib"
export DEP_OPTS="DEBUG=1 MULTIPROCESS=1"
export GOAL="install"
export TEST_RUNNER_EXTRA="--v2transport"
export BITCOIN_CONFIG="--enable-debug CC='clang -m32' CXX='clang++ -m32' \
CPPFLAGS='-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE' CXXFLAGS='-Wno-error=documentation'"
export BITCOIND=bitcoin-node  # Used in functional tests
