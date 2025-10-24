#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME="ci_mac_native_fuzz"  # macos does not use a container, but the env var is needed for logging
export CMAKE_GENERATOR="Ninja"
export BITCOIN_CONFIG="-DBUILD_FOR_FUZZING=ON -DCMAKE_EXE_LINKER_FLAGS='-Wl,-stack_size -Wl,0x80000' -DAPPEND_CPPFLAGS='-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG'"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=true
export GOAL="all"
# Can't run tcpdump: tcpdump: en0: You don't have permission to capture on that device
export CI_TCPDUMP_OK_TO_FAIL=1
