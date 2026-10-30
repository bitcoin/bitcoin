#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-linux-gnu
export DPKG_ADD_ARCH="i386"
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"
export CI_IMAGE_PLATFORM="linux/amd64"
export CI_CONTAINER_CAP="--security-opt seccomp=unconfined"
export PACKAGES="g++-i686-linux-gnu binutils-i686-linux-gnu libstdc++6:i386 libatomic1:i386 python3-pip"
export PIP_PACKAGES="--break-system-packages pycapnp"
export DEP_OPTS="DEBUG=1"
export CI_LIMIT_STACK_SIZE=1
printf -v BITCOIN_CONFIG "%q " \
 --preset=dev-mode \
 -DCMAKE_BUILD_TYPE=Debug \
 -DAPPEND_CPPFLAGS=-DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE
export BITCOIN_CONFIG
export BITCOIN_CMD="bitcoin -m" # Used in functional tests
