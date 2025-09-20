#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test dumptxoutset rollback with competing forks.

Test that dumptxoutset rollback functionality works correctly when competing
forks exist at or after the target rollback height.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class DumptxoutsetForksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.setup_nodes()
        # Don't connect nodes initially

    def run_test(self):
        """Test that dumptxoutset rollback fails when competing forks exist."""
        node0, node1 = self.nodes[0], self.nodes[1]
        
        # Set consistent mock time for deterministic results
        mocktime = node0.getblockheader(node0.getblockhash(0))['time'] + 1
        for node in self.nodes:
            node.setmocktime(mocktime)
        
        self.log.info("Create common blockchain to height 15")
        self.connect_nodes(0, 1)
        self.generate(node0, 15, sync_fun=self.sync_all)
        
        assert_equal(node0.getblockcount(), 15)
        assert_equal(node1.getblockcount(), 15)
        
        target_height = 15
        target_hash = node0.getblockhash(target_height)
        
        self.log.info("Test baseline - dumptxoutset should work before forks exist")
        baseline_result = node0.dumptxoutset("baseline_utxo.dat", rollback=target_height)
        assert_equal(baseline_result['base_height'], target_height)
        assert_equal(baseline_result['base_hash'], target_hash)
        
        self.log.info("Disconnect nodes and create competing forks")
        self.disconnect_nodes(0, 1)
        
        # Node0: Extend main chain (longer)
        self.generate(node0, 3, sync_fun=lambda: None)  # Height 18
        
        # Node1: Create competing fork (shorter)
        self.generate(node1, 2, sync_fun=lambda: None)  # Height 17
        
        assert_equal(node0.getblockcount(), 18)
        assert_equal(node1.getblockcount(), 17)
        
        self.log.info("Transfer competing fork to main node")
        # Make node0 aware of the competing fork by submitting its blocks
        for height in range(16, 18):  # Heights 16-17 from the fork
            fork_hash = node1.getblockhash(height)
            fork_data = node1.getblock(fork_hash, 0)  # Raw block data
            node0.submitblock(fork_data)
        
        # Verify fork visibility
        chaintips = node0.getchaintips()
        assert len(chaintips) >= 2, f"Expected at least 2 chain tips, got {len(chaintips)}"
        
        # Find the active chain and competing fork
        active_tip = [tip for tip in chaintips if tip['status'] == 'active'][0]
        fork_tips = [tip for tip in chaintips if tip.get('branchlen', 0) > 0]
        assert len(fork_tips) >= 1, "Expected at least one competing fork"
        
        assert_equal(active_tip['height'], 18)
        
        self.log.info("Test that dumptxoutset rollback fails with competing fork present")
        # Record original state
        original_tip = node0.getbestblockhash()
        original_height = node0.getblockcount()
        
        # This should fail due to competing fork interference
        assert_raises_rpc_error(
            -1, 
            "Could not roll back to requested height", 
            node0.dumptxoutset, 
            "fork_test_utxo.dat", 
            rollback=target_height
        )
        
        # Verify node state is restored after failed attempt
        current_tip = node0.getbestblockhash()
        current_height = node0.getblockcount()
        assert_equal(current_tip, original_tip)
        assert_equal(current_height, original_height)


if __name__ == '__main__':
    DumptxoutsetForksTest(__file__).main() 