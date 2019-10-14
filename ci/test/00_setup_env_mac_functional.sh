#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin14
export BREW_PACKAGES="automake berkeley-db4 libtool boost miniupnpc pkg-config protobuf qt qrencode python3 ccache zeromq"
export PIP_PACKAGES="zmq"
export OSX_SDK=10.11
export RUN_CI_ON_HOST=true
export RUN_UNIT_TESTS=true
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-gui --enable-bip70 --enable-reduce-exports --enable-werror"
export NO_DEPENDS=1
