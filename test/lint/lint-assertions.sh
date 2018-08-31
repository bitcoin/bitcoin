#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
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

exit ${EXIT_CODE}
