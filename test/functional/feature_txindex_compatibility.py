#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test txindex forward compatibility.

"""

import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

class TxIndexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-txindex"],["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(
            self.num_nodes,
            extra_args=self.extra_args,
            versions=[
                None,
                280200,
            ],
        )
        self.start_nodes()

    def run_test(self):
        self._test_txindex_compatibility()

    def _test_txindex_compatibility(self):
        node = self.nodes[0]
        legacy_node = self.nodes[1]
        self.wallet = MiniWallet(self.nodes[0])
        tx1 = self.wallet.send_self_transfer(from_node=self.nodes[0])
        self.generate(self.nodes[0], 1)
        txId1 = tx1['txid']

        for n in self.nodes:
            self.wait_until(lambda: n.getindexinfo()['txindex']['synced'])

        self.log.info("Ensure that queries to the txindex are consistent between the different index versions")
        assert_equal(node.getrawtransaction(txId1), tx1['hex'])
        assert_equal(legacy_node.getrawtransaction(txId1), tx1['hex'])

        self.log.info("Exercise the new index running on a datadir with the old version")
        self.stop_nodes()
        self.cleanup_folder(node.chain_path)
        shutil.copytree(legacy_node.chain_path, node.chain_path)
        msg = "txindex contains entries in the legacy format"
        with node.assert_debug_log(expected_msgs=[msg]):
            self.start_node(0)
        self.wait_until(lambda: node.getindexinfo()['txindex']['synced'])
        assert_equal(node.getrawtransaction(txId1), tx1['hex'])

        self.log.info("Test that looking up a newly added transaction in a mixed-format db is possible")
        tx2 = self.wallet.send_self_transfer(from_node=self.nodes[0])
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        txId2 = tx2['txid']
        self.wait_until(lambda: node.getindexinfo()['txindex']['synced'])
        assert_equal(node.getrawtransaction(txId1), tx1['hex'])
        assert_equal(node.getrawtransaction(txId2), tx2['hex'])


if __name__ == '__main__':
    TxIndexTest(__file__).main()
