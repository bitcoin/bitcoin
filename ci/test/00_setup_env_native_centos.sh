#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export DOCKER_NAME_TAG=centos:7
export DOCKER_PACKAGES="gcc-c++ libtool make git python3 python36-zmq"
export PACKAGES="boost-devel libevent-devel libdb4-devel libdb4-cxx-devel miniupnpc-devel zeromq-devel qt5-qtbase-devel qt5-qttools-devel qrencode-devel"
export NO_DEPENDS=1
export GOAL="install"
export BITCOIN_CONFIG="--enable-reduce-exports"
