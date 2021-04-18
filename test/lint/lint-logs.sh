#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check that all logs are terminated with '\n'
#
# Some logs are continued over multiple lines. They should be explicitly
# commented with \* Continued *\
#
# There are some instances of LogPrintf() in comments. Those can be
# ignored

export LC_ALL=C
UNTERMINATED_LOGS=$(git grep --extended-regexp "LogPrintf?\(" -- "*.cpp" | \
    grep -v '\\n"' | \
    grep -v '\.\.\.' | \
    grep -v "/\* Continued \*/" | \
    grep -v "LogPrint()" | \
    grep -v "LogPrintf()")
if [[ ${UNTERMINATED_LOGS} != "" ]]; then
    # shellcheck disable=SC2028
    echo "All calls to LogPrintf() and LogPrint() should be terminated with \\n"
    echo
    echo "${UNTERMINATED_LOGS}"
    exit 1
fi
