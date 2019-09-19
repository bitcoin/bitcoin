#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-pc-linux-gnu
export DEP_OPTS="PROTOBUF=1"
export PACKAGES="g++-multilib python3-zmq"
export GOAL="install"
export BITCOIN_CONFIG="--enable-zmq --with-gui=qt5 --enable-bip70 --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++"
export CONFIG_SHELL="/bin/dash"
