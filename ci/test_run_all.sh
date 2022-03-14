#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

set -o errexit; source ./ci/test/00_setup_env.sh
set -o errexit; source ./ci/test/03_before_install.sh
set -o errexit; source ./ci/test/04_install.sh
set -o errexit; source ./ci/test/05_before_script.sh
