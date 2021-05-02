#!/usr/bin/env bash
#
# Copyright (c) 2018 The XBit Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for SIGNAL/SLOT connect style, removed since Qt4 support drop.

export LC_ALL=C

EXIT_CODE=0

OUTPUT=$(git grep -E '(SIGNAL|, ?SLOT)\(' -- src/qt)
if [[ ${OUTPUT} != "" ]]; then
    echo "Use Qt5 connect style in:"
    echo "$OUTPUT"
    EXIT_CODE=1
fi

exit ${EXIT_CODE}
