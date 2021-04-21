#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_omnij
export DOCKER_NAME_TAG=ubuntu:18.04
export PACKAGES="python3-zmq openjdk-11-jdk"
export DEP_OPTS="NO_QT=1 NO_UPNP=1"
export RUN_UNIT_TESTS_SEQUENTIAL="false"
export RUN_UNIT_TESTS="false"
export RUN_OMNIJ_TESTS="true"
export GOAL="install"
export TEST_PREVIOUS_RELEASES=true
export BITCOIN_CONFIG="--with-gui=no"
