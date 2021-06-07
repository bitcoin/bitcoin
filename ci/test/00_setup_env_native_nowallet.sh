#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet
export DOCKER_NAME_TAG=ubuntu:20.04  # Use focal to have one config run the tests in python3.6, see doc/dependencies.md
export PACKAGES="python3-zmq clang llvm"
export DEP_OPTS="NO_WALLET=1"
export GOAL="install"
export SYSCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports CC=clang CXX=clang++ --enable-external-signer"
export CCACHE_SIZE=250M
# Relic documentation isn't right
export NO_WERROR=1
