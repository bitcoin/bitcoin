#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Homebrew's python@3.12 is marked as externally managed (PEP 668).
# Therefore, `--break-system-packages` is needed.
export PIP_PACKAGES="--break-system-packages zmq"
export GOAL="install"
export CMAKE_GENERATOR="Ninja"
export BITCOIN_CONFIG="\
  -DBUILD_GUI=ON -DWITH_ZMQ=ON -DREDUCE_EXPORTS=ON \
  -DBUILD_FUZZ_BINARY=ON -DCMAKE_BUILD_TYPE=Debug -DAPPEND_CFLAGS='-O2 -g' -DAPPEND_CXXFLAGS='-O2 -g' \
"
export RUN_FUZZ_TESTS=true
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
