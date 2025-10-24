#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-linux-gnu
export DPKG_ADD_ARCH="i386"
export CONTAINER_NAME=ci_i686_no_multiprocess
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
export CI_IMAGE_PLATFORM="linux/amd64"
export PACKAGES="llvm clang g++-i686-linux-gnu binutils-i686-linux-gnu libstdc++6:i386 libatomic1:i386"
export DEP_OPTS="DEBUG=1 NO_IPC=1"
export GOAL="install"
export CI_LIMIT_STACK_SIZE=1
export TEST_RUNNER_EXTRA="--v2transport --usecli"
export BITCOIN_CONFIG="\
 -DCMAKE_BUILD_TYPE=Debug \
 -DCMAKE_C_COMPILER='clang;-m32' \
 -DCMAKE_CXX_COMPILER='clang++;-m32' \
 -DAPPEND_CPPFLAGS='-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE' \
"
