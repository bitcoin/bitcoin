#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CMAKE_GENERATOR="Ninja"
export BITCOIN_CONFIG="-DBUILD_FOR_FUZZING=ON"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="all"
# Can't run tcpdump: tcpdump: en0: You don't have permission to capture on that device
export CI_TCPDUMP_OK_TO_FAIL=1
