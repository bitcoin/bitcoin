#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Commits a scripted-diff from a script or text file.

export LC_ALL=C

SCRIPT_FILE=$1
TITLE=$2
DESCRIPTION=$3

git commit -m "scripted-diff: $TITLE" \
-m "$DESCRIPTION" \
-m "
-BEGIN VERIFY SCRIPT-
$(cat $SCRIPT_FILE)
-END VERIFY SCRIPT-
"

# You may now clean up the script file.
