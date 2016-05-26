#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test the BIP66 changeover logic
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class BIP66Test(BitcoinTestFramework):

    def __init__(self):
        self.num_nodes = 3

    def setup_chain(self):
        print "Initializing test directory "+self.options.tmpdir
        initialize_chain_clean(self.options.tmpdir, self.num_nodes)

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-blockversion=2"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-blockversion=3"]))
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[2], 0)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        cnt = self.nodes[0].getblockcount()

        # Mine some old-version blocks
        self.nodes[1].wallet.generate(10)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 10):
            raise AssertionError("Failed to mine 10 version=2 blocks")

        # Mine 75 new-version blocks
        for i in xrange(15):
            self.nodes[2].wallet.generate(5)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 85):
            raise AssertionError("Failed to mine 75 version=3 blocks")

        # TODO: check that new DERSIG rules are not enforced

        # Mine 1 new-version block
        self.nodes[2].wallet.generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 86):
            raise AssertionError("Failed to mine a version=3 blocks")

        # TODO: check that new DERSIG rules are enforced

        # Mine 18 new-version blocks
        for i in xrange(2):
            self.nodes[2].wallet.generate(9)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 104):
            raise AssertionError("Failed to mine 18 version=3 blocks")

        # Mine 1 old-version block
        self.nodes[1].wallet.generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 105):
            raise AssertionError("Failed to mine a version=2 block after 94 version=3 blocks")

        # Mine 1 new-version blocks
        self.nodes[2].wallet.generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 106):
            raise AssertionError("Failed to mine a version=3 block")

        # Mine 1 old-version blocks
        try:
            self.nodes[1].wallet.generate(1)
            raise AssertionError("Succeeded to mine a version=2 block after 95 version=3 blocks")
        except JSONRPCException:
            pass
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 106):
            raise AssertionError("Accepted a version=2 block after 95 version=3 blocks")

        # Mine 1 new-version blocks
        self.nodes[2].wallet.generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 107):
            raise AssertionError("Failed to mine a version=3 block")

if __name__ == '__main__':
    BIP66Test().main()
