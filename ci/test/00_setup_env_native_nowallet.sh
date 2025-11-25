#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet
export HOST=x86_64-pc-linux-gnu
export PACKAGES="python3-zmq"
export DEP_OPTS="NO_WALLET=1 CC=gcc-14 CXX=g++-14"
export GOAL="install"
export BITCOIN_CONFIG="--enable-reduce-exports CC=gcc-14 CXX=g++-14"
