#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_sqlite
export PACKAGES="python3-zmq qtbase5-dev qttools5-dev-tools libdbus-1-dev libharfbuzz-dev"
export DEP_OPTS="NO_BDB=1 NO_UPNP=1 DEBUG=1"
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --enable-reduce-exports --with-sqlite --without-bdb LDFLAGS=-static-libstdc++"
