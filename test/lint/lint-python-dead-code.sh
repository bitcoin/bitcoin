#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Find dead Python code.

export LC_ALL=C

if ! command -v vulture > /dev/null; then
    echo "Skipping Python dead code linting since vulture is not installed. Install by running \"pip3 install vulture\""
    exit 0
fi

vulture \
    --min-confidence 60 \
    $(git rev-parse --show-toplevel) \
    $(dirname "${BASH_SOURCE[0]}")/lint-python-dead-code-whitelist
