#!/usr/bin/env bash
#
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="ubuntu:22.04"
export CONTAINER_NAME=ci_native_tidy
export PACKAGES="clang libclang-dev llvm-dev clang-tidy bear cmake libevent-dev libboost-dev libminiupnpc-dev libnatpmp-dev libzmq3-dev systemtap-sdt-dev libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libqrencode-dev libsqlite3-dev libdb++-dev libgmp-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
export RUN_TIDY=true
export GOAL="install"
export SYSCOIN_CONFIG="CC=clang CXX=clang++ --with-incompatible-bdb --disable-hardening CFLAGS='-O0 -g0' CXXFLAGS='-O0 -g0'"
export CCACHE_SIZE=200M
