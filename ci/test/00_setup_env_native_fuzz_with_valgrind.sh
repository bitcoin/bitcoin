#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="ubuntu:22.04"
export CONTAINER_NAME=ci_native_fuzz_valgrind
export PACKAGES="clang llvm python3 libevent-dev bsdmainutils libboost-dev libsqlite3-dev valgrind"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export FUZZ_TESTS_CONFIG="--valgrind"
export GOAL="install"
# Temporarily pin dwarf 4, until valgrind can understand clang's dwarf 5
export SYSCOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer CC=clang CXX=clang++ CFLAGS='-gdwarf-4' CXXFLAGS='-gdwarf-4'"
export CCACHE_SIZE=200M
