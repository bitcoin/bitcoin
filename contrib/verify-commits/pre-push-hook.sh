#!/usr/bin/env bash
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C
if ! [[ "$2" =~ ^(git@)?(www.)?github.com(:|/)bitcoin/bitcoin(.git)?$ ]]; then
    exit 0
fi

while read LINE; do
    set -- A "$LINE"
    if [ "$4" != "refs/heads/master" ]; then
        continue
    fi
    if ! ./contrib/verify-commits/verify-commits.py "$3" > /dev/null 2>&1; then
        echo "ERROR: A commit is not signed, can't push"
        ./contrib/verify-commits/verify-commits.py
        exit 1
    fi
done < /dev/stdin
