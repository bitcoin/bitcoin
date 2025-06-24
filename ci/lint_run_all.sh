#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Only used in .cirrus.yml for stale re-runs of old pull request tasks. This
# file can be removed in September 2025.

cp "./ci/retry/retry" "/ci_retry"
cp "./.python-version" "/.python-version"
mkdir --parents "/test/lint"
cp --recursive "./test/lint/test_runner" "/test/lint/"
set -o errexit; source ./ci/lint/01_install.sh
set -o errexit
./ci/lint/06_script.sh
