#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=arm64-apple-darwin
export PIP_PACKAGES="zmq"
export GOAL="install"
export SYSCOIN_CONFIG="--with-gui --with-miniupnpc --with-natpmp --enable-reduce-exports"
export TEST_RUNNER_EXTRA="--exclude interface_zmq_nevm"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export CCACHE_SIZE=300M
export CPATH="/opt/homebrew/include"
export LDFLAGS="-L/opt/homebrew/lib -L/opt/homebrew/opt $LDFLAGS"

