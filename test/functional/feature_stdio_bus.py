#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test stdio_bus shadow mode integration.

This test verifies that:
1. -stdiobus=shadow mode can be enabled without affecting consensus
2. Block validation produces identical results with and without stdio_bus
3. Transaction admission produces identical results with and without stdio_bus
4. P2P message handling is unaffected by stdio_bus hooks
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class StdioBusShadowModeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # Node 0: stdio_bus disabled (control)
        # Node 1: stdio_bus shadow mode enabled
        self.extra_args = [
            [],  # Node 0: default (stdio_bus off)
            ["-stdiobus=shadow"],  # Node 1: shadow mode
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Testing stdio_bus shadow mode does not affect consensus")
        
        # Test 1: Both nodes should start successfully
        self.test_nodes_start()
        
        # Test 2: Block generation and sync should work identically
        self.test_block_sync()
        
        # Test 3: Transaction relay should work identically
        self.test_tx_relay()
        
        # Test 4: Chain state should be identical
        self.test_chain_state_identical()
        
        self.log.info("All stdio_bus shadow mode tests passed!")

    def test_nodes_start(self):
        """Verify both nodes started successfully"""
        self.log.info("Verifying nodes started successfully")
        
        for i, node in enumerate(self.nodes):
            info = node.getblockchaininfo()
            self.log.info(f"Node {i}: chain={info['chain']}, blocks={info['blocks']}")
            assert_equal(info['chain'], 'regtest')

    def test_block_sync(self):
        """Test that blocks sync identically between nodes"""
        self.log.info("Testing block synchronization")
        
        # Generate blocks on node 0 (control)
        self.log.info("Generating 10 blocks on node 0 (control)")
        blocks_0 = self.generate(self.nodes[0], 10)
        
        # Sync and verify
        self.sync_all()
        
        # Both nodes should have same tip
        tip_0 = self.nodes[0].getbestblockhash()
        tip_1 = self.nodes[1].getbestblockhash()
        assert_equal(tip_0, tip_1)
        self.log.info(f"Both nodes at tip: {tip_0}")
        
        # Generate blocks on node 1 (shadow mode)
        self.log.info("Generating 10 blocks on node 1 (shadow mode)")
        blocks_1 = self.generate(self.nodes[1], 10)
        
        # Sync and verify
        self.sync_all()
        
        tip_0 = self.nodes[0].getbestblockhash()
        tip_1 = self.nodes[1].getbestblockhash()
        assert_equal(tip_0, tip_1)
        self.log.info(f"Both nodes at tip: {tip_0}")
        
        # Verify block count
        assert_equal(self.nodes[0].getblockcount(), 220)  # 200 cached + 20 new
        assert_equal(self.nodes[1].getblockcount(), 220)

    def test_tx_relay(self):
        """Test that transactions relay identically"""
        self.log.info("Testing transaction relay")
        
        # Get addresses
        addr_0 = self.nodes[0].getnewaddress()
        addr_1 = self.nodes[1].getnewaddress()
        
        # Send from node 0 to node 1
        self.log.info("Sending transaction from node 0 to node 1")
        txid_0 = self.nodes[0].sendtoaddress(addr_1, 1.0)
        
        # Wait for tx to appear in node 1's mempool
        self.sync_mempools()
        
        # Verify tx is in both mempools
        assert txid_0 in self.nodes[0].getrawmempool()
        assert txid_0 in self.nodes[1].getrawmempool()
        self.log.info(f"Transaction {txid_0} in both mempools")
        
        # Send from node 1 (shadow mode) to node 0
        self.log.info("Sending transaction from node 1 (shadow mode) to node 0")
        txid_1 = self.nodes[1].sendtoaddress(addr_0, 1.0)
        
        # Wait for tx to appear in node 0's mempool
        self.sync_mempools()
        
        # Verify tx is in both mempools
        assert txid_1 in self.nodes[0].getrawmempool()
        assert txid_1 in self.nodes[1].getrawmempool()
        self.log.info(f"Transaction {txid_1} in both mempools")
        
        # Mine transactions
        self.generate(self.nodes[0], 1)
        self.sync_all()
        
        # Verify mempools are empty
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)
        self.log.info("Transactions confirmed, mempools empty")

    def test_chain_state_identical(self):
        """Verify chain state is identical on both nodes"""
        self.log.info("Verifying chain state is identical")
        
        # Compare blockchain info
        info_0 = self.nodes[0].getblockchaininfo()
        info_1 = self.nodes[1].getblockchaininfo()
        
        assert_equal(info_0['blocks'], info_1['blocks'])
        assert_equal(info_0['bestblockhash'], info_1['bestblockhash'])
        assert_equal(info_0['chainwork'], info_1['chainwork'])
        
        self.log.info(f"Chain state identical: blocks={info_0['blocks']}, tip={info_0['bestblockhash'][:16]}...")
        
        # Compare UTXO set hash
        utxo_0 = self.nodes[0].gettxoutsetinfo()
        utxo_1 = self.nodes[1].gettxoutsetinfo()
        
        assert_equal(utxo_0['hash_serialized_3'], utxo_1['hash_serialized_3'])
        assert_equal(utxo_0['total_amount'], utxo_1['total_amount'])
        
        self.log.info(f"UTXO set identical: hash={utxo_0['hash_serialized_3'][:16]}...")


class StdioBusModeParsingTest(BitcoinTestFramework):
    """Test -stdiobus argument parsing"""
    
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def run_test(self):
        self.log.info("Testing -stdiobus argument parsing")
        
        # Test valid modes
        self.test_mode("off")
        self.test_mode("shadow")
        
        # Note: "active" mode is defined but not fully implemented yet
        # self.test_mode("active")
        
        self.log.info("All mode parsing tests passed!")

    def test_mode(self, mode):
        """Test that a specific mode works"""
        self.log.info(f"Testing -stdiobus={mode}")
        
        self.restart_node(0, extra_args=[f"-stdiobus={mode}"])
        
        # Node should start successfully
        info = self.nodes[0].getblockchaininfo()
        assert_equal(info['chain'], 'regtest')
        self.log.info(f"Node started successfully with -stdiobus={mode}")


if __name__ == '__main__':
    StdioBusShadowModeTest(__file__).main()
