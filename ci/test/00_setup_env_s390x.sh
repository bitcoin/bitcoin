#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=s390x-linux-gnu
export PACKAGES="python3-zmq"
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"
export CI_IMAGE_PLATFORM="linux/s390x"
# bind tests excluded for now: under qemu-user, s390x userspace reads socket
# information produced by the x86 host kernel with a different byte order.
export TEST_RUNNER_EXTRA="--exclude rpc_bind --exclude feature_bind_extra"
export RUN_FUNCTIONAL_TESTS=true
printf -v BITCOIN_CONFIG "%q " \
  --preset=dev-mode \
  -DREDUCE_EXPORTS=ON
export BITCOIN_CONFIG
