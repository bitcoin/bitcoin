#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet
export DOCKER_NAME_TAG=ubuntu:16.04  # Use xenial to have one config run the tests in python3.5, see doc/dependencies.md
export PACKAGES="python3-zmq clang-3.8 llvm-3.8"  # Use clang-3.8 to test C++11 compatibility, see doc/dependencies.md
export DEP_OPTS="NO_WALLET=1"
export GOAL="install"
export WIDECOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports CC=clang-3.8 CXX=clang++-3.8 --with-boost-process"
