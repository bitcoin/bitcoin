#!/bin/bash
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
    --ignore-names "argtypes,connection_lost,connection_made,converter,data_received,daemon,errcheck,get_ecdh_key,get_privkey,is_compressed,is_fullyvalid,msg_generic,on_*,optionxform,restype,set_privkey,*serialize_v2" \
    $(git ls-files -- "*.py" ":(exclude)contrib/" ":(exclude)src/crc32c/" ":(exclude)test/functional/test_framework/address.py")
