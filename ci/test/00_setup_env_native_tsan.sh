#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export DOCKER_NAME_TAG=ubuntu:20.04
export PACKAGES="clang llvm libc++abi-dev libc++-dev python3-zmq"
export DEP_OPTS="CC=clang CXX='clang++ -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-gui=no CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER' CXXFLAGS='-g' --with-sanitizers=thread CC=clang CXX='clang++ -stdlib=libc++' --with-boost-process"
