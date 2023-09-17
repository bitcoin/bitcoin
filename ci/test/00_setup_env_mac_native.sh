#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin
export PIP_PACKAGES="zmq"
export GOAL="install"
export BITCOIN_CONFIG="--with-gui --with-miniupnpc --with-natpmp --enable-reduce-exports"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export CCACHE_MAXSIZE=400M
export RUN_FUZZ_TESTS=true
export FUZZ_TESTS_CONFIG="--exclude banman"  # https://github.com/bitcoin/bitcoin/issues/27924
