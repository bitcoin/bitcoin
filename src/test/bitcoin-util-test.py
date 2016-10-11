#!/usr/bin/env python
# Copyright 2014 BitPay, Inc.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from __future__ import division,print_function,unicode_literals
import os
import bctest
import buildenv
import argparse

help_text="""Test framework for bitcoin utils.

Runs automatically during `make check`.

Can also be run manually from the src directory by specifiying the source directory:

test/bitcoin-util-test.py --src=[srcdir]
"""


if __name__ == '__main__':
    verbose = False
    try:
        srcdir = os.environ["srcdir"]
    except:
        parser = argparse.ArgumentParser(description=help_text)
        parser.add_argument('-s', '--srcdir')
        parser.add_argument('-v', '--verbose', action='store_true')
        args = parser.parse_args()
        srcdir = args.srcdir
        verbose = args.verbose
    bctest.bctester(srcdir + "/test/data", "bitcoin-util-test.json", buildenv, verbose = verbose)
