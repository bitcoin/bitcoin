#!/usr/bin/env bash
#
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export CONTAINER_NAME=ci_native_tidy
export TIDY_LLVM_V="18"
export PACKAGES="clang-${TIDY_LLVM_V} libclang-${TIDY_LLVM_V}-dev llvm-${TIDY_LLVM_V}-dev libomp-${TIDY_LLVM_V}-dev clang-tidy-${TIDY_LLVM_V} jq libevent-dev libboost-dev libminiupnpc-dev libzmq3-dev systemtap-sdt-dev qtbase5-dev qttools5-dev qttools5-dev-tools libqrencode-dev libsqlite3-dev libdb++-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
export RUN_CHECK_DEPS=true
export RUN_TIDY=true
export GOAL="install"
export BITCOIN_CONFIG="\
 -DWITH_ZMQ=ON -DBUILD_GUI=ON -DBUILD_BENCH=ON -DWITH_MINIUPNPC=ON -DWITH_USDT=ON -DWITH_BDB=ON -DWARN_INCOMPATIBLE_BDB=OFF \
 -DENABLE_HARDENING=OFF \
 -DCMAKE_C_COMPILER=clang-${TIDY_LLVM_V} \
 -DCMAKE_CXX_COMPILER=clang++-${TIDY_LLVM_V} \
 -DCMAKE_C_FLAGS_RELWITHDEBINFO='-O0 -g0' \
 -DCMAKE_CXX_FLAGS_RELWITHDEBINFO='-O0 -g0' \
"
