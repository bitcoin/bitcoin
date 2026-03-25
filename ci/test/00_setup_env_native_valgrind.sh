#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="mirror.gcr.io/debian:trixie"
export CONTAINER_NAME=ci_native_valgrind
export PACKAGES="clang llvm libclang-rt-dev valgrind python3-zmq libevent-dev libboost-dev libzmq3-dev libsqlite3-dev libcapnp-dev capnproto python3-pip"
export PIP_PACKAGES="--break-system-packages pycapnp"
export USE_VALGRIND=1
export NO_DEPENDS=1
# bind tests excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
export TEST_RUNNER_EXTRA="--exclude rpc_bind --exclude feature_bind_extra"
export GOAL="install"
# GUI disabled, because it only passes with a DEBUG=1 depends build
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DWITH_USDT=OFF \
 -DCMAKE_C_COMPILER=clang \
 -DCMAKE_CXX_COMPILER=clang++ \
"
