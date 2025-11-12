#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_previous_releases
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:22.04"
# Use minimum supported python3.10 and gcc-12, see doc/dependencies.md
export PACKAGES="gcc-12 g++-12 python3-zmq"
export DEP_OPTS="CC=gcc-12 CXX=g++-12"
export TEST_RUNNER_EXTRA="--previous-releases --coverage --extended --exclude feature_dbcrash"  # Run extended tests so that coverage does not fail, but exclude the very slow dbcrash
export GOAL="install"
export CI_LIMIT_STACK_SIZE=1
export DOWNLOAD_PREVIOUS_RELEASES="true"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DREDUCE_EXPORTS=ON \
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_C_FLAGS='-funsigned-char' \
 -DCMAKE_C_FLAGS_DEBUG='-g2 -O2' \
 -DCMAKE_CXX_FLAGS='-funsigned-char' \
 -DCMAKE_CXX_FLAGS_DEBUG='-g2 -O2' \
 -DAPPEND_CPPFLAGS='-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE' \
"
