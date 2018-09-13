#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test relay of transaction chains.

Ensure that if bitcoind announces a transaction it will also respond
to getdata requests for parents of the transaction.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    disconnect_nodes,
    sync_blocks
)

class ChainRelayTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Generating one utxo for node0")

        self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[1].generate(100)
        self.sync_all()

        # Node0 should now have 1 utxo.
        assert_equal(len(self.nodes[0].listunspent()), 1)

        assert self.nodes[0].getbalance() > 10 # Should be 50, but doesn't matter

        self.log.info("Disconnect node0 from node1")
        disconnect_nodes(self.nodes[0], 1)
        disconnect_nodes(self.nodes[1], 0)

        assert_equal(len(self.nodes[0].getpeerinfo()), 0)

        # This next part is sort of a hack -- we're going to reconnect the
        # nodes after generating a transaction on node0, and we'll need a way
        # to test that the p2p connections are fully up before generating the
        # child transaction. To do that we will just test whether a block generated
        # on node1 makes it over to node0. Perhaps there's a status field in
        # getpeerinfo() that might be sufficient as well (synced_headers or
        # something?).
        self.log.info("Mine a block on node1")
        self.nodes[1].generate(1)

        self.log.info("Generating new transaction on node0")
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1, "", "", False)
        assert_equal(len(self.nodes[0].getrawmempool()), 1)

        self.log.info("Reconnect node0 to node1")
        connect_nodes(self.nodes[0], 1)

        sync_blocks(self.nodes)
        # Check that node1's mempool is still empty
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        self.log.info("Generating child transaction on node0")
        txid2 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 2, "", "", False)
        assert_equal(self.nodes[0].getmempoolentry(txid2)['ancestorcount'], 2)

        self.log.info("Check that both transactions end up on node1")
        self.sync_all()

        # sync_all ensures the mempool's are synced, but for avoidance of doubt
        # just verify that the child transaction made it to node1
        assert txid2 in self.nodes[1].getrawmempool()

        self.log.info("Node1 successfully synced")

if __name__ == '__main__':
    ChainRelayTest().main()
