#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:23.10"  # This version will reach EOL in Jul 2024, and can be replaced by "ubuntu:24.04" (or anything else that ships the wanted clang version).
export PACKAGES="clang-17 llvm-17 libclang-rt-17-dev libc++abi-17-dev libc++-17-dev python3-zmq"
export DEP_OPTS="CC=clang-17 CXX='clang++-17 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER -DDEBUG_LOCKCONTENTION' CXXFLAGS='-g' --with-sanitizers=thread"
