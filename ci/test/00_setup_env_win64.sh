#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export DOCKER_NAME_TAG=ubuntu:18.04  # Check that bionic can cross-compile to win64 (bionic is used in the gitian build as well)
export HOST=x86_64-w64-mingw32
export PACKAGES="python3 nsis g++-mingw-w64-x86-64 wine-binfmt wine64"
export RUN_FUNCTIONAL_TESTS=false
export RUN_SECURITY_TESTS="true"
export GOAL="deploy"
export BITCOIN_CONFIG="--enable-reduce-exports --disable-gui-tests --without-boost-process"
