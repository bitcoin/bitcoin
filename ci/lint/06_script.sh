#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

if [ -n "$CIRRUS_PR" ]; then
  # CIRRUS_PR will be present in a Cirrus environment. For builds triggered
  # by a pull request this is the name of the branch targeted by the pull request.
  # https://cirrus-ci.org/guide/writing-tasks/#environment-variables
  COMMIT_RANGE="$CIRRUS_BRANCH..HEAD"
  test/lint/commit-script-check.sh $COMMIT_RANGE
fi

test/lint/check-doc.py
test/lint/check-rpc-mappings.py .
test/lint/lint-all.sh

if [ "$CIRRUS_REPO_FULL_NAME" = "syscoin/syscoin" ] && [ -n "$CIRRUS_CRON" ]; then
    git log --merges --before="2 days ago" -1 --format='%H' > ./contrib/verify-commits/trusted-sha512-root-commit
    ${CI_RETRY_EXE}  gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys $(<contrib/verify-commits/trusted-keys) &&
    ./contrib/verify-commits/verify-commits.py --clean-merge=2;
fi
