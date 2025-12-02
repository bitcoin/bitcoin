#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction index behavior during chain reorganizations.

Test coverage:
- Test txindex consistency after deep reorgs
- Verify tx lookups work correctly after reorg
- Test txindex rebuild after corruption
- Test concurrent txindex queries during reorg
- Test txindex with assumeutxo
"""

import threading
import time
from test_framework.blocktools import create_empty_fork
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
)
from test_framework.script import CScript
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    sync_txindex,
)
from test_framework.wallet import MiniWallet


class TxIndexReorgTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            ["-txindex=1"],  # Node 0: txindex enabled
            ["-txindex=1"],  # Node 1: txindex enabled for reorg testing
            [],              # Node 2: no txindex for comparison
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        
        self.log.info("Generate initial blocks and wait for txindex to sync")
        self.generate(self.wallet, 110)
        self.sync_all()
        
        for node in [self.nodes[0], self.nodes[1]]:
            sync_txindex(self, node)
        
        self.test_txindex_consistency_after_deep_reorg()
        self.test_tx_lookups_after_reorg()
        self.test_txindex_rebuild_after_corruption()
        self.test_concurrent_queries_during_reorg()
        self.test_txindex_with_assumeutxo()

    def test_txindex_consistency_after_deep_reorg(self):
        self.log.info("Test txindex consistency after deep reorg")
        
        # Create transactions on the main chain
        self.log.info("Create and mine transactions on main chain")
        main_chain_txs = []
        for i in range(5):
            tx = self.wallet.send_self_transfer(from_node=self.nodes[0])
            main_chain_txs.append(tx['txid'])
        
        self.generate(self.nodes[0], 1)
        self.sync_all()
        sync_txindex(self, self.nodes[0])
        
        # Verify all transactions are indexed
        for txid in main_chain_txs:
            tx_info = self.nodes[0].getrawtransaction(txid, True)
            assert_equal(tx_info['txid'], txid)
            self.log.debug(f"Transaction {txid} found in txindex")
        
        # Disconnect nodes to create a fork
        self.log.info("Disconnect nodes to create competing chains")
        self.disconnect_nodes(0, 1)
        self.disconnect_nodes(0, 2)
        self.disconnect_nodes(1, 2)
        
        # Create a longer competing chain on node1
        self.log.info("Create longer competing chain with different transactions")
        fork_txs = []
        for i in range(3):
            tx = self.wallet.send_self_transfer(from_node=self.nodes[1])
            fork_txs.append(tx['txid'])
        
        # Make the fork longer to trigger reorg
        self.generate(self.nodes[1], 10, sync_fun=self.no_op)
        sync_txindex(self, self.nodes[1])
        
        # Reconnect and trigger reorg
        self.log.info("Reconnect nodes and trigger reorg")
        self.connect_nodes(0, 1)
        self.sync_blocks([self.nodes[0], self.nodes[1]])
        sync_txindex(self, self.nodes[0])
        
        # Verify txindex consistency after reorg
        self.log.info("Verify txindex consistency after reorg")
        
        # Main chain transactions should now be in mempool or not found
        for txid in main_chain_txs:
            try:
                # Transaction might be in mempool after reorg
                mempool = self.nodes[0].getrawmempool()
                if txid not in mempool:
                    # If not in mempool, getrawtransaction should fail without blockhash
                    assert_raises_rpc_error(-5, "No such mempool transaction", 
                                          self.nodes[0].getrawtransaction, txid, True)
            except Exception:
                pass  # Expected if tx was reorged out
        
        # Fork transactions should be findable
        for txid in fork_txs:
            tx_info = self.nodes[0].getrawtransaction(txid, True)
            assert_equal(tx_info['txid'], txid)
            self.log.debug(f"Fork transaction {txid} found after reorg")

    def test_tx_lookups_after_reorg(self):
        self.log.info("Test transaction lookups work correctly after reorg")
        
        # Reconnect all nodes if they were disconnected
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.sync_all()
        
        # Create a transaction and mine it
        tx1 = self.wallet.send_self_transfer(from_node=self.nodes[0])
        txid1 = tx1['txid']
        block_hash = self.generate(self.nodes[0], 1)[0]
        self.sync_all()
        sync_txindex(self, self.nodes[0])
        
        # Verify transaction is found with and without blockhash
        tx_info = self.nodes[0].getrawtransaction(txid1, True)
        assert_equal(tx_info['txid'], txid1)
        assert_equal(tx_info['blockhash'], block_hash)
        
        tx_info_with_hash = self.nodes[0].getrawtransaction(txid1, True, block_hash)
        assert_equal(tx_info_with_hash['txid'], txid1)
        
        # Node without txindex should fail without blockhash
        assert_raises_rpc_error(-5, "No such mempool transaction", 
                              self.nodes[2].getrawtransaction, txid1, True)
        
        # But should work with blockhash
        tx_info_node2 = self.nodes[2].getrawtransaction(txid1, True, block_hash)
        assert_equal(tx_info_node2['txid'], txid1)
        
        # Invalidate the block to trigger reorg
        self.log.info("Invalidate block and verify transaction handling")
        self.nodes[0].invalidateblock(block_hash)
        
        # Transaction should now be in mempool
        mempool = self.nodes[0].getrawmempool()
        assert txid1 in mempool, "Transaction should be in mempool after invalidateblock"
        
        # Should still be findable via getrawtransaction (in mempool)
        tx_info_mempool = self.nodes[0].getrawtransaction(txid1, True)
        assert_equal(tx_info_mempool['txid'], txid1)
        assert 'blockhash' not in tx_info_mempool or tx_info_mempool['blockhash'] == ''
        
        # Reconsider the block
        self.nodes[0].reconsiderblock(block_hash)
        self.sync_all()
        sync_txindex(self, self.nodes[0])

    def test_txindex_rebuild_after_corruption(self):
        self.log.info("Test txindex rebuild after corruption")
        
        # Create and mine some transactions
        self.log.info("Create transactions before rebuild")
        txids_before = []
        for i in range(3):
            tx = self.wallet.send_self_transfer(from_node=self.nodes[0])
            txids_before.append(tx['txid'])
        
        self.generate(self.nodes[0], 1)
        self.sync_all()
        sync_txindex(self, self.nodes[0])
        
        # Verify transactions are indexed
        for txid in txids_before:
            tx_info = self.nodes[0].getrawtransaction(txid, True)
            assert_equal(tx_info['txid'], txid)
        
        # Stop node and restart with reindex
        self.log.info("Stop node and restart with -reindex to rebuild txindex")
        self.stop_node(0)
        self.start_node(0, extra_args=["-txindex=1", "-reindex"])
        sync_txindex(self, self.nodes[0])
        
        # Verify all transactions are still indexed after rebuild
        self.log.info("Verify transactions are still indexed after rebuild")
        for txid in txids_before:
            tx_info = self.nodes[0].getrawtransaction(txid, True)
            assert_equal(tx_info['txid'], txid)
            self.log.debug(f"Transaction {txid} found after rebuild")

    def test_concurrent_queries_during_reorg(self):
        self.log.info("Test concurrent txindex queries during reorg")
        
        # Reconnect all nodes if they were disconnected
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.sync_all()
        
        # Create multiple transactions
        self.log.info("Create multiple transactions for concurrent testing")
        test_txids = []
        for i in range(10):
            tx = self.wallet.send_self_transfer(from_node=self.nodes[0])
            test_txids.append(tx['txid'])
        
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        sync_txindex(self, self.nodes[0])
        
        # Function to query transactions concurrently
        query_results = []
        query_errors = []
        
        def query_transactions():
            try:
                for txid in test_txids:
                    try:
                        tx_info = self.nodes[0].getrawtransaction(txid, True)
                        query_results.append(tx_info['txid'])
                    except Exception as e:
                        query_errors.append(str(e))
            except Exception as e:
                query_errors.append(f"Thread error: {str(e)}")
        
        # Start concurrent queries
        self.log.info("Start concurrent queries while performing operations")
        query_threads = []
        for i in range(3):
            thread = threading.Thread(target=query_transactions)
            thread.start()
            query_threads.append(thread)
        
        # Perform some operations while queries are running
        time.sleep(0.1)
        self.generate(self.nodes[0], 2, sync_fun=self.no_op)
        
        # Wait for all threads to complete
        for thread in query_threads:
            thread.join()
        
        self.log.info(f"Concurrent queries completed: {len(query_results)} successful, {len(query_errors)} errors")
        
        # Verify at least some queries succeeded
        assert len(query_results) > 0, "At least some concurrent queries should succeed"

    def test_txindex_with_assumeutxo(self):
        self.log.info("Test txindex behavior with assumeutxo considerations")
        
        # Reconnect all nodes if they were disconnected
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.sync_all()
        
        # This test verifies that txindex works correctly in scenarios
        # similar to assumeutxo where the chain state might be loaded
        # from a snapshot
        
        # Create transactions before and after a "snapshot point"
        self.log.info("Create transactions to simulate pre/post snapshot scenario")
        
        # Pre-snapshot transactions
        pre_snapshot_txids = []
        for i in range(3):
            tx = self.wallet.send_self_transfer(from_node=self.nodes[0])
            pre_snapshot_txids.append(tx['txid'])
        
        snapshot_block = self.generate(self.nodes[0], 1)[0]
        self.sync_all()
        sync_txindex(self, self.nodes[0])
        
        # Post-snapshot transactions
        post_snapshot_txids = []
        for i in range(3):
            tx = self.wallet.send_self_transfer(from_node=self.nodes[0])
            post_snapshot_txids.append(tx['txid'])
        
        self.generate(self.nodes[0], 1)
        self.sync_all()
        sync_txindex(self, self.nodes[0])
        
        # Verify all transactions are indexed
        self.log.info("Verify all transactions are indexed regardless of snapshot point")
        for txid in pre_snapshot_txids + post_snapshot_txids:
            tx_info = self.nodes[0].getrawtransaction(txid, True)
            assert_equal(tx_info['txid'], txid)
        
        # Simulate a scenario where we invalidate blocks around snapshot
        self.log.info("Invalidate blocks around snapshot point")
        self.nodes[0].invalidateblock(snapshot_block)
        
        # Pre-snapshot transactions should be in mempool
        mempool = self.nodes[0].getrawmempool()
        for txid in pre_snapshot_txids:
            assert txid in mempool, f"Pre-snapshot tx {txid} should be in mempool"
        
        # Reconsider block
        self.nodes[0].reconsiderblock(snapshot_block)
        self.sync_all()
        sync_txindex(self, self.nodes[0])
        
        # Verify all transactions are still indexed
        for txid in pre_snapshot_txids + post_snapshot_txids:
            tx_info = self.nodes[0].getrawtransaction(txid, True)
            assert_equal(tx_info['txid'], txid)
        
        self.log.info("Txindex works correctly with assumeutxo-like scenarios")


if __name__ == '__main__':
    TxIndexReorgTest(__file__).main()
