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

    def create_common_chain(self):
        """Create a common blockchain that both nodes will share."""
        self.log.info("Creating common blockchain to height 15")
        self.connect_nodes(0, 1)
        self.generate(self.nodes[0], 15, sync_fun=self.sync_all)
        
        assert_equal(self.nodes[0].getblockcount(), 15)
        assert_equal(self.nodes[1].getblockcount(), 15)
        
        return 15, self.nodes[0].getblockhash(15)

    def test_baseline_functionality(self, target_height, target_hash):
        """Test that dumptxoutset works before forks exist."""
        self.log.info("Testing baseline dumptxoutset functionality")
        baseline_result = self.nodes[0].dumptxoutset("baseline_utxo.dat", rollback=target_height)
        assert_equal(baseline_result['base_height'], target_height)
        assert_equal(baseline_result['base_hash'], target_hash)

    def create_competing_forks(self):
        """Create competing forks by disconnecting nodes and mining separately."""
        self.log.info("Disconnecting nodes and creating competing forks")
        self.disconnect_nodes(0, 1)
        
        self.generate(self.nodes[0], 3, sync_fun=lambda: None)

        self.generate(self.nodes[1], 2, sync_fun=lambda: None)
        
        assert_equal(self.nodes[0].getblockcount(), 18)
        assert_equal(self.nodes[1].getblockcount(), 17)

    def transfer_fork_to_main_node(self):
        """Make the main node aware of the competing fork."""
        self.log.info("Transferring competing fork blocks to main node")
        for height in range(16, 18): 
            fork_hash = self.nodes[1].getblockhash(height)
            fork_data = self.nodes[1].getblock(fork_hash, 0)
            self.nodes[0].submitblock(fork_data)

    def verify_fork_visibility(self):
        """Verify that the main node can see both chains."""
        self.log.info("Verifying fork visibility")
        chaintips = self.nodes[0].getchaintips()
        assert len(chaintips) >= 2, f"Expected at least 2 chain tips, got {len(chaintips)}"
        
        active_tip = [tip for tip in chaintips if tip['status'] == 'active'][0]
        fork_tips = [tip for tip in chaintips if tip.get('branchlen', 0) > 0]
        assert len(fork_tips) >= 1, "Expected at least one competing fork"
        
        assert_equal(active_tip['height'], 18)
        return active_tip, fork_tips

    def test_rollback_with_forks(self, target_height, target_hash):
        """Test that dumptxoutset rollback works correctly even when competing forks are present."""
        self.log.info("Testing dumptxoutset rollback with competing forks present")
        
        original_tip = self.nodes[0].getbestblockhash()
        original_height = self.nodes[0].getblockcount()
        
        # This should now work correctly with our fix
        result = self.nodes[0].dumptxoutset("fork_test_utxo.dat", rollback=target_height)
        
        # Verify the snapshot was created from the correct block on the main chain
        assert_equal(result['base_height'], target_height)
        assert_equal(result['base_hash'], target_hash)
        
        # Verify node state is restored after successful rollback
        current_tip = self.nodes[0].getbestblockhash()
        current_height = self.nodes[0].getblockcount()
        assert_equal(current_tip, original_tip)
        assert_equal(current_height, original_height)

    def run_test(self):
        """Main test logic"""
        mocktime = self.nodes[0].getblockheader(self.nodes[0].getblockhash(0))['time'] + 1
        for node in self.nodes:
            node.setmocktime(mocktime)

        target_height, target_hash = self.create_common_chain()
        self.test_baseline_functionality(target_height, target_hash)

        self.create_competing_forks()
        self.transfer_fork_to_main_node()
        self.verify_fork_visibility()

        # Test the main functionality
        self.test_rollback_with_forks(target_height, target_hash)


if __name__ == '__main__':
    DumptxoutsetForksTest(__file__).main() 