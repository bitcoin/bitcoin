#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_netbsd_native
export CI_OS_NAME=netbsd
export BSD_RELEASE=11.0
export PACKAGES="bash ccache cmake curl gmake lsof ninja-build perl pkgconf py313-pip py313-zmq rsync"
export PIP_PACKAGES="pycapnp"
export PYTHON=python3.13
export MAKE=gmake
export GOAL=install
export TEST_RUNNER_EXTRA="--exclude feature_reindex_init"
export DEP_OPTS="NO_QT=1 CXXFLAGS=-DKJ_NO_EXCEPTIONS=0 build_CXXFLAGS=-DKJ_NO_EXCEPTIONS=0"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DREDUCE_EXPORTS=ON \
 -DWITH_USDT=OFF \
"
