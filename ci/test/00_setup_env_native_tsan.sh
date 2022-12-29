#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export DOCKER_NAME_TAG=debian:bookworm  # For clang-15
export PACKAGES="clang-15 llvm-15 libc++abi-15-dev libc++-15-dev python3-zmq"
export DEP_OPTS="CC=clang-15 CXX='clang++-15 -stdlib=libc++' NO_QR=1"  # qr disabled due to libqrencode 3.4.4 compile failure, https://github.com/bitcoin/bitcoin/pull/26768#issuecomment-1367403430
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER -DDEBUG_LOCKCONTENTION' CXXFLAGS='-g' --with-sanitizers=thread"
