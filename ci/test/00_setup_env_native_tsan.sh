#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
export APT_LLVM_V="21"
LIBCXX_DIR="/cxx_build/"
LIBCXX_FLAGS="-fsanitize=thread -nostdinc++ -nostdlib++ -isystem ${LIBCXX_DIR}include/c++/v1 -L${LIBCXX_DIR}lib -Wl,-rpath,${LIBCXX_DIR}lib -lc++ -lc++abi -lpthread -Wno-unused-command-line-argument"
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} llvm-${APT_LLVM_V}-dev libclang-${APT_LLVM_V}-dev libclang-rt-${APT_LLVM_V}-dev python3-zmq python3-pip"
export PIP_PACKAGES="--break-system-packages pycapnp"
export DEP_OPTS="CC=clang CXX=clang++ CXXFLAGS='${LIBCXX_FLAGS}' NO_QT=1"
export GOAL="install"
export CI_LIMIT_STACK_SIZE=1
export BITCOIN_CONFIG="\
  --preset=dev-mode \
  -DBUILD_GUI=OFF \
  -DSANITIZERS=thread \
  -DAPPEND_CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKCONTENTION -D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES' \
"
export USE_INSTRUMENTED_LIBCPP="Thread"
