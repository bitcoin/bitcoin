#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-pc-linux-gnu
export CONTAINER_NAME=ci_i686_multiprocess
export CI_IMAGE_NAME_TAG=ubuntu:20.04
export PACKAGES="cmake python3 llvm clang g++-multilib"
export DEP_OPTS="DEBUG=1 MULTIPROCESS=1"
export GOAL="install"
export SYSCOIN_CONFIG="--enable-debug CC='clang -m32' CXX='clang++ -m32' LDFLAGS='--rtlib=compiler-rt -lgcc_s'"
export TEST_RUNNER_ENV="SYSCOIND=syscoin-node"
export TEST_RUNNER_EXTRA="--nosandbox"
