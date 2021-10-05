#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export DOCKER_NAME_TAG="ubuntu:20.04"
export CONTAINER_NAME=ci_native_valgrind
export PACKAGES="valgrind clang llvm python3-zmq libevent-dev bsdmainutils libboost-dev libboost-system-dev libboost-filesystem-dev libboost-test-dev libdb5.3++-dev libminiupnpc-dev libnatpmp-dev libzmq3-dev libsqlite3-dev"
export USE_VALGRIND=1
export NO_DEPENDS=1
export TEST_RUNNER_EXTRA="--exclude rpc_bind,feature_bind_extra"  # Excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-incompatible-bdb --with-gui=no CC=clang CXX=clang++"  # TODO enable GUI
