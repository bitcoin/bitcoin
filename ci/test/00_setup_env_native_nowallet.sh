#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet
export PACKAGES="python3-zmq"
export DEP_OPTS="NO_WALLET=1"
export GOAL="install"
export BITCOIN_CONFIG="--enable-reduce-exports --with-boost-process"
