#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export PACKAGES="python3-zmq qtbase5-dev qttools5-dev-tools libdbus-1-dev libharfbuzz-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
# TODO: we have few rpcs that aren't covered by any test, re-enable the line below once it's fixed
# export TEST_RUNNER_EXTRA="--coverage --extended --exclude feature_pruning,feature_dbcrash"  # Run extended tests so that coverage does not fail, but exclude the very slow dbcrash
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++"
