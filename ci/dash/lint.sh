#!/usr/bin/env bash
# Copyright (c) 2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script runs the Dash-specific linters

export LC_ALL=C.UTF-8

set -e

source ./ci/dash/matrix.sh

# Check commit scripts for PRs
if [ "$PULL_REQUEST" != "false" ]; then 
    test/lint/commit-script-check.sh "$COMMIT_RANGE"
fi

# Run linters if CHECK_DOC is set
if [ "$CHECK_DOC" = 1 ]; then
    # TODO: Verify subtrees
    #test/lint/git-subtree-check.sh src/crypto/ctaes
    #test/lint/git-subtree-check.sh src/secp256k1
    #test/lint/git-subtree-check.sh src/univalue
    #test/lint/git-subtree-check.sh src/leveldb
    # TODO: Check docs (re-enable after all Bitcoin PRs have been merged and docs fully fixed)
    #test/lint/check-doc.py
    # Run all linters
    test/lint/all-lint.py
fi