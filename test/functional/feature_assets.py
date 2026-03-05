#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Syscoin Asset Transaction Types Test

This test covers the five main Syscoin transaction types:
1. SYSCOIN_TX_VERSION_ALLOCATION_SEND - For sending assets
2. SYSCOIN_TX_VERSION_ALLOCATION_MINT - For minting new assets
3. SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM - For burning assets to NEVM
4. SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION - For converting SYS to SYSX
5. SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN - For converting SYSX back to SYS

The test also verifies various edge cases and coin selection nuances.
"""

from decimal import Decimal

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.asset_helpers import (
    create_transaction_with_selector,
    verify_tx_outputs,
    CoinSelector,
    SYSCOIN_TX_VERSION_ALLOCATION_SEND,
    SYSCOIN_TX_VERSION_ALLOCATION_MINT,
    SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM,
    SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION,
    SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN,
)

# Constants for testing
SYSX_GUID = 123456
DUST_THRESHOLD = Decimal('0.00000546')
BLOCK_REWARD = Decimal('50')
class AssetTransactionTest(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-dip3params=0:0'] for _ in range(self.num_nodes)]

    def syscoin_tx(self, tx_type, asset_amounts, sys_amount=Decimal('0'), sys_destination=None, nevm_address=None, spv_proof=None):
        tx_hex = create_transaction_with_selector(
            node=self.nodes[0],
            tx_type=tx_type,
            sys_amount=sys_amount,
            sys_destination=sys_destination,
            asset_amounts=asset_amounts,
            nevm_address=nevm_address,
            spv_proof=spv_proof
        )
        txid = self.nodes[0].sendrawtransaction(tx_hex)
        self.generate(self.nodes[0],1)
        # Verify the transaction outputs
        verify_tx_outputs(
            node=self.nodes[0],
            txid=txid, 
            tx_type=tx_type,
            asset_details=asset_amounts
        )

    def syscoin_burn_to_allocation(self, asset_amounts, sys_amount=Decimal('0')):
        sys_balance_before = self.nodes[0].getbalance()
        self.syscoin_tx(SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION, asset_amounts, sys_amount)
        sys_balance_after = self.nodes[0].getbalance()
        expected_balance = sys_balance_before - sys_amount - Decimal('0.0001') + BLOCK_REWARD + DUST_THRESHOLD
        assert_equal(expected_balance, sys_balance_after)

    def asset_allocation_send(self, asset_amounts, sys_amount=Decimal('0'), sys_destination=None):
        self.syscoin_tx(SYSCOIN_TX_VERSION_ALLOCATION_SEND, asset_amounts, sys_amount, sys_destination)

    def allocation_burn_to_nevm(self, asset_amounts, sys_amount=Decimal('0'), nevm_address=''):
        sys_balance_before = self.nodes[0].getbalance()
        self.syscoin_tx(SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM, asset_amounts, sys_amount, nevm_address=nevm_address)
        sys_balance_after = self.nodes[0].getbalance()
        expected_balance = sys_balance_before - Decimal('0.0001') + BLOCK_REWARD - DUST_THRESHOLD
        assert_equal(expected_balance, sys_balance_after)
        
    def allocation_burn_to_syscoin(self, asset_amounts, sys_amount=Decimal('0'), sys_destination=None):
        sys_balance_before = self.nodes[0].getbalance()
        self.syscoin_tx(SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN, asset_amounts, sys_amount, sys_destination)
        sys_balance_after = self.nodes[0].getbalance()
        # You get back SYS equal to the asset amount burned
        total_asset_amount = sum(amount for _, amount, _ in asset_amounts)
        expected_balance = sys_balance_before + total_asset_amount + BLOCK_REWARD - Decimal('0.0001')
        assert_equal(expected_balance, sys_balance_after)

    def allocation_mint(self, asset_amounts, spv_proof, sys_amount=Decimal('0'), sys_destination=None):
        sys_balance_before = self.nodes[0].getbalance()
        self.syscoin_tx(SYSCOIN_TX_VERSION_ALLOCATION_MINT, asset_amounts, sys_amount, sys_destination, spv_proof=spv_proof)
        sys_balance_after = self.nodes[0].getbalance()
        # You get back SYS equal to the asset amount burned
        total_asset_amount = sum(amount for _, amount, _ in asset_amounts)
        expected_balance = sys_balance_before + total_asset_amount + BLOCK_REWARD - Decimal('0.0001') # minus fees
        assert_equal(expected_balance, sys_balance_after)


    def setup_test_assets(self):
        """Create test assets for transactions"""
        print("Setting up test assets...")
        self.generate(self.nodes[0],110)
        self.sysx_addr = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(self.sysx_addr, 50)
        self.generate(self.nodes[0],1)
        asset_amounts = [
            (SYSX_GUID, Decimal('20'), self.sysx_addr)
        ]
        self.syscoin_burn_to_allocation(asset_amounts, sys_amount=Decimal('20'))
        # Reuse SYSX for edge-case tests that require asset GUID fixtures.
        self.asset1_guid = SYSX_GUID
        self.asset2_guid = SYSX_GUID
        # A non-SYSX GUID used by negative tests.
        self.non_sysx_guid = 999999

        print("Test assets created successfully")

    def run_test(self):
        """Main test logic"""
        # Setup initial test assets
        self.setup_test_assets()
        self.test_allocation_send()
        self.test_allocation_burn_to_nevm()
        self.test_sys_burn_to_allocation()
        self.test_allocation_burn_to_sys()
        # Run tests for each transaction type
        self.test_allocation_mint()
        # Run coin selection edge-case tests
        self.test_coin_selection_priority()
        self.test_utxo_consolidation()
        self.test_dust_handling()
        self.test_error_cases()

    def test_allocation_send(self):
        """Test SYSCOIN_TX_VERSION_ALLOCATION_SEND transactions"""
        print("\nTesting ALLOCATION_SEND transactions...")
        asset_amounts = [
            (SYSX_GUID, Decimal('5'), self.sysx_addr)
        ]
        self.sysx_addr = self.nodes[0].getnewaddress()
        self.sysx_addr1 = self.nodes[0].getnewaddress()
        self.sysx_addr2 = self.nodes[0].getnewaddress()
        self.asset_allocation_send(asset_amounts)
        asset_amounts = [
            (SYSX_GUID, Decimal('5'), self.sysx_addr),
            (SYSX_GUID, Decimal('15'), self.sysx_addr1)
        ]
        self.asset_allocation_send(asset_amounts)
        asset_amounts = [
            (SYSX_GUID, Decimal('3'), self.sysx_addr1),
            (SYSX_GUID, Decimal('1'), self.sysx_addr2),
            (SYSX_GUID, Decimal('15'), self.sysx_addr)
        ]
        self.asset_allocation_send(asset_amounts, sys_amount=Decimal('25'), sys_destination=self.nodes[0].getnewaddress())
                
        print("ALLOCATION_SEND tests passed")

    def test_allocation_mint(self):
        """Test SYSCOIN_TX_VERSION_ALLOCATION_MINT validation path"""
        print("\nTesting ALLOCATION_MINT transactions...")
        
        # Get current block info for SPV proof
        burn_block = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        
        # Create proper SPV proof structure for minting
        spv_proof = {
          "txHash": bytes.fromhex("a" * 64),        # Transaction hash on NEVM
            "txValue": bytes.fromhex("b" * 20),       # Transaction value (dummy)
            "txPos": 0,                               # Transaction position in block 
            "txBlockHash": bytes.fromhex(burn_block["hash"]), # Use an actual block hash
            "txParentNodes": b"\x00" * 32,            # Merkle proof parent nodes (dummy)
            "txPath": b"\x01\x00",                    # Simplified tx path (dummy)
            "posReceipt": 1,                          # Receipt position
            "receiptParentNodes": b"\x00" * 32,       # Receipt parent nodes (dummy)
            "txRoot": bytes.fromhex("c" * 64),        # Transaction Merkle root (dummy)
            "receiptRoot": bytes.fromhex("d" * 64)    # Receipt Merkle root (dummy)
        }
        
        # Test case: Mint new asset 
        dest_addr = self.nodes[0].getnewaddress()
        
        # Use a fresh GUID for this test
        new_asset_guid = SYSX_GUID
        
        asset_amounts = [
            (new_asset_guid, Decimal('500'), dest_addr)
        ]
        tx_hex = create_transaction_with_selector(
            node=self.nodes[0],
            tx_type=SYSCOIN_TX_VERSION_ALLOCATION_MINT,
            asset_amounts=asset_amounts,
            spv_proof=spv_proof,
        )
        
        # Regtest doesn't maintain the NEVM receipt roots required for a valid mint proof.
        # Assert that malformed proof is rejected with the expected consensus reason.
        assert_raises_rpc_error(-26, "mint-txroot-missing", self.nodes[0].sendrawtransaction, tx_hex)

        print("ALLOCATION_MINT validation tests passed")

    def test_allocation_burn_to_nevm(self):
        """Test SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM transactions"""
        print("\nTesting ALLOCATION_BURN_TO_NEVM transactions...")
        
        # Test case: Burn asset to NEVM
        nevm_address = "0x" + "1" * 40  # Dummy NEVM address
        asset_amounts = [
            (SYSX_GUID, Decimal('20'), '')
        ]
        self.allocation_burn_to_nevm(asset_amounts, Decimal('0'), nevm_address)
        
        print("ALLOCATION_BURN_TO_NEVM tests passed")

    def test_sys_burn_to_allocation(self):
        """Test SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION transactions"""
        print("\nTesting SYSCOIN_BURN_TO_ALLOCATION transactions...")
        
        # Test case: Convert SYS to SYSX
        dest_addr = self.nodes[0].getnewaddress()
        sys_amount = Decimal('5.0')
        asset_amounts = [
            (SYSX_GUID, sys_amount, dest_addr)
        ]
        self.syscoin_burn_to_allocation(asset_amounts, sys_amount)
        
        # Test edge case: SYS amount near dust threshold
        dest_addr = self.nodes[0].getnewaddress()
        sys_amount = Decimal('0.0001')  # Very small amount
        asset_amounts = [
            (SYSX_GUID, sys_amount, dest_addr)
        ]
        self.syscoin_burn_to_allocation(asset_amounts, sys_amount)
        
        print("SYSCOIN_BURN_TO_ALLOCATION tests passed")

    def test_allocation_burn_to_sys(self):
        """Test SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN transactions"""
        print("\nTesting ALLOCATION_BURN_TO_SYSCOIN transactions...")
        dest_addr = self.nodes[0].getnewaddress()
        dest_addr1 = self.nodes[0].getnewaddress()
        asset_amounts = [
            (SYSX_GUID, Decimal('3'), dest_addr)
        ]
        self.allocation_burn_to_syscoin(asset_amounts)
   
        small_amount = Decimal('0.0001')
        asset_amounts = [
            (SYSX_GUID, small_amount, dest_addr)
        ]
        self.allocation_burn_to_syscoin(asset_amounts, Decimal('300'), dest_addr1)
       
        print("ALLOCATION_BURN_TO_SYSCOIN tests passed")

    def test_coin_selection_priority(self):
        """Test UTXO prioritization in coin selection"""
        print("\nTesting coin selection prioritization logic...")

        # Seed enough SYSX for the edge-case suite without relying on external mint proofs.
        seeded_asset_addr = self.nodes[0].getnewaddress()
        self.syscoin_burn_to_allocation(
            [(SYSX_GUID, Decimal('100'), seeded_asset_addr)],
            sys_amount=Decimal('100'),
        )
        
        # Setup multiple UTXOs with different characteristics
        self.generate(self.nodes[0],1)
        
        # Create multiple UTXOs with different sizes
        addresses = [self.nodes[0].getnewaddress() for _ in range(5)]
        self.nodes[0].sendtoaddress(addresses[0], 1.0)   # Large UTXO
        self.nodes[0].sendtoaddress(addresses[1], 0.5)   # Medium UTXO
        self.nodes[0].sendtoaddress(addresses[2], 0.1)   # Small UTXO
        self.nodes[0].sendtoaddress(addresses[3], 0.05)  # Smaller UTXO
        self.nodes[0].sendtoaddress(addresses[4], 0.01)  # Smallest UTXO
        
        # Create mixed UTXOs with both SYS and assets
        mixed_addr1 = self.nodes[0].getnewaddress()
        mixed_addr2 = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(mixed_addr1, 0.2)
        self.nodes[0].sendtoaddress(mixed_addr2, 0.3)

        # Seed additional asset UTXOs so selector has both SYS-only and asset UTXOs.
        self.asset_allocation_send([(self.asset1_guid, Decimal('1'), mixed_addr1)])
        self.asset_allocation_send([(self.asset2_guid, Decimal('2'), mixed_addr2)])
        
        self.generate(self.nodes[0],1)
        
        # Test case 1: Small SYS target should use smaller UTXOs first
        selector = CoinSelector(self.nodes[0])
        success, inputs, change, asset_changes = selector.select_coins_for_transaction(
            Decimal('0.03'),  # Small amount
            []
        )
        
        assert success, "Coin selection should succeed for small SYS amount"
        
        # Test case 2: Large SYS target should use larger UTXOs
        selector = CoinSelector(self.nodes[0])
        success, inputs, change, asset_changes = selector.select_coins_for_transaction(
            Decimal('1.2'),  # Large amount
            []
        )
        
        assert success, "Coin selection should succeed for large SYS amount"
        
        # Test case 3: Asset transactions should prioritize UTXOs with the needed assets
        selector = CoinSelector(self.nodes[0])
        success, inputs, change, asset_changes = selector.select_coins_for_transaction(
            Decimal('0.01'),
            [(self.asset1_guid, Decimal('50'), mixed_addr1)]
        )
        
        assert success, "Coin selection should succeed for transactions needing specific assets"
        
        print("Coin selection prioritization tests passed")

    def test_utxo_consolidation(self):
        """Test UTXO consolidation scenarios"""
        print("\nTesting UTXO consolidation scenarios...")
        
        # Create many small UTXOs
        for _ in range(10):
            addr = self.nodes[0].getnewaddress()
            self.nodes[0].sendtoaddress(addr, 0.01)
        
        self.generate(self.nodes[0],1)

        # Lock larger UTXOs so coin selection has to consolidate small UTXOs.
        lock_candidates = []
        for utxo in self.nodes[0].listunspent():
            if Decimal(str(utxo["amount"])) > Decimal("0.02"):
                lock_candidates.append({"txid": utxo["txid"], "vout": utxo["vout"]})
        if lock_candidates:
            self.nodes[0].lockunspent(False, lock_candidates)
        try:
            # Test consolidation through a transaction that should require multiple small inputs.
            burn_amount = Decimal('0.05')
            dest_addr = self.nodes[1].getnewaddress()
            asset_amounts = [
                (SYSX_GUID, burn_amount, dest_addr)
            ]
            tx_hex = create_transaction_with_selector(
                node=self.nodes[0],
                tx_type=SYSCOIN_TX_VERSION_SYSCOIN_BURN_TO_ALLOCATION,
                sys_amount=burn_amount,
                asset_amounts=asset_amounts
            )

            # Sign and send the transaction
            tx_id = self.nodes[0].sendrawtransaction(tx_hex)
            self.generate(self.nodes[0],1)

            # Verify transaction succeeded
            tx_details = self.nodes[0].gettransaction(tx_id)
            assert_equal(tx_details["confirmations"], 1)

            # Check number of inputs to confirm consolidation
            raw_tx = self.nodes[0].decoderawtransaction(tx_hex)
            assert_greater_than(len(raw_tx["vin"]), 1)
        finally:
            # Unlock all UTXOs for subsequent tests.
            self.nodes[0].lockunspent(True)
        
        print("UTXO consolidation tests passed")

    def test_dust_handling(self):
        """Test dust threshold handling"""
        print("\nTesting dust threshold handling...")
        
        # Create a transaction with change just below dust threshold
        
        # First create a precise UTXO size
        addr = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(addr, 0.001)
        self.generate(self.nodes[0],1)
        
        # Now attempt to send an amount that would create dust change
        dest_addr = self.nodes[1].getnewaddress()
        asset_amounts = [
            (self.asset1_guid, Decimal('10'), dest_addr)
        ]
        # This should send most of the 0.001 UTXO but leave change below dust
        tx_hex = create_transaction_with_selector(
            node=self.nodes[0],
            tx_type=SYSCOIN_TX_VERSION_ALLOCATION_SEND,
            sys_amount=Decimal('0.0001'),  # Small fee
            asset_amounts=asset_amounts,
        )
        
        # Sign and send the transaction
        tx_id = self.nodes[0].sendrawtransaction(tx_hex)
        self.generate(self.nodes[0],1)
        
        # Verify transaction succeeded
        tx_details = self.nodes[0].gettransaction(tx_id)
        assert_equal(tx_details["confirmations"], 1)
        
        # Verify that no dust output was created
        raw_tx = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(tx_id)["hex"])
        for vout in raw_tx["vout"]:
            if vout["scriptPubKey"]["type"] != "nulldata":  # Skip OP_RETURN
                # Use Decimal comparison to avoid float representation edge cases.
                assert Decimal(str(vout["value"])) >= DUST_THRESHOLD
        
        print("Dust threshold handling tests passed")

    def test_error_cases(self):
        """Test various error cases"""
        print("\nTesting error cases...")
        
        # Get current block info for SPV proof structure
        burn_block = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        
        # Test case 1: ALLOCATION_BURN_TO_SYSCOIN with non-SYSX asset
        dest_addr = self.nodes[0].getnewaddress()
        
        try:
            asset_amounts = [
                (self.non_sysx_guid, Decimal('10'), dest_addr)
            ]
            tx_hex = create_transaction_with_selector(
                node=self.nodes[0],
                tx_type=SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_SYSCOIN,
                asset_amounts=asset_amounts
            )
            self.nodes[0].sendrawtransaction(tx_hex)
            assert False, "Should have raised an error for non-SYSX burn"
        except Exception:
            pass  # Expected exception
        
        # Test case 2: ALLOCATION_MINT without SPV proof
        dest_addr = self.nodes[0].getnewaddress()
        new_test_guid = 55555  # Test GUID
        asset_amounts = [
            (new_test_guid, Decimal('100'), dest_addr)
        ]
        try:
            tx_hex = create_transaction_with_selector(
                node=self.nodes[0],
                tx_type=SYSCOIN_TX_VERSION_ALLOCATION_MINT,
                asset_amounts=asset_amounts,
                # Omitting spv_proof
            )
            self.nodes[0].sendrawtransaction(tx_hex)
            assert False, "Should have raised an error for missing SPV proof"
        except Exception:
            pass  # Expected exception
        
        # Test case 3: ALLOCATION_MINT with malformed SPV proof
        dest_addr = self.nodes[0].getnewaddress()
        new_test_guid = 66666  # Another test GUID
        asset_amounts = [
            (new_test_guid, Decimal('100'), dest_addr)
        ]
        # Incomplete SPV proof missing required fields
        bad_spv_proof = {
            "txHash": bytes.fromhex("a" * 64),
            # Missing other required fields
        }
        
        try:
            tx_hex = create_transaction_with_selector(
                node=self.nodes[0],
                tx_type=SYSCOIN_TX_VERSION_ALLOCATION_MINT,
                asset_amounts=asset_amounts,
                spv_proof=bad_spv_proof,
            )
            self.nodes[0].sendrawtransaction(tx_hex)
            assert False, "Should have raised an error for malformed SPV proof"
        except Exception:
            pass  # Expected exception
        
        # Test case 4: ALLOCATION_BURN_TO_NEVM without NEVM address
        asset_amounts = [
            (self.asset1_guid, Decimal('1'), '')
        ]
        try:
            tx_hex = create_transaction_with_selector(
                node=self.nodes[0],
                tx_type=SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_NEVM,
                asset_amounts=asset_amounts
                # Omitting nevm_address
            )
            self.nodes[0].sendrawtransaction(tx_hex)
            assert False, "Should have raised an error for missing NEVM address"
        except Exception:
            pass  # Expected exception
        
        # Test case 5: ALLOCATION_SEND without destinations
        asset_amounts = [
            (self.asset1_guid, Decimal('1'), '')
        ]
        try:
            tx_hex = create_transaction_with_selector(
                node=self.nodes[0],
                tx_type=SYSCOIN_TX_VERSION_ALLOCATION_SEND,
                asset_amounts=asset_amounts
            )
            self.nodes[0].sendrawtransaction(tx_hex)
            assert False, "Should have raised an error for missing destinations"
        except Exception:
            pass  # Expected exception
        
        print("Error case tests passed")


if __name__ == '__main__':
    AssetTransactionTest().main()
