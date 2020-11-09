#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export HOST=x86_64-w64-mingw32
export PACKAGES="python3 nsis g++-mingw-w64-x86-64 wine-binfmt wine64 file"
export DPKG_ADD_ARCH="i386"
export RUN_FUNCTIONAL_TESTS=false
export RUN_SECURITY_TESTS="false"
export GOAL="deploy"
export BITCOIN_CONFIG="--enable-gui --enable-reduce-exports --disable-miner --without-boost-process"
export DIRECT_WINE_EXEC_TESTS=true
