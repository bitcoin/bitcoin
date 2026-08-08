#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-linux-gnu
export DPKG_ADD_ARCH="i386"
export CONTAINER_NAME=ci_i686_no_multiprocess
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"
export CI_IMAGE_PLATFORM="linux/amd64"
export CI_CONTAINER_CAP="--security-opt seccomp=unconfined"
export PACKAGES="g++-i686-linux-gnu binutils-i686-linux-gnu libstdc++6:i386 libatomic1:i386"
export DEP_OPTS="DEBUG=1 NO_IPC=1"
export GOAL="install"
export CI_LIMIT_STACK_SIZE=1
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DENABLE_IPC=OFF \
 -DCMAKE_BUILD_TYPE=Debug \
 -DAPPEND_CPPFLAGS='-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE' \
"
