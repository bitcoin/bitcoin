#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Exercise API with -disablewallet.
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class DisableWalletTest (BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self, split=False):
        self.nodes = start_nodes(1, self.options.tmpdir, [['-disablewallet']])
        self.is_network_split = False
        self.sync_all()

    def run_test (self):
        # Check regression: https://github.com/bitcoin/bitcoin/issues/6963#issuecomment-154548880
        x = self.nodes[0].validateaddress('3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy')
        assert(x['isvalid'] == False)
        x = self.nodes[0].validateaddress('mneYUmWYsuk7kySiURxCi3AGxrAqZxLgPZ')
        assert(x['isvalid'] == True)

        # Checking mining to an address without a wallet
        try:
            self.nodes[0].generatetoaddress(1, 'mneYUmWYsuk7kySiURxCi3AGxrAqZxLgPZ')
        except JSONRPCException,e:
            assert("Invalid address" not in e.error['message'])
            assert("ProcessNewBlock, block not accepted" not in e.error['message'])
            assert("Couldn't create new block" not in e.error['message'])

        try:
            self.nodes[0].generatetoaddress(1, '3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy')
            raise AssertionError("Must not mine to invalid address!")
        except JSONRPCException,e:
            assert("Invalid address" in e.error['message'])

if __name__ == '__main__':
    DisableWalletTest ().main ()
