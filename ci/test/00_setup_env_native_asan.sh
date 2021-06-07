#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_asan
export PACKAGES="clang llvm python3-zmq qtbase5-dev qttools5-dev-tools libevent-dev bsdmainutils libboost-dev libboost-system-dev libboost-filesystem-dev libboost-test-dev libdb5.3++-dev libminiupnpc-dev libnatpmp-dev libzmq3-dev libqrencode-dev libsqlite3-dev libgmp3-dev libcurl4-gnutls-dev"
export DOCKER_NAME_TAG=ubuntu:hirsute
export NO_DEPENDS=1
export GOAL="install"
export TEST_RUNNER_EXTRA="--exclude feature_deterministicmns,feature_llmq_is_retroactive,feature_llmqdkgerrors,feature_llmqconnections,feature_llmqsigning,feature_llmqsimplepose"
export SYSCOIN_CONFIG="--enable-zmq --with-incompatible-bdb --with-gui=qt5 CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER' --with-sanitizers=address,integer,undefined CC=clang CXX=clang++ --enable-external-signer"