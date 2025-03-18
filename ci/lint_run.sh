#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8
set -o errexit -o pipefail -o xtrace

# Only used in .cirrus.yml. Refer to test/lint/README.md on how to run locally.
export PATH="/python_build/bin:${PATH}"
./ci/lint/06_script.sh
