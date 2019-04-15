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
    --ignore-names "argtypes,connection_lost,connection_made,converter,data_received,daemon,errcheck,is_compressed,is_valid,verify_ecdsa,msg_generic,on_*,optionxform,restype,profile_with_perf" \
    $(git ls-files -- "*.py" ":(exclude)contrib/" ":(exclude)test/functional/data/invalid_txs.py")
