#!/usr/bin/env python3
# Copyright 2014 BitPay Inc.
# Copyright 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test framework for bitcoin utils.

Runs automatically during `make check`.

Can also be run manually."""

import os
import sys
import argparse
import logging

if __name__ == '__main__':
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import buildenv
    import bctest

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-v', '--verbose', action='store_true')
    args = parser.parse_args()
    verbose = args.verbose

    if verbose:
        level = logging.DEBUG
    else:
        level = logging.ERROR
    formatter = '%(asctime)s - %(levelname)s - %(message)s'
    # Add the format/level to the logger
    logging.basicConfig(format = formatter, level=level)

    bctest.bctester(buildenv.SRCDIR + "/test/util/data", "bitcoin-util-test.json", buildenv)
