#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME="ci_mac_native"  # macos does not use a container, but the env var is needed for logging
export PIP_PACKAGES="--break-system-packages pycapnp zmq"
export GOAL="install deploy"
export CMAKE_GENERATOR="Ninja"
export BITCOIN_CONFIG="-DBUILD_GUI=ON -DWITH_ZMQ=ON -DBUILD_KERNEL_LIB=ON -DBUILD_UTIL_CHAINSTATE=ON -DBUILD_KERNEL_TEST=ON -DREDUCE_EXPORTS=ON -DCMAKE_EXE_LINKER_FLAGS='-Wl,-stack_size -Wl,0x80000'"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export BITCOIN_CMD="bitcoin -m" # Used in functional tests
