#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-unknown-linux-gnu
export PACKAGES="python3-zmq qtbase5-dev qttools5-dev-tools libdbus-1-dev libharfbuzz-dev"
export DEP_OPTS="NO_QT=1 NO_UPNP=1 DEBUG=1 ALLOW_HOST_PACKAGES=1"
export TEST_RUNNER_EXTRA="--coverage --extended --exclude feature_dbcrash"  # Run extended tests so that coverage does not fail, but exclude the very slow dbcrash
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-gui=qt5 --enable-glibc-back-compat --enable-reduce-exports --enable-debug CFLAGS=\"-g0 -O2 -funsigned-char\" CXXFLAGS=\"-g0 -O2 -funsigned-char\""
