#!/usr/bin/env bash
#
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_alpine_musl
export CI_IMAGE_NAME_TAG="mirror.gcr.io/alpine:3.22"
export CI_BASE_PACKAGES="build-base musl-dev pkgconf curl ccache make ninja git python3-dev py3-pip which patch xz procps rsync util-linux bison e2fsprogs cmake dash linux-headers"
export PIP_PACKAGES="--break-system-packages pyzmq pycapnp"
export DEP_OPTS="DEBUG=1"
export GOAL="install"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DREDUCE_EXPORTS=ON \
 -DCMAKE_BUILD_TYPE=Debug \
"
export BITCOIN_CMD="bitcoin -m" # Used in functional tests
