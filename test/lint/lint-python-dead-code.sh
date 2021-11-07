#!/usr/bin/env bash
#
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Find dead Python code.

export LC_ALL=C

if ! command -v vulture > /dev/null; then
    echo "Skipping Python dead code linting since vulture is not installed. Install by running \"pip3 install vulture\""
    exit 0
fi

# --min-confidence 100 will only report code that is guaranteed to be unused within the analyzed files.
# Any value below 100 introduces the risk of false positives, which would create an unacceptable maintenance burden.
mapfile -t FILES < <(git ls-files -- "*.py")
if ! vulture --min-confidence 100 "${FILES[@]}"; then
    echo "Python dead code detection found some issues"
    exit 1
fi
