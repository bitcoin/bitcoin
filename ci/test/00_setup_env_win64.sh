#!/usr/bin/env bash
#
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export CI_IMAGE_NAME_TAG="mirror.gcr.io/debian:trixie"  # Check that https://packages.debian.org/trixie/g++-mingw-w64-ucrt64 can cross-compile
export HOST=x86_64-w64-mingw32ucrt
export PACKAGES="g++-mingw-w64-ucrt64 nsis"
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="deploy"
export BITCOIN_CONFIG="\
  --preset=dev-mode \
  -DENABLE_IPC=OFF \
  -DWITH_USDT=OFF \
  -DREDUCE_EXPORTS=ON \
  -DCMAKE_CXX_FLAGS='-Wno-error=maybe-uninitialized' \
"
