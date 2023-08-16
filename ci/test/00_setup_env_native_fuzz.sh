#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export DOCKER_NAME_TAG="ubuntu:20.04"
export CONTAINER_NAME=ci_native_fuzz
export PACKAGES="clang llvm python3 libevent-dev bsdmainutils libboost-filesystem-dev libboost-test-dev libboost-thread-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
export CPPFLAGS="-DDEBUG_LOCKORDER -DENABLE_DASH_DEBUG -DARENA_DEBUG"
export CXXFLAGS="-Werror -Wno-unused-command-line-argument -Wno-unused-value -Wno-deprecated-builtins"
export PYZMQ=true
export RUN_UNIT_TESTS=false
export RUN_INTEGRATION_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --disable-ccache --enable-fuzz --with-sanitizers=fuzzer,address,undefined --enable-c++17  --enable-suppress-external-warnings CC=clang-15 CXX=clang++-15"
