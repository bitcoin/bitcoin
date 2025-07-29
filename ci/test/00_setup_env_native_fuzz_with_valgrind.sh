#!/usr/bin/env bash
#
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_fuzz_valgrind
export PACKAGES="clang llvm libclang-rt-dev python3 libevent-dev bsdmainutils libboost-dev valgrind"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export FUZZ_TESTS_CONFIG="--valgrind"
export GOAL="install"
# Temporarily pin dwarf 4, until valgrind can understand clang's dwarf 5
export BITCOIN_CONFIG="--enable-fuzz --with-sanitizers=fuzzer CC=clang-18 CXX=clang++-18 CFLAGS='-gdwarf-4' CXXFLAGS='-gdwarf-4'"
export CCACHE_MAXSIZE=200M
