#!/usr/bin/env bash
#
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script checks for git modules
export LC_ALL=C
EXIT_CODE=0

CMD=$(git submodule status --recursive)
if test -n "$CMD";
then
    echo These submodules were found, delete them:
    echo "$CMD"
    EXIT_CODE=1
fi

exit $EXIT_CODE

