#!/usr/bin/env python3
#
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# This script runs all test/lint/lint-* files, and fails if any exit
# with a non-zero status code.

from glob import glob
from pathlib import Path
from subprocess import run
from sys import executable

exit_code = 0
mod_path = Path(__file__).parent
for lint in glob(f"{mod_path}/lint-*.py"):
    result = run([executable, lint])
    if result.returncode != 0:
        print(f"^---- failure generated from {lint.split('/')[-1]}")
        exit_code |= result.returncode

exit(exit_code)
