#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

cp "./ci/retry/retry" "/ci_retry"
set -o errexit; source ./ci/lint/04_install.sh
set -o errexit
./ci/lint/06_script.sh
