#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=arm64-apple-darwin
export PIP_PACKAGES="zmq"
export GOAL="install"
export BITCOIN_CONFIG="-DWITH_GUI=Qt5 -DWITH_MINIUPNPC=ON -DWITH_NATPMP=ON -DREDUCE_EXPORTS=ON"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export CCACHE_SIZE=300M
