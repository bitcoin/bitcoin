#!/usr/bin/env python3
# Copyright (c) 2021 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Common utility functions
'''
import shutil
import sys
import os


def determine_wellknown_cmd(envvar, progname) -> list[str]:
    maybe_env = os.getenv(envvar)
    maybe_which = shutil.which(progname)
    if maybe_env:
        return maybe_env.split(' ')  # Well-known vars are often meant to be word-split
    elif maybe_which:
        return [ maybe_which ]
    else:
        sys.exit(f"{progname} not found")
