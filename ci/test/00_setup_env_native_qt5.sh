#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_qt5
export PACKAGES="python3-zmq qtbase5-dev qttools5-dev-tools libdbus-1-dev libharfbuzz-dev"
export DEP_OPTS="NO_UPNP=1 DEBUG=1"
# TODO: we have few rpcs that aren't covered by any test, re-enable the line below once it's fixed
# export TEST_RUNNER_EXTRA="--previous-releases --coverage --extended --exclude feature_pruning,feature_dbcrash"  # Run extended tests so that coverage does not fail, but exclude the very slow dbcrash
export RUN_UNIT_TESTS_SEQUENTIAL="true"
export RUN_UNIT_TESTS="false"
export GOAL="install"
export PREVIOUS_RELEASES_TO_DOWNLOAD="v0.15.0.0 v0.16.1.1 v0.17.0.3 v18.2.2 v19.3.0 v20.0.1"
export BITCOIN_CONFIG="--enable-zmq --with-libs=no --enable-reduce-exports --disable-fuzz-binary LDFLAGS=-static-libstdc++ --with-boost-process"
