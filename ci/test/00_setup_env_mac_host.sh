#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin18
export PIP_PACKAGES="zmq"
export GOAL="install"
export BITCOIN_CONFIG="--with-gui --enable-reduce-exports --with-boost-process"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export CCACHE_SIZE=300M

export RUN_SECURITY_TESTS="true"
