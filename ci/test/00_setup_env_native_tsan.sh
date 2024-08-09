#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export PACKAGES="clang-18 llvm-18 libclang-rt-18-dev libc++abi-18-dev libc++-18-dev python3-zmq"
export DEP_OPTS="CC=clang-18 CXX='clang++-18 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="-DWITH_ZMQ=ON -DSANITIZERS=thread \
-DAPPEND_CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER -DDEBUG_LOCKCONTENTION -D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES'"
