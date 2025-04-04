#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit -o pipefail -o xtrace


echo "Running test-one-commit on $( git log -1 )"

# Use clang++, because it is a bit faster and uses less memory than g++
CC=clang CXX=clang++ cmake -B build -DWERROR=ON -DWITH_ZMQ=ON -DBUILD_GUI=ON -DBUILD_BENCH=ON -DBUILD_FUZZ_BINARY=ON -DWITH_BDB=ON -DWITH_USDT=ON -DCMAKE_CXX_FLAGS='-Wno-error=unused-member-function'

cmake --build build -j "$( nproc )" && ctest --output-on-failure --stop-on-failure --test-dir build -j "$( nproc )"

./build/test/functional/test_runner.py -j $(( $(nproc) * 2 )) --combinedlogslen=99999999
