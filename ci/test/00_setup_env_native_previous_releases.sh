#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_previous_releases
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:22.04"
# Use minimum supported python3.9 (or best effort 3.10) and gcc-11, see doc/dependencies.md
export PACKAGES="gcc-11 g++-11 python3-zmq"
export DEP_OPTS="NO_UPNP=1 NO_NATPMP=1 DEBUG=1 CC=gcc-11 CXX=g++-11"
export TEST_RUNNER_EXTRA="--previous-releases --coverage --extended --exclude feature_dbcrash"  # Run extended tests so that coverage does not fail, but exclude the very slow dbcrash
export RUN_UNIT_TESTS_SEQUENTIAL="true"
export RUN_UNIT_TESTS="false"
export GOAL="install"
export DOWNLOAD_PREVIOUS_RELEASES="true"
export BITCOIN_CONFIG="-DWITH_ZMQ=ON -DBUILD_GUI=ON -DREDUCE_EXPORTS=ON -DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_C_FLAGS='-funsigned-char' -DCMAKE_C_FLAGS_DEBUG='-g0 -O2' \
-DCMAKE_CXX_FLAGS='-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE -funsigned-char' -DCMAKE_CXX_FLAGS_DEBUG='-g0 -O2'"
