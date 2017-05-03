#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool persistence.

By default, bitcoind will dump mempool on shutdown and
then reload it on startup. This can be overridden with
the -persistmempool=false command line option.

Test is as follows:

  - start node0, node1 and node2. node1 has -persistmempool=false
  - create 5 transactions on node2 to its own address. Note that these
    are not sent to node0 or node1 addresses because we don't want
    them to be saved in the wallet.
  - check that node0 and node1 have 5 transactions in their mempools
  - shutdown all nodes.
  - startup node0. Verify that it still has 5 transactions
    in its mempool. Shutdown node0. This tests that by default the
    mempool is persistent.
  - startup node1. Verify that its mempool is empty. Shutdown node1.
    This tests that with -persistmempool=false, the mempool is not
    dumped to disk when the node is shut down.
  - Restart node0 with -persistmempool=false. Verify that its mempool is
    empty. Shutdown node0. This tests that with -persistmempool=false,
    the mempool is not loaded from disk on start up.
  - Restart node0 with -persistmempool=true. Verify that it has 5
    transactions in its mempool. This tests that -persistmempool=false
    does not overwrite a previously valid mempool stored on disk.

"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class MempoolPersistTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 3
        self.setup_clean_chain = False

    def setup_network(self):
        # We need 3 nodes for this test. Node1 does not have a persistent mempool.
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-persistmempool=false"]))
        self.nodes.append(start_node(2, self.options.tmpdir))
        connect_nodes_bi(self.nodes, 0, 2)
        connect_nodes_bi(self.nodes, 1, 2)
        self.is_network_split = False

    def run_test(self):
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)

        self.log.debug("Mine a single block to get out of IBD")
        self.nodes[0].generate(1)

        self.log.debug("Send 5 transactions from node2 (to its own address)")
        for i in range(5):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), Decimal("10"))
        self.sync_all()

        self.log.debug("Verify that node0 and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

        self.log.debug("Stop-start node0 and node1. Verify that node0 has the transactions in its mempool and node1 does not.")
        stop_nodes(self.nodes)
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir))
        self.nodes.append(start_node(1, self.options.tmpdir))
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        self.log.debug("Stop-start node0 with -persistmempool=false. Verify that it doesn't load its mempool.dat file.")
        stop_nodes(self.nodes)
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-persistmempool=false"]))
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        self.log.debug("Stop-start node0. Verify that it has the transactions in its mempool.")
        stop_nodes(self.nodes)
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir))
        assert_equal(len(self.nodes[0].getrawmempool()), 5)

if __name__ == '__main__':
    MempoolPersistTest().main()
