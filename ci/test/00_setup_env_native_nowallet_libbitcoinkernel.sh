#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet_libbitcoinkernel
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
# Use minimum supported python3.10 (or best-effort 3.12) and clang-17, see doc/dependencies.md
export PACKAGES="python3-zmq python3-pip clang-17 llvm-17 libc++abi-17-dev libc++-17-dev"
export PIP_PACKAGES="--break-system-packages pycapnp"
export DEP_OPTS="NO_WALLET=1 CC=clang-17 CXX='clang++-17 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="-DREDUCE_EXPORTS=ON -DBUILD_UTIL_CHAINSTATE=ON -DBUILD_KERNEL_LIB=ON -DBUILD_KERNEL_TEST=ON -DBUILD_SHARED_LIBS=ON"
