#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for common C++ issues.

export LC_ALL=C

EXIT_CODE=0

# Check for use of NULL instead of nullptr.
#
# The POSIX regular expression [^abc] matches any character except a, b and c.
OUTPUT=$(git grep -E "[^A-Za-z0-9_\"]NULL[^A-Za-z0-9_\"]" -- "*.cpp" "*.h" ":(exclude)src/leveldb/" ":(exclude)src/secp256k1/" ":(exclude)src/univalue/" ":(exclude)src/torcontrol.cpp" | grep -vE "//.*NULL")
if [[ ${OUTPUT} != "" ]]; then
    echo "Use nullptr instead of NULL:"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
