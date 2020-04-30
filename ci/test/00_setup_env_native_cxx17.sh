#!/usr/bin/env bash
#
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_cxx17
export PACKAGES="python3-zmq libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev libminiupnpc-dev libzmq3-dev"
export NO_DEPENDS=1
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-incompatible-bdb --enable-c++17"
