#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:noble"  # Check that g++-mingw-w64-x86-64-posix (version 13.2, similar to guix) can cross-compile
export CI_IMAGE_PLATFORM="linux/amd64"
export HOST=x86_64-w64-mingw32
export DPKG_ADD_ARCH="i386"
export PACKAGES="nsis g++-mingw-w64-x86-64-posix wine-binfmt wine64 wine32 file"
# Install wine, but do not run unit tests, as they surface frequent
# false-positives.
export RUN_UNIT_TESTS=${RUN_UNIT_TESTS:-false}
export RUN_FUNCTIONAL_TESTS=false
export GOAL="deploy"
export BITCOIN_CONFIG="-DREDUCE_EXPORTS=ON -DBUILD_GUI_TESTS=OFF \
-DCMAKE_CXX_FLAGS='-Wno-error=maybe-uninitialized -Wno-error=array-bounds'"
