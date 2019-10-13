#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin14
export BREW_PACKAGES="automake berkeley-db4 libtool boost miniupnpc openssl pkg-config protobuf qt qrencode librsvg pyenv"
export OSX_SDK=10.11
export RUN_UNIT_TESTS=true
export RUN_FUNCTIONAL_TESTS=true
export GOAL="deploy"
export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --enable-werror"
