#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=s390x-linux-gnu
# The host arch is unknown, so we run the tests through qemu.
# If the host is s390x and wants to run the tests natively, it can set QEMU_USER_CMD to the empty string.
if [ -z ${QEMU_USER_CMD+x} ]; then export QEMU_USER_CMD="${QEMU_USER_CMD:-"qemu-s390x"}"; fi
export PACKAGES="python3-zmq"
if [ -n "$QEMU_USER_CMD" ]; then
  # Likely cross-compiling, so install the needed gcc and qemu-user
  export DPKG_ADD_ARCH="s390x"
  export PACKAGES="$PACKAGES g++-s390x-linux-gnu qemu-user libc6:s390x libstdc++6:s390x libfontconfig1:s390x libxcb1:s390x"
fi
# Use debian to avoid 404 apt errors
export CONTAINER_NAME=ci_s390x
export DOCKER_NAME_TAG="debian:buster"
export RUN_UNIT_TESTS=true
export TEST_RUNNER_ENV="LC_ALL=C"
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
export SYSCOIN_CONFIG="--enable-reduce-exports --with-incompatible-bdb"
