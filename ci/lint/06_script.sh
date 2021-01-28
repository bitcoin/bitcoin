#!/usr/bin/env bash
#
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

GIT_HEAD=$(git rev-parse HEAD)
if [ -n "$CIRRUS_PR" ]; then
  COMMIT_RANGE="$CIRRUS_BASE_SHA..$GIT_HEAD"
  test/lint/commit-script-check.sh $COMMIT_RANGE
fi
export COMMIT_RANGE

test/lint/check-doc.py
test/lint/lint-all.sh

if [ "$CIRRUS_REPO_FULL_NAME" = "syscoin/syscoin" ] && [ -n "$CIRRUS_CRON" ]; then
    git log --merges --before="2 days ago" -1 --format='%H' > ./contrib/verify-commits/trusted-sha512-root-commit
    ${CI_RETRY_EXE}  gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys $(<contrib/verify-commits/trusted-keys) &&
    ./contrib/verify-commits/verify-commits.py --clean-merge=2;
fi

echo
git log --no-merges --oneline $COMMIT_RANGE
