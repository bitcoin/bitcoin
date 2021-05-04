#!/usr/bin/env bash
#
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export DOCKER_NAME_TAG="ubuntu:20.04"
LIBCXX_DIR="${BASE_SCRATCH_DIR}/msan/build/"
export MSAN_FLAGS="-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer -g -O1 -fno-optimize-sibling-calls"
LIBCXX_FLAGS="-nostdinc++ -stdlib=libc++ -L${LIBCXX_DIR}lib -lc++abi -I${LIBCXX_DIR}include -I${LIBCXX_DIR}include/c++/v1 -lpthread -Wl,-rpath,${LIBCXX_DIR}lib -Wno-unused-command-line-argument"
export MSAN_AND_LIBCXX_FLAGS="${MSAN_FLAGS} ${LIBCXX_FLAGS}"

export CONTAINER_NAME="ci_native_msan"
export PACKAGES="clang-9 llvm-9 cmake"
export DEP_OPTS="NO_BDB=1 NO_QT=1 CC='clang' CXX='clang++' CFLAGS='${MSAN_FLAGS}' CXXFLAGS='${MSAN_AND_LIBCXX_FLAGS}' boost_cxxflags='-std=c++17 -fvisibility=hidden -fPIC ${MSAN_AND_LIBCXX_FLAGS}' zeromq_cxxflags='-std=c++17 ${MSAN_AND_LIBCXX_FLAGS}'"
export GOAL="install"
export BITCOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer,memory --with-asm=no --prefix=${DEPENDS_DIR}/x86_64-pc-linux-gnu/ CC=clang CXX=clang++ CFLAGS='${MSAN_FLAGS}' CXXFLAGS='${MSAN_AND_LIBCXX_FLAGS}'"
export USE_MEMORY_SANITIZER="true"
export RUN_UNIT_TESTS="false"
export RUN_FUNCTIONAL_TESTS="false"
export RUN_FUZZ_TESTS=true
export CCACHE_SIZE=250M
