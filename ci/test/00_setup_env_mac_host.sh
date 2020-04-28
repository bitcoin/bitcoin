#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_macos
export HOST=x86_64-apple-darwin
export PIP_PACKAGES="zmq lief"
export RUN_SECURITY_TESTS="true"
export GOAL="install"
export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --disable-miner --enable-werror"
export TEST_RUNNER_EXTRA="wallet_disable"  # Only run wallet_disable as a smoke test, see https://github.com/bitcoin/bitcoin/pull/17240#issuecomment-546022121 why the other tests are disabled
# Run without depends
export NO_DEPENDS=1
export OSX_SDK=""
