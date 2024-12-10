#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet_libbitcoinkernel
export CI_IMAGE_NAME_TAG="docker.io/debian:bookworm"
# Use minimum supported python3.10 (or best-effort 3.11) and clang-16, see doc/dependencies.md
export PACKAGES="python3-zmq clang-16 llvm-16 libc++abi-16-dev libc++-16-dev"
export DEP_OPTS="NO_WALLET=1 CC=clang-16 CXX='clang++-16 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="\
 -DAPPEND_CPPFLAGS='\
  -D_LIBCPP_ABI_BOUNDED_ITERATORS \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STD_ARRAY \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_STRING \
  -D_LIBCPP_ABI_BOUNDED_ITERATORS_IN_VECTOR \
  -D_LIBCPP_ABI_BOUNDED_UNIQUE_PTR \
 ' \
 -DBUILD_KERNEL_LIB=ON \
 -DBUILD_SHARED_LIBS=ON \
 -DBUILD_UTIL_CHAINSTATE=ON \
 -DREDUCE_EXPORTS=ON \
"
