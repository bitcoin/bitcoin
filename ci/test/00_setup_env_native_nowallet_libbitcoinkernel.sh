#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet_libbitcoinkernel
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:22.04"
# Use minimum supported python3.9 (or best-effort 3.10) and clang-15, see doc/dependencies.md
export PACKAGES="python3-zmq clang-15 llvm-15 libc++abi-15-dev libc++-15-dev"
export DEP_OPTS="NO_WALLET=1 CC=clang-15 CXX='clang++-15 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="-DREDUCE_EXPORTS=ON -DBUILD_UTIL_CHAINSTATE=ON -DBUILD_KERNEL_LIB=ON -DBUILD_SHARED_LIBS=ON"
