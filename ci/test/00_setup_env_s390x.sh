#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=s390x-linux-gnu
export PACKAGES="python3-zmq"
export CONTAINER_NAME=ci_s390x
export CI_IMAGE_NAME_TAG="mirror.gcr.io/debian:trixie"
export CI_IMAGE_PLATFORM="linux/s390x"
# bind tests excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
export TEST_RUNNER_EXTRA="--exclude rpc_bind --exclude feature_bind_extra"
export RUN_FUNCTIONAL_TESTS=true
export GOAL="install"
export BITCOIN_CONFIG="\
  --preset=dev-mode \
  -DREDUCE_EXPORTS=ON \
"
