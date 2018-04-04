#!/bin/bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check unnecessary extern keyword usage for function declarations.

REGEXP_EXCLUDE_FILES_WITH_PREFIX="src/(crypto/ctaes/|leveldb/|secp256k1/|tinyformat.h|univalue/)"

EXIT_CODE=0
for SOURCE_FILE in $(git ls-files -- "*.h" "*.cpp" | grep -vE "^${REGEXP_EXCLUDE_FILES_WITH_PREFIX}")
do
    if [[ $(grep -E -c "extern ([^_\"\(]+)\(" ${SOURCE_FILE}) != 0 ]]; then
        echo "${SOURCE_FILE} is using unnecessary extern keyword for function declarations"
        EXIT_CODE=1
    fi
done
exit ${EXIT_CODE}
