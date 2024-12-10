#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export APT_LLVM_V="19"
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} libclang-rt-${APT_LLVM_V}-dev libc++abi-${APT_LLVM_V}-dev libc++-${APT_LLVM_V}-dev python3-zmq"
export DEP_OPTS="CC=clang-${APT_LLVM_V} CXX='clang++-${APT_LLVM_V} -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="\
 -DAPPEND_CPPFLAGS='\
  -DARENA_DEBUG \
  -DDEBUG_LOCKCONTENTION \
  -DDEBUG_LOCKORDER \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STD_ARRAY \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STRING \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_VECTOR \
  -D_LIBCPP_ABI_BOUNDED_UNIQUE_PTR \
  -D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES \
 ' \
 -DSANITIZERS=thread \
 -DWITH_ZMQ=ON \
"
