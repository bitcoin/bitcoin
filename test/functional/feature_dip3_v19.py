#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Syscoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_dip3_v19.py

Checks DIP3 for v19 and validates that old MNs work with new BLS scheme
'''
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    force_finish_mnsync
)

import time

class DIP3V19Test(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, extra_args=[['-dip19params=350']] * 6, fast_dip3_enforcement=True, default_legacy_bls=True)
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        
    def check_chainlocks_with_distance(self, blocks, distance=5):
        """Check for chainlocks on blocks that are 'distance' blocks behind the tip"""
        if not blocks or len(blocks) <= distance:
            self.log.info("Not enough blocks to check for chainlocks")
            return False
            
        try:
            block_to_check = blocks[-distance]  # Check the block 'distance' back from the tip
            self.log.info(f"Checking for chainlock on block {block_to_check}")
            
            # Wait for chainlock to appear
            start_time = time.time()
            max_wait = 10  # seconds
            while time.time() - start_time < max_wait:
                chainlocks = self.nodes[0].getchainlocks()
                if "active_chainlock" in chainlocks and chainlocks["active_chainlock"] is not None:
                    if chainlocks["active_chainlock"]["blockhash"] == block_to_check:
                        self.log.info(f"Successfully verified chainlock on block {block_to_check}")
                        return True
                time.sleep(0.5)
                
            self.log.info(f"No chainlock found for block {block_to_check} after waiting {max_wait} seconds")
            return False
        except Exception as e:
            self.log.info(f"Error checking for chainlocks: {str(e)}")
            return False

    def wait_for_chainlocks_active(self, timeout=30):
        """Wait for chainlocks to activate"""
        self.log.info("Waiting for chainlocks to activate...")
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                clstatus = self.nodes[0].getchainlocks()
                if 'active_chainlock' in clstatus and clstatus['active_chainlock'] is not None:
                    self.log.info("Chainlocks successfully activated")
                    return True
            except Exception as e:
                self.log.info(f"Error getting chainlocks: {str(e)}")
            time.sleep(1)
            
        self.log.info("Failed to activate chainlocks within timeout")
        return False

    def run_test(self):
        # Setup and initial checks
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
        self.wait_for_sporks_same()
        
        # Store masternode info before v19 for comparison
        original_mn_list = self.nodes[0].masternode_list()
        self.log.info(f"Original masternode count: {len(original_mn_list)}")
        
        # Create pre-v19 quorums (needed for chainlocks)
        self.log.info("Creating pre-v19 quorums")
        self.log.info("Mining 4 quorums")
        for i in range(4):
            self.mine_quorum()
        self.wait_for_sporks_same()
        
        # Verify chainlocks are working pre-v19
        self.log.info("Verifying chainlocks pre-v19")
        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl)
 
        # Pre-v19: Test masternode service update
        mn = self.mninfo[0]
        self.log.info(f"Testing MN service update pre-v19 for {mn.proTxHash}")
        orig_service = self.nodes[0].protx_info(mn.proTxHash)['state']['service']
        
        # Update service using the operator private key
        self.nodes[0].sendtoaddress(mn.collateral_address, 0.001)
        self.nodes[0].protx_update_service(
            mn.proTxHash, 
            '127.0.0.2:1000', 
            mn.keyOperator,
            "", 
            "", 
            mn.collateral_address,
            True  # Use legacy=True pre-v19
        )
        self.generate(self.nodes[0], 1)
        
        # Verify service update worked
        updated_service = self.nodes[0].protx_info(mn.proTxHash)['state']['service']
        assert updated_service == '127.0.0.2:1000', f"Service update failed: {updated_service} != 127.0.0.2:1000"
        
        self.log.info(f"Pre-v19 service update successful: {orig_service} -> {updated_service}")
        
        # Move to v19 activation height
        current_height = self.nodes[0].getblockcount()
        v19_height = 351
        if current_height < v19_height:
            blocks_to_mine = v19_height - current_height
            self.log.info(f"Mining {blocks_to_mine} blocks to reach v19 activation height ({v19_height})")
            self.generate(self.nodes[0], blocks_to_mine)
        
        self.log.info(f"v19 activated at height: {self.nodes[0].getblockcount()}")
        # post-V19 cannot use legacy
        assert_raises_rpc_error(-4, 'bad-protx-bls-sig', self.nodes[0].protx_update_service,  mn.proTxHash, orig_service, mn.keyOperator, "", "", mn.collateral_address, True)
        self.nodes[0].protx_update_service(
            mn.proTxHash, 
            orig_service, 
            mn.keyOperator,
            "", 
            "", 
            mn.collateral_address,
        )
        self.generate(self.nodes[0], 1)
        updated_service1 = self.nodes[0].protx_info(mn.proTxHash)['state']['service']
        assert updated_service1 == orig_service, f"Service update failed: {updated_service1} != {orig_service}"
        
        self.log.info(f"Post-v19 service update successful: {updated_service} -> {orig_service}")
        # Create post-v19 quorums
        self.log.info("Creating post-v19 quorums")
        self.log.info("Mining 4 quorums")
        for i in range(4):
            self.mine_quorum()
        self.bump_mocktime(30)  # Give time for quorum processing
        
        # Mine blocks to ensure chainlocks
        self.log.info("Mining additional blocks to ensure chainlocks post-v19")
        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl)
        self.log.info("v19 BLS scheme transition test successful - all critical operations verified!")


if __name__ == '__main__':
    DIP3V19Test().main()
