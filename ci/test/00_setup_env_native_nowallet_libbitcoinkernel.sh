#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet_libbitcoinkernel
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:20.04"
# Use minimum supported python3.8 and clang-10, see doc/dependencies.md
export PACKAGES="python3-zmq clang-10 llvm-10 libc++abi-10-dev libc++-10-dev"
export DEP_OPTS="NO_WALLET=1 CC=clang-10 CXX='clang++-10 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="--enable-reduce-exports --enable-experimental-util-chainstate --with-experimental-kernel-lib --enable-shared"
