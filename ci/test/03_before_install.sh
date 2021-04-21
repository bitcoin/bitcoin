#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

# Add llvm-symbolizer directory to PATH. Needed to get symbolized stack traces from the sanitizers.
PATH=$PATH:/usr/lib/llvm-6.0/bin/
export PATH

