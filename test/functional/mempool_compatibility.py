#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that mempool.dat is both backward and forward compatible between versions

NOTE: The test is designed to prevent cases when compatibility is broken accidentally.
In case we need to break mempool compatibility we can continue to use the test by just bumping the version number.

Previous releases are required by this test, see test/README.md.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


class MempoolCompatibilityTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.add_nodes(self.num_nodes, versions=[
            200100,  # Last release without unbroadcast serialization and without XOR
            None,
        ])
        self.start_nodes()

    def run_test(self):
        self.log.info("Test that mempool.dat is compatible between versions")

        old_node, new_node = self.nodes
        assert "unbroadcastcount" not in old_node.getmempoolinfo()
        new_wallet = MiniWallet(new_node, mode=MiniWalletMode.RAW_P2PK)
        self.generate(new_wallet, 1, sync_fun=self.no_op)
        self.generate(new_node, COINBASE_MATURITY, sync_fun=self.no_op)
        # Sync the nodes to ensure old_node has the block that contains the coinbase that new_wallet will spend.
        # Otherwise, because coinbases are only valid in a block and not as loose txns, if the nodes aren't synced
        # unbroadcasted_tx won't pass old_node's `MemPoolAccept::PreChecks`.
        self.connect_nodes(0, 1)
        self.sync_blocks()

        self.log.info("Add a transaction to mempool on old node and shutdown")
        old_tx_hash = new_wallet.send_self_transfer(from_node=old_node)["txid"]
        assert old_tx_hash in old_node.getrawmempool()
        self.stop_node(0)
        self.stop_node(1)

        self.log.info("Move mempool.dat from old to new node")
        old_node_mempool = old_node.chain_path / "mempool.dat"
        new_node_mempool = new_node.chain_path / "mempool.dat"
        old_node_mempool.rename(new_node_mempool)

        self.log.info("Start new node and verify mempool contains the tx")
        self.start_node(1, extra_args=["-persistmempoolv1=1"])
        assert old_tx_hash in new_node.getrawmempool()

        self.log.info("Add unbroadcasted tx to mempool on new node and shutdown")
        unbroadcasted_tx_hash = new_wallet.send_self_transfer(from_node=new_node)['txid']
        assert unbroadcasted_tx_hash in new_node.getrawmempool()
        assert new_node.getmempoolentry(unbroadcasted_tx_hash)['unbroadcast']
        self.stop_node(1)

        self.log.info("Move mempool.dat from new to old node")
        new_node_mempool.rename(old_node_mempool)

        self.log.info("Start old node again and verify mempool contains both txs")
        self.start_node(0, ['-nowallet'])
        assert old_tx_hash in old_node.getrawmempool()
        assert unbroadcasted_tx_hash in old_node.getrawmempool()


if __name__ == "__main__":
    MempoolCompatibilityTest(__file__).main()
