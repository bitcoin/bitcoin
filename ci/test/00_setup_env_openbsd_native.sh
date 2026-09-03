#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_openbsd_native
export CI_OS_NAME=openbsd
export BSD_RELEASE=7.9
export PACKAGES="bash bison ccache cmake-core curl gmake gtar-- ninja py3-pip py3-zmq rsync--"
export PIP_PACKAGES="--break-system-packages pycapnp"
export MAKE=gmake
export GOAL=install
export DEP_OPTS="NO_QT=1"
export CI_LIMIT_NOFILE=1024
export TEST_RUNNER_EXTRA="--exclude feature_reindex_init --exclude p2p_private_broadcast_retry_v1"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DREDUCE_EXPORTS=ON \
 -DWITH_USDT=OFF \
"
