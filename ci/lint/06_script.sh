#!/usr/bin/env bash
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

if [ -n "$LOCAL_BRANCH" ]; then
  # To faithfully recreate CI linting locally, specify all commits on the current
  # branch.
  COMMIT_RANGE="$(git merge-base HEAD master)..HEAD"
elif [ -n "$CIRRUS_PR" ]; then
  COMMIT_RANGE="HEAD~..HEAD"
  echo
  git log --no-merges --oneline "$COMMIT_RANGE"
  echo
  test/lint/commit-script-check.sh "$COMMIT_RANGE"
else
  COMMIT_RANGE="SKIP_EMPTY_NOT_A_PR"
fi
export COMMIT_RANGE

test/lint/check-doc.py
test/lint/all-lint.py

if [ "$CIRRUS_REPO_FULL_NAME" = "syscoin/syscoin" ] && [ "$CIRRUS_PR" = "" ] ; then
    # Sanity check only the last few commits to get notified of missing sigs,
    # missing keys, or expired keys. Usually there is only one new merge commit
    # per push on the master branch and a few commits on release branches, so
    # sanity checking only a few (10) commits seems sufficient and cheap.
    git log HEAD~10 -1 --format='%H' > ./contrib/verify-commits/trusted-sha512-root-commit
    git log HEAD~10 -1 --format='%H' > ./contrib/verify-commits/trusted-git-root
    mapfile -t KEYS < contrib/verify-commits/trusted-keys
    git config user.email "ci@ci.ci"
    git config user.name "ci"
    ${CI_RETRY_EXE} gpg --keyserver hkps://keys.openpgp.org --recv-keys "${KEYS[@]}" &&
    ./contrib/verify-commits/verify-commits.py;
fi
