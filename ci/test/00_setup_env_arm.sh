#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=arm-linux-gnueabihf
# The host arch is unknown, so we run the tests through qemu.
# If the host is arm and wants to run the tests natively, it can set QEMU_USER_CMD to the empty string.
export QEMU_USER_CMD="${QEMU_USER_CMD:"qemu-arm -L /usr/arm-linux-gnueabihf/"}"
# We don't know whether the host can run the cross compiled binaries. To run them, either qemu-user or libc6:armhf for
# the target is required, so install both.
export DPKG_ADD_ARCH="armhf"
export PACKAGES="python3 g++-arm-linux-gnueabihf busybox qemu-user libc6:armhf libstdc++6:armhf libfontconfig1:armhf libxcb1:armhf"
export USE_BUSY_BOX=true
export RUN_UNIT_TESTS=true
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
# -Wno-psabi is to disable ABI warnings: "note: parameter passing for argument of type ... changed in GCC 7.1"
# This could be removed once the ABI change warning does not show up by default
export BITCOIN_CONFIG="--enable-glibc-back-compat --enable-reduce-exports CXXFLAGS=-Wno-psabi"
