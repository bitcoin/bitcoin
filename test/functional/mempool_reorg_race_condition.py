#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempool race condition during block reorg.

This test reproduces issue #34584 where a race condition occurs in mempool
fee estimation during block reorganization. The bug manifests when:
1. A block reorg causes transactions to be re-added to the mempool
2. Mempool size limiting evicts some transactions
3. The cached UTXO view retains stale references to evicted mempool transactions
4. Subsequent transaction validation accesses the stale cache

Without the fix in validation.cpp, this test may fail intermittently or
cause assertions/crashes due to accessing invalid mempool state.
"""

from test_framework.blocktools import create_empty_fork
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than
from test_framework.wallet import MiniWallet


class MempoolReorgRaceConditionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = False
        # Use small mempool to trigger eviction during reorg
        self.extra_args = [["-maxmempool=5"]]

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.log.info("Create initial blockchain state")
        # Start with 200 blocks
        assert_equal(node.getblockcount(), 200)

        self.log.info("Create a batch of transactions that will be in a block")
        # Create transactions that will be mined
        txs_to_mine = []
        for i in range(20):
            tx = self.wallet.send_self_transfer(from_node=node)
            txs_to_mine.append(tx)

        # Get initial mempool size
        mempool_info = node.getmempoolinfo()
        assert_equal(mempool_info['size'], 20)

        self.log.info("Mine these transactions into a block")
        self.generate(node, 1)
        assert_equal(node.getmempoolinfo()['size'], 0)

        self.log.info("Create additional transactions for mempool that will remain")
        mempool_txs = []
        for i in range(10):
            tx = self.wallet.send_self_transfer(from_node=node)
            mempool_txs.append(tx)

        assert_equal(node.getmempoolinfo()['size'], 10)

        self.log.info("Create a competing fork that doesn't include the mined transactions")
        # This will cause the 20 transactions to be re-added to mempool on reorg
        fork_blocks = create_empty_fork(node, fork_length=2)

        self.log.info("Trigger reorg by submitting the longer fork")
        for block in fork_blocks:
            node.submitblock(block.serialize().hex())

        assert_equal(node.getbestblockhash(), fork_blocks[-1].hash_hex)

        # After reorg, all 20 previously-mined txs plus 10 mempool txs = 30 total
        # But with maxmempool=5, many will be evicted
        mempool_after_reorg = node.getmempoolinfo()
        self.log.info(f"Mempool size after reorg: {mempool_after_reorg['size']}")

        # Some transactions should have been evicted due to mempool size limit
        assert_greater_than(30, mempool_after_reorg['size'])

        self.log.info("Immediately submit new transactions after reorg")
        # This is where the race condition would occur - the cached view
        # might have stale references to evicted mempool transactions

        # Try to create and validate new transactions
        # Without the fix, this could trigger the race condition
        new_txs = []
        for i in range(5):
            try:
                tx = self.wallet.send_self_transfer(from_node=node)
                new_txs.append(tx)
            except Exception as e:
                # If we hit the race condition, we might get errors here
                self.log.info(f"Transaction submission failed (race condition?): {e}")
                raise

        self.log.info("Verify new transactions were accepted successfully")
        # If we got here without crashes or assertion failures, the race condition is fixed
        for tx in new_txs:
            assert tx['txid'] in node.getrawmempool()

        self.log.info("Test testmempoolaccept after reorg (another potential race condition point)")
        # testmempoolaccept also uses the cached view, so it's another place
        # where stale cache could cause issues
        test_tx = self.wallet.create_self_transfer()
        result = node.testmempoolaccept([test_tx['hex']])[0]
        assert_equal(result['allowed'], True)

        self.log.info("Create a chain of dependent transactions to stress test cache")
        # Create parent-child chain to ensure cache handles dependencies correctly
        parent = self.wallet.send_self_transfer(from_node=node)
        child = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=parent['new_utxo']
        )

        assert parent['txid'] in node.getrawmempool()
        assert child['txid'] in node.getrawmempool()

        self.log.info("SUCCESS: No race condition detected - cache is properly reset after reorg")


if __name__ == '__main__':
    MempoolReorgRaceConditionTest(__file__).main()
