#!/bin/sh
# Copyright (c) 2017-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This simple script checks for commits beginning with: scripted-diff:
# If found, looks for a script between the lines -BEGIN VERIFY SCRIPT- and
# -END VERIFY SCRIPT-. If no ending is found, it reads until the end of the
# commit message.

# The resulting script should exactly transform the previous commit into the current
# one. Any remaining diff signals an error.

export LC_ALL=C
if test "x$1" = "x"; then
    echo "Usage: $0 <commit>..."
    exit 1
fi

RET=0
PREV_BRANCH=$(git name-rev --name-only HEAD)
PREV_HEAD=$(git rev-parse HEAD)
for commit in $(git rev-list --reverse $1); do
    if git rev-list -n 1 --pretty="%s" $commit | grep -q "^scripted-diff:"; then
        git checkout --quiet $commit^ || exit
        SCRIPT="$(git rev-list --format=%b -n1 $commit | sed '/^-BEGIN VERIFY SCRIPT-$/,/^-END VERIFY SCRIPT-$/{//!b};d')"
        if test "x$SCRIPT" = "x"; then
            echo "Error: missing script for: $commit"
            echo "Failed"
            RET=1
        else
            echo "Running script for: $commit"
            echo "$SCRIPT"
            (eval "$SCRIPT")
            git --no-pager diff --exit-code $commit && echo "OK" || (echo "Failed"; false) || RET=1
        fi
        git reset --quiet --hard HEAD
     else
        if git rev-list "--format=%b" -n1 $commit | grep -q '^-\(BEGIN\|END\)[ a-zA-Z]*-$'; then
            echo "Error: script block marker but no scripted-diff in title of commit $commit"
            echo "Failed"
            RET=1
        fi
    fi
done
git checkout --quiet $PREV_BRANCH 2>/dev/null || git checkout --quiet $PREV_HEAD
exit $RET
