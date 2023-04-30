#!/usr/bin/env bash
#
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_coverage
export CI_IMAGE_NAME_TAG="ubuntu:23.04"
export PACKAGES="gcc g++ git python3-zmq libevent-dev libboost-dev libdb5.3++-dev libsqlite3-dev libminiupnpc-dev libzmq3-dev lcov build-essential libtool autotools-dev automake pkg-config bsdmainutils bsdextrautils libxml2-dev python3-pip python3-dev ccache"
export DEP_OPTS="NO_QT=1 NO_NATPMP=1 NO_UPNP=1 NO_ZMQ=1 NO_USDT=1"
export RUN_UNIT_TESTS="false"
export RUN_FUNCTIONAL_TESTS="false"
export RUN_COVERAGE="true"
export UPLOAD_COVERAGE="true"
export TEST_RUNNER_ARGS="--timeout-factor=10 --exclude=feature_dbcrash $MAKEJOBS"
export GOAL="install"
export NO_WERROR=1
export DOWNLOAD_PREVIOUS_RELEASES="true"
export BDB_PREFIX="/tmp/cirrus-ci-build/depends/x86_64-pc-linux-gnu"
export BITCOIN_CONFIG="--disable-fuzz --enable-fuzz-binary=no --with-gui=no --disable-bench BDB_LIBS=\"-L${BDB_PREFIX}/lib -ldb_cxx-4.8\" BDB_CFLAGS=\"-I${BDB_PREFIX}/include\" --enable-lcov"
