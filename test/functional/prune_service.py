#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import (NetworkThread,
                                     NodeConn,
                                     NodeConnCB)

class BaseNode(NodeConnCB):
    def on_version(self, conn, message):
        assert_equal(message.nServices, 8) # make sure NODE_BLOOM is disabled
        pass
        
class PruneServiceFlagsTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-prune=550']]

    def setup_network(self):
        self.nodes = start_nodes(1, self.options.tmpdir, self.extra_args[:1])


    def run_test(self):
        # Connect to node0
        node0 = BaseNode()
        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], node0))
        node0.add_connection(connections[0])
        NetworkThread().start() 
        node0.wait_for_verack()


if __name__ == '__main__':
    PruneServiceFlagsTest().main()
