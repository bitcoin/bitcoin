#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

if [ "$TRAVIS_EVENT_TYPE" = "pull_request" ]; then
  # TRAVIS_BRANCH will be present in a Travis environment. For builds triggered
  # by a pull request this is the name of the branch targeted by the pull request.
  # https://docs.travis-ci.com/user/environment-variables/
  COMMIT_RANGE="$TRAVIS_BRANCH..HEAD"
  test/lint/commit-script-check.sh $COMMIT_RANGE
fi

test/lint/git-subtree-check.sh src/crypto/ctaes
test/lint/git-subtree-check.sh src/secp256k1-zkp
test/lint/git-subtree-check.sh src/univalue
test/lint/git-subtree-check.sh src/leveldb
test/lint/git-subtree-check.sh src/crc32c
test/lint/check-doc.py
test/lint/check-rpc-mappings.py .
test/lint/lint-all.sh

if [ "$TRAVIS_REPO_SLUG" = "bitcoin/bitcoin" ] && [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
    git log --merges --before="2 days ago" -1 --format='%H' > ./contrib/verify-commits/trusted-sha512-root-commit
    travis_retry gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys $(<contrib/verify-commits/trusted-keys) &&
    ./contrib/verify-commits/verify-commits.py --clean-merge=2;
fi
