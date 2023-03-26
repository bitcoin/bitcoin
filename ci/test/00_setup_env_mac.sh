#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin19
export PACKAGES="cmake imagemagick libcap-dev librsvg2-bin libz-dev libbz2-dev libtiff-tools python3-dev python3-setuptools"
export XCODE_VERSION=12.1
export XCODE_BUILD_ID=12A7403
export RUN_UNIT_TESTS=false
export RUN_INTEGRATION_TESTS=false
export GOAL="all deploy"
export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --disable-miner --enable-werror"
