#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet_libbitcoinkernel
export CI_IMAGE_NAME_TAG=ubuntu:focal
# Use minimum supported python3.7 (or python3.8, as best-effort) and clang-8, see doc/dependencies.md
export PACKAGES="python3-zmq clang-8 llvm-8 libc++abi-8-dev libc++-8-dev"
export DEP_OPTS="NO_WALLET=1 CC=clang-8 CXX='clang++-8 -stdlib=libc++'"
export GOAL="install"
export NO_WERROR=1
export BITCOIN_CONFIG="--enable-reduce-exports CC=clang-8 CXX='clang++-8 -stdlib=libc++' --enable-experimental-util-chainstate --with-experimental-kernel-lib --enable-shared"
