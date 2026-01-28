#!/usr/bin/env bash
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -o errexit -o pipefail -o xtrace

if [ -n "${LINT_CI_IS_PR}" ]; then
  export COMMIT_RANGE="HEAD~..HEAD"
  if [ "$(git rev-list -1 HEAD)" != "$(git rev-list -1 --merges HEAD)" ]; then
    echo "Error: The top commit must be a merge commit, usually the remote 'pull/<PR_NUMBER>/merge' branch."
    false
  fi
fi

RUST_BACKTRACE=1 cargo run --manifest-path "./test/lint/test_runner/Cargo.toml" -- "$@"

if [ "${LINT_CI_SANITY_CHECK_COMMIT_SIG}" = "1" ] ; then
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
