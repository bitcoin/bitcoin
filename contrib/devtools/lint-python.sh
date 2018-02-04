#!/bin/sh
#
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for specified flake8 warnings in python files.

# F401: module imported but unused
flake8 --ignore=B,C,E,F,I,N,W --select=F401 .
