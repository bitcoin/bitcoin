#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Helper script to create the cache
# (see BitcoinTestFramework.setup_chain)
#

from test_framework.test_framework import BitcoinTestFramework

class CreateCache(BitcoinTestFramework):

    def setup_network(self):
        # Don't setup any test nodes
        self.options.noshutdown = True

    def run_test(self):
        pass

if __name__ == '__main__':
    CreateCache().main()
