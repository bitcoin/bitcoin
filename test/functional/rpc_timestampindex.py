#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2017-2018 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test timestampindex generation and fetching
#

import time

from test_framework.test_framework import RavenTestFramework
from test_framework.util import *


class TimestampIndexTest(RavenTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        self.add_nodes(4, [
            # Nodes 0/1 are "wallet" nodes
            ["-debug"],
            ["-debug", "-timestampindex"],
            # Nodes 2/3 are used for testing
            ["-debug"],
            ["-debug", "-timestampindex"]])

        self.start_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 0, 2)
        connect_nodes_bi(self.nodes, 0, 3)

        self.sync_all()

    def run_test(self):
        print("Mining 25 blocks...")
        blockhashes = self.nodes[0].generate(25)
        time.sleep(3)
        print("Mining 25 blocks...")
        blockhashes.extend(self.nodes[0].generate(25))
        time.sleep(3)
        print("Mining 25 blocks...")
        blockhashes.extend(self.nodes[0].generate(25))
        self.sync_all()
        low = self.nodes[1].getblock(blockhashes[0])["time"]
        high = low + 76

        print("Checking timestamp index...")
        hashes = self.nodes[1].getblockhashes(high, low)

        assert_equal(len(hashes), len(blockhashes))

        assert_equal(hashes, blockhashes)

        print("Passed\n")


if __name__ == '__main__':
    TimestampIndexTest().main()
