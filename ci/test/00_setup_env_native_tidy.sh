#!/usr/bin/env bash
#
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export DOCKER_NAME_TAG="ubuntu:22.04"
export CONTAINER_NAME=ci_native_tidy
export PACKAGES="clang llvm clang-tidy bear libevent-dev libboost-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
export RUN_TIDY=true
export GOAL="install"
export BITCOIN_CONFIG="CC=clang CXX=clang++ --disable-hardening CFLAGS='-O0 -g0' CXXFLAGS='-O0 -g0'"
export CCACHE_SIZE=200M
