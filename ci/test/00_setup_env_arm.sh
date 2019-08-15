#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=arm-linux-gnueabihf
export PACKAGES="python3 g++-arm-linux-gnueabihf"
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="install"
# -Wno-psabi is to disable ABI warnings: "note: parameter passing for argument of type ... changed in GCC 7.1"
# This could be removed once the ABI change warning does not show up by default
export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports CXXFLAGS=-Wno-psabi"
