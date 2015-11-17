#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test the CHECKLOCKTIMEVERIFY (BIP65) soft-fork logic
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import os
import shutil

class BIP65Test(BitcoinTestFramework):

    def setup_network(self):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, []))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-blockversion=3"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-blockversion=4"]))
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[2], 0)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        cnt = self.nodes[0].getblockcount()

        # Mine some old-version blocks
        self.nodes[1].generate(10)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 10):
            raise AssertionError("Failed to mine 10 version=3 blocks")

        # Mine 75 new-version blocks
        for i in xrange(15):
            self.nodes[2].generate(5)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 85):
            raise AssertionError("Failed to mine 75 version=4 blocks")

        # TODO: check that new CHECKLOCKTIMEVERIFY rules are not enforced

        # Mine 1 new-version block
        self.nodes[2].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 86):
            raise AssertionFailure("Failed to mine a version=4 block")

        # TODO: check that new CHECKLOCKTIMEVERIFY rules are enforced

        # Mine 18 new-version blocks
        self.nodes[2].generate(18)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 104):
            raise AssertionError("Failed to mine 18 version=4 blocks")

        # Mine 1 old-version block
        self.nodes[1].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 105):
            raise AssertionError("Failed to mine a version=3 block after 94 version=4 blocks")

        # Mine 1 new-version blocks
        self.nodes[2].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 106):
            raise AssertionError("Failed to mine a version=3 block")

        # Mine 1 old-version blocks
        try:
            self.nodes[1].generate(1)
            raise AssertionError("Succeeded to mine a version=3 block after 95 version=4 blocks")
        except JSONRPCException:
            pass
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 106):
            raise AssertionError("Accepted a version=3 block after 95 version=4 blocks")

        # Mine 1 new-version blocks
        self.nodes[2].generate(1)
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 107):
            raise AssertionError("Failed to mine a version=4 block")

if __name__ == '__main__':
    BIP65Test().main()
