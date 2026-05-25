#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_coverage
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
export APT_LLVM_V="22"
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} libclang-rt-${APT_LLVM_V}-dev python3-zmq python3-pip libevent-dev libboost-dev libsqlite3-dev systemtap-sdt-dev libzmq3-dev qt6-base-dev qt6-tools-dev qt6-l10n-tools libqrencode-dev capnproto libcapnp-dev"
export PIP_PACKAGES="--break-system-packages pycapnp"
export NO_DEPENDS=1
export GOAL="install"
export RUN_COVERAGE="true"
export BITCOIN_CONFIG="\
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DAPPEND_CFLAGS='-fprofile-instr-generate -fcoverage-mapping' \
  -DAPPEND_CXXFLAGS='-fprofile-instr-generate -fcoverage-mapping' \
  -DAPPEND_LDFLAGS='-fprofile-instr-generate -fcoverage-mapping' \
"
