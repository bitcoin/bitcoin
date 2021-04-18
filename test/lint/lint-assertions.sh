#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for assertions with obvious side effects.

export LC_ALL=C

EXIT_CODE=0

# PRE31-C (SEI CERT C Coding Standard):
# "Assertions should not contain assignments, increment, or decrement operators."
OUTPUT=$(git grep -E '[^_]assert\(.*(\+\+|\-\-|[^=!<>]=[^=!<>]).*\);' -- "*.cpp" "*.h")
if [[ ${OUTPUT} != "" ]]; then
    echo "Assertions should not have side effects:"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi

# Macro CHECK_NONFATAL(condition) should be used instead of assert for RPC code, where it
# is undesirable to crash the whole program. See: src/util/check.h
# src/rpc/server.cpp is excluded from this check since it's mostly meta-code.
OUTPUT=$(git grep -nE '\<(A|a)ssert *\(.*\);' -- "src/rpc/" "src/wallet/rpc*" ":(exclude)src/rpc/server.cpp")
if [[ ${OUTPUT} != "" ]]; then
    echo "CHECK_NONFATAL(condition) should be used instead of assert for RPC code."
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
