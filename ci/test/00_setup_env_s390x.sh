#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=s390x-linux-gnu
# The host arch is unknown, so we run the tests through qemu.
# If the host is s390x and wants to run the tests natively, it can set QEMU_USER_CMD to the empty string.
if [ -z ${QEMU_USER_CMD+x} ]; then export QEMU_USER_CMD="${QEMU_USER_CMD:-"qemu-s390x"}"; fi
export PACKAGES="python3-zmq bsdmainutils qemu-user"
export RUN_UNIT_TESTS=true
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="--enable-reduce-exports --with-incompatible-bdb"
