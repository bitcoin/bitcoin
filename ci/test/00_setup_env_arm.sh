#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=arm-linux-gnueabihf
# The host arch is unknown, so we run the tests through qemu.
# If the host is arm and wants to run the tests natively, it can set QEMU_USER_CMD to the empty string.
if [ -z ${QEMU_USER_CMD+x} ]; then export QEMU_USER_CMD="${QEMU_USER_CMD:-"qemu-arm -L /usr/arm-linux-gnueabihf/"}"; fi
export DPKG_ADD_ARCH="armhf"
export PACKAGES="python3-zmq g++-arm-linux-gnueabihf busybox libc6:armhf libstdc++6:armhf libfontconfig1:armhf libxcb1:armhf"
if [ -n "$QEMU_USER_CMD" ]; then
  # Likely cross-compiling, so install the needed gcc and qemu-user
  export PACKAGES="$PACKAGES qemu-user"
fi
export CONTAINER_NAME=ci_arm_linux
# Use debian to avoid 404 apt errors when cross compiling
export CI_IMAGE_NAME_TAG="debian:bullseye"
export USE_BUSY_BOX=true
export RUN_UNIT_TESTS=false # SYSCOIN TODO: Illegal instruction (core dumped) in bls_tests.cpp
export RUN_FUNCTIONAL_TESTS=false
export GOAL="install"
# -Wno-psabi is to disable ABI warnings: "note: parameter passing for argument of type ... changed in GCC 7.1"
# This could be removed once the ABI change warning does not show up by default
export SYSCOIN_CONFIG="--enable-reduce-exports CXXFLAGS=-Wno-psabi"
