#!/usr/bin/env python3
"""utxoindex test
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    sync_blocks,
    sync_mempools,
    assert_is_hash_string,
)

class UTXOIndexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [["-utxoscriptindex"], [], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def check_utxoscriptindex(self, node_id, minConf, addr, expected_txid):
        txouts = self.nodes[node_id].getutxoscriptindex(minConf, (addr,))
        txid = txouts[0]["txid"]
        assert_is_hash_string(txid)
        assert_equal(txid, expected_txid)

    def assert_empty_utxoscriptindex(self, node_id, minConf, addr):
        txouts = self.nodes[node_id].getutxoscriptindex(minConf, (addr,))
        assert_equal(txouts, [])

    def run_test(self):
        print("Generating test blockchain...")

        # Check that there's no UTXO on any of the nodes
        for node in self.nodes:
            assert_equal(node.listunspent(), [])

        # mining
        self.nodes[0].generate(101)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), 50)

        # TX1: send from node0 to node1
        # - check if txout from tx1 is there
        address = self.nodes[1].getnewaddress()
        txid1 = self.nodes[0].sendtoaddress(address, 10)

        self.sync_all()

        self.check_utxoscriptindex(node_id = 0, minConf = 0, addr = address, expected_txid = txid1)

        self.nodes[0].generate(101)  # node will collect its own fee

        self.check_utxoscriptindex(node_id=0, minConf=1, addr=address, expected_txid=txid1)

        #restart node 2 with utxoscriptindex on, shall reindex utxo database
        self.stop_node(2)

        self.start_node(2, ["-utxoscriptindex"])

        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[1], 2)

        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), 5090)
        assert_equal(self.nodes[1].getbalance(), 10)

        self.check_utxoscriptindex(node_id = 0, minConf = 1, addr = address, expected_txid = txid1)
        self.check_utxoscriptindex(node_id = 2, minConf = 1, addr = address, expected_txid = txid1)

        # Restart node 2 and check if utxo index is still valid
        self.stop_node(2)

        self.start_node(2, ["-utxoscriptindex"])

        self.sync_all()

        self.check_utxoscriptindex(node_id = 2, minConf = 1, addr = address, expected_txid = txid1)

        # Stop node 2. We want to restart it later and orphan a node 1 block in
        # order to test txoutindex handling the reorg. In other words, node 2 is
        # stopped so that it won't build on a node 1 block.
        self.stop_node(2)

        # TX2: send from node1 to node0
        # - check if txout from tx1 is gone
        # - check if txout from tx2 is there
        address2 = self.nodes[0].getnewaddress()
        txid2 = self.nodes[1].sendtoaddress(address2, 5)

        sync_mempools([self.nodes[0], self.nodes[1]])

        self.assert_empty_utxoscriptindex(node_id = 0, minConf = 0, addr = address)
        self.check_utxoscriptindex(node_id = 0, minConf = 0, addr = address2, expected_txid = txid2)

        self.nodes[1].generate(1)
        self.sync_all([self.nodes[:2]])
        assert_equal(self.nodes[0].getbalance(), 5145)

        self.assert_empty_utxoscriptindex(node_id=0, minConf=0, addr=address)
        self.assert_empty_utxoscriptindex(node_id=0, minConf=1, addr=address)
        self.check_utxoscriptindex(node_id = 0, minConf = 0, addr = address2, expected_txid = txid2)
        self.check_utxoscriptindex(node_id=0, minConf=1, addr=address2, expected_txid=txid2)

        # start node 2
        self.start_node(2, self.extra_args[2])

        # mine 10 blocks alone to have the longest chain
        self.nodes[2].generate(10)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[1], 2)
        self.nodes[2].generate(1)
        sync_blocks(self.nodes)

        # TX2 must be reverted
        # - check if txout from tx1 is there again
        # - check if txout from tx2 is gone
        assert_equal(self.nodes[0].getbalance(), 5640)

        self.assert_empty_utxoscriptindex(node_id=0, minConf=0, addr=address)
        self.check_utxoscriptindex(node_id = 0, minConf = 1, addr = address, expected_txid = txid1)
        self.check_utxoscriptindex(node_id = 0, minConf = 0, addr = address2, expected_txid = txid2)
        self.assert_empty_utxoscriptindex(node_id=0, minConf=1, addr=address2)

if __name__ == '__main__':
    UTXOIndexTest().main()
