#!/usr/bin/env bash
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=i686-pc-linux-gnu
export CONTAINER_NAME=ci_i686_centos
export CI_IMAGE_NAME_TAG="quay.io/centos/amd64:stream9"
export CI_BASE_PACKAGES="gcc-c++ glibc-devel.x86_64 libstdc++-devel.x86_64 glibc-devel.i686 libstdc++-devel.i686 ccache make git python3 python3-pip which patch lbzip2 xz procps-ng dash rsync coreutils bison util-linux e2fsprogs cmake"
export PIP_PACKAGES="pyzmq"
export GOAL="install"
export NO_WERROR=1  # Suppress error: #warning _FORTIFY_SOURCE > 2 is treated like 2 on this platform [-Werror=cpp]
export BITCOIN_CONFIG="-DWITH_ZMQ=ON -DBUILD_GUI=ON -DREDUCE_EXPORTS=ON"
export CONFIG_SHELL="/bin/dash"
