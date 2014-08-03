#!/usr/bin/env python
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the getchaintips API.

# Since the test framework does not generate orphan blocks, we can
# unfortunately not check for them!

from test_framework import BitcoinTestFramework
from util import assert_equal

class GetChainTipsTest (BitcoinTestFramework):

    def run_test (self, nodes):
        res = nodes[0].getchaintips ()
        assert_equal (len (res), 1)
        res = res[0]
        assert_equal (res['branchlen'], 0)
        assert_equal (res['height'], 200)

if __name__ == '__main__':
    GetChainTipsTest ().main ()
