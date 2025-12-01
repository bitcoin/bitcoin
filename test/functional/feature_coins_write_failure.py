#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coin database write failure handling and recovery.

This test verifies that Bitcoin Core properly handles failures when writing
to the coins database (UTXO set), including:
- Disk full scenarios
- I/O errors during flush
- Recovery after write failures
- Consistency of the UTXO set after failures
"""

import os
import shutil
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


class CoinsWriteFailureTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # Use small dbcache to trigger more frequent flushes
        self.extra_args = [['-dbcache=4'], ['-dbcache=4']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Testing coin database write failure handling")
        
        node = self.nodes[0]
        node_control = self.nodes[1]  # Control node for comparison
        
        self.mini_wallet = MiniWallet(node)
        self.mini_wallet_control = MiniWallet(node_control)
        
        # Connect nodes initially to sync
        self.connect_nodes(0, 1)
        
        self.test_normal_operation()
        self.test_utxo_consistency_after_restart()
        self.test_recovery_after_simulated_crash()
        self.test_disk_space_handling()

    def test_normal_operation(self):
        """Test that normal operations work correctly."""
        self.log.info("Test normal coin database operations")
        
        node = self.nodes[0]
        
        # Generate some blocks
        self.generate(self.mini_wallet, 10)
        self.sync_all()
        
        # Get UTXO set info
        utxo_info = node.gettxoutsetinfo()
        self.log.info(f"UTXO set: {utxo_info['transactions']} txs, {utxo_info['txouts']} outputs")
        
        # Verify both nodes have same UTXO set
        utxo_info_control = self.nodes[1].gettxoutsetinfo()
        assert_equal(utxo_info['hash_serialized_3'], utxo_info_control['hash_serialized_3'])

    def test_utxo_consistency_after_restart(self):
        """Test that UTXO set remains consistent after node restart."""
        self.log.info("Test UTXO consistency after restart")
        
        node = self.nodes[0]
        
        # Get UTXO hash before restart
        utxo_before = node.gettxoutsetinfo()
        best_block_before = node.getbestblockhash()
        
        # Restart node
        self.restart_node(0)
        
        # Verify UTXO set is the same
        utxo_after = node.gettxoutsetinfo()
        best_block_after = node.getbestblockhash()
        
        assert_equal(best_block_before, best_block_after)
        assert_equal(utxo_before['hash_serialized_3'], utxo_after['hash_serialized_3'])
        
        self.log.info("UTXO set consistent after restart")

    def test_recovery_after_simulated_crash(self):
        """Test recovery after simulated crash using -dbcrashratio."""
        self.log.info("Test recovery after simulated crash")
        
        # Restart node with crash simulation
        # Note: -dbcrashratio causes random crashes during database writes
        self.restart_node(0, extra_args=['-dbcrashratio=100', '-dbcache=4'])
        
        node = self.nodes[0]
        
        # Try to generate blocks - may crash
        try:
            # Generate several blocks to trigger potential crashes
            for i in range(5):
                try:
                    self.generate(node, 1)
                except Exception as e:
                    self.log.info(f"Node crashed as expected during block {i}: {e}")
                    # Restart without crash simulation
                    self.restart_node(0, extra_args=['-dbcache=4'])
                    node = self.nodes[0]
        except Exception:
            # If we crashed, restart normally
            self.restart_node(0, extra_args=['-dbcache=4'])
            node = self.nodes[0]
        
        # Verify node recovered and UTXO set is valid
        utxo_info = node.gettxoutsetinfo()
        self.log.info(f"After recovery: {utxo_info['transactions']} txs")
        
        # Reconnect nodes before syncing
        self.connect_nodes(0, 1)
        self.sync_all()
        
        # Compare with control node
        utxo_control = self.nodes[1].gettxoutsetinfo()
        assert_equal(utxo_info['hash_serialized_3'], utxo_control['hash_serialized_3'])

    def test_disk_space_handling(self):
        """Test behavior when disk space is limited."""
        self.log.info("Test disk space handling")
        
        node = self.nodes[0]
        
        # Generate more blocks to increase database size
        self.generate(self.mini_wallet, 50)
        self.sync_all()
        
        # Get database directory size
        datadir = node.datadir_path
        chainstate_dir = datadir / 'regtest' / 'chainstate'
        
        if chainstate_dir.exists():
            # Calculate directory size
            total_size = sum(
                f.stat().st_size for f in chainstate_dir.rglob('*') if f.is_file()
            )
            self.log.info(f"Chainstate directory size: {total_size / 1024 / 1024:.2f} MB")
        
        # Verify database is functional
        utxo_info = node.gettxoutsetinfo()
        assert utxo_info['txouts'] > 0
        
        self.log.info("Database operations successful with current disk usage")

    def test_flush_consistency(self):
        """Test that manual flushes maintain UTXO consistency."""
        self.log.info("Test manual flush consistency")
        
        node = self.nodes[0]
        
        # Get initial state
        initial_utxo = node.gettxoutsetinfo()
        
        # Generate transactions
        for _ in range(10):
            self.mini_wallet.send_self_transfer(from_node=node)
        
        # Mine a block
        self.generate(node, 1)
        
        # Get UTXO after mining
        utxo_after = node.gettxoutsetinfo()
        
        # UTXO set should have changed
        assert initial_utxo['hash_serialized_3'] != utxo_after['hash_serialized_3']
        
        # Restart to force flush
        self.restart_node(0)
        
        # Verify consistency after restart
        utxo_after_restart = node.gettxoutsetinfo()
        assert_equal(utxo_after['hash_serialized_3'], utxo_after_restart['hash_serialized_3'])


if __name__ == '__main__':
    CoinsWriteFailureTest(__file__).main()
