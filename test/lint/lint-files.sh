#!/usr/bin/env bash
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

set -e
cd "$(dirname "$0")/../.."
test/lint/lint-files.py
