#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Make sure we explicitly open all text files using UTF-8 (or ASCII) encoding to
# avoid potential issues on the BSDs where the locale is not always set.

export LC_ALL=C
EXIT_CODE=0
OUTPUT=$(git grep " open(" -- "*.py" ":(exclude)src/crc32c/" | grep -vE "encoding=.(ascii|utf8|utf-8)." | grep -vE "open\([^,]*, ['\"][^'\"]*b[^'\"]*['\"]")
if [[ ${OUTPUT} != "" ]]; then
    echo "Python's open(...) seems to be used to open text files without explicitly"
    echo "specifying encoding=\"utf8\":"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi
OUTPUT=$(git grep "check_output(" -- "*.py" ":(exclude)src/crc32c/"| grep "universal_newlines=True" | grep -vE "encoding=.(ascii|utf8|utf-8).")
if [[ ${OUTPUT} != "" ]]; then
    echo "Python's check_output(...) seems to be used to get program outputs without explicitly"
    echo "specifying encoding=\"utf8\":"
    echo
    echo "${OUTPUT}"
    EXIT_CODE=1
fi
exit ${EXIT_CODE}
