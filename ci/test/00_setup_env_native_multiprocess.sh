#!/usr/bin/env bash
#
# Copyright (c) 2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_multiprocess
export DOCKER_NAME_TAG=ubuntu:20.04
export PACKAGES="cmake python3"
export DEP_OPTS="MULTIPROCESS=1"
export GOAL="install"
export WIDECOIN_CONFIG="--with-boost-process"
export TEST_RUNNER_ENV="WIDECOIND=widecoin-node"
