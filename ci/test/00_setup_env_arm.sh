#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=arm-linux-gnueabihf
export DPKG_ADD_ARCH="armhf"
export PACKAGES="python3-zmq g++-arm-linux-gnueabihf busybox libc6:armhf libstdc++6:armhf libfontconfig1:armhf libxcb1:armhf"
export CONTAINER_NAME=ci_arm_linux
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"  # Check that https://packages.ubuntu.com/noble/g++-arm-linux-gnueabihf (version 13.x, similar to guix) can cross-compile
export CI_IMAGE_PLATFORM="linux/arm64"
export USE_BUSY_BOX=true
export RUN_UNIT_TESTS=true
export RUN_FUNCTIONAL_TESTS=false
export GOAL="install"
# -Wno-psabi is to disable ABI warnings: "note: parameter passing for argument of type ... changed in GCC 7.1"
# This could be removed once the ABI change warning does not show up by default
export BITCOIN_CONFIG="-DREDUCE_EXPORTS=ON -DCMAKE_CXX_FLAGS='-Wno-psabi -Wno-error=maybe-uninitialized'"
