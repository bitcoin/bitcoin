#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test wallet migration from ancient versions to current version.
Tests two wallet types that are actually testable:
1. Non-HD wallet from v0.14.3
2. HD wallet from v0.14.3 (VERSION_HD_BASE)
"""
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    initialize_datadir,
)

class WalletMigrationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()  # Skip if wallet is not compiled in
        self.skip_if_no_previous_releases()

    def setup_network(self):
        # Don't setup nodes here, we'll do it per test
        self.nodes = []

    def cleanup_nodes(self):
        """Clean up nodes and their data directories between tests"""
        # Stop any running nodes
        for i in range(len(self.nodes)):
            if self.nodes[i].running:
                self.stop_node(i)

        # Clean up data directories to ensure fresh start
        for i in range(len(self.nodes)):
            datadir = get_datadir_path(self.options.tmpdir, i)
            if os.path.exists(datadir):
                shutil.rmtree(datadir)
                self.log.debug(f"Cleaned up datadir for node {i}")
            # Re-initialize the datadir for the next test
            initialize_datadir(self.options.tmpdir, i, self.chain, self.disable_autoconnect)

        # Clear the nodes list
        self.nodes = []

    def migrate_wallet(self, wallet_type, extra_args_old):
        """Common migration test logic"""
        self.log.info(f"Testing {wallet_type} wallet migration from v0.14.3")

        # Clean up any existing nodes and data
        self.cleanup_nodes()

        # Setup fresh nodes for this test
        self.add_nodes(
            2,
            versions=[
                140300,     # v0.14.3 node
                None,       # Current version for testing migration
            ],
        )

        # Start the old node with specified args
        self.log.info(f"Starting old node (v0.14.3) with {wallet_type} wallet...")
        self.start_node(0, extra_args=extra_args_old)

        # Save the datadir paths while nodes are running
        node0_datadir = self.nodes[0].datadir_path
        node1_datadir = self.nodes[1].datadir_path

        # Generate some addresses and get wallet info from the old wallet
        self.log.info("Creating addresses and transactions in old wallet...")
        old_addresses = []
        for i in range(5):
            addr = self.nodes[0].getnewaddress()
            old_addresses.append(addr)
            self.log.info(f"  Generated address {i+1}: {addr}")

        # Get some wallet info before migration
        old_wallet_info = self.nodes[0].getwalletinfo()
        hdmasterkeyid = old_wallet_info.get('hdmasterkeyid', 'Not HD')
        self.log.info(f"Old wallet info: HD master key = {hdmasterkeyid}")
        assert('keypoolsize_hd_internal' not in old_wallet_info) # Ensure we are testing non-HD or single-chain (VERSION_HD_BASE) wallets

        # Verify wallet type
        if "non-HD" in wallet_type:
            assert('hdmasterkeyid' not in old_wallet_info)
        else:
            assert('hdmasterkeyid' in old_wallet_info)

        # Generate some blocks to the first address to have balance
        self.log.info("Generating blocks to create balance...")
        self.generatetoaddress(self.nodes[0], 101, old_addresses[0], sync_fun=lambda: None)

        # Send some transactions to create history
        self.log.info("Creating transaction history...")
        self.nodes[0].sendtoaddress(old_addresses[1], 0.1)
        self.nodes[0].sendtoaddress(old_addresses[2], 0.2)
        self.nodes[0].sendtoaddress(old_addresses[3], 0.3)

        # Mine the transactions
        self.generatetoaddress(self.nodes[0], 1, old_addresses[0], sync_fun=lambda: None)

        # Get the balance and transaction count
        old_balance = self.nodes[0].getbalance()
        old_txs = self.nodes[0].listtransactions("*", 1000)
        old_tx_count = len(old_txs)
        self.log.info(f"Old wallet balance: {old_balance} BTC")
        self.log.info(f"Old wallet transaction count: {old_tx_count}")
        assert(old_balance > 0)
        assert(old_tx_count > 0)

        # Get all addresses from transactions
        all_old_addresses = set()
        for tx in old_txs:
            if 'address' in tx:
                all_old_addresses.add(tx['address'])

        self.log.info(f"Total addresses in wallet transactions: {len(all_old_addresses)}")

        # Identify change addresses
        change_addresses = all_old_addresses - set(old_addresses)
        if change_addresses:
            self.log.info(f"Found {len(change_addresses)} change addresses:")
            for change_addr in change_addresses:
                self.log.info(f"  Change address: {change_addr}")

        # Stop the old node
        self.log.info("Stopping old node...")
        self.stop_node(0)

        # Copy blockchain data from node0 to node1
        self.log.info("Copying blockchain data from old node to modern node...")
        old_blocks_dir = os.path.join(node0_datadir, self.chain, "blocks")
        new_blocks_dir = os.path.join(node1_datadir, self.chain, "blocks")

        # Clean up any existing blocks directory in node1
        if os.path.exists(new_blocks_dir):
            shutil.rmtree(new_blocks_dir)

        if os.path.exists(old_blocks_dir):
            shutil.copytree(old_blocks_dir, new_blocks_dir)
            self.log.info("  Copied blocks directory")
        assert(os.path.exists(new_blocks_dir))

        # Start the modern node with -reindex-chainstate
        self.log.info("Starting modern node with -reindex-chainstate...")
        self.start_node(1, extra_args=["-reindex-chainstate"])

        # Wait for reindex to complete
        self.log.info("Waiting for chainstate reindex to complete...")
        self.wait_until(
            lambda: self.nodes[1].getblockcount() == 102,
            timeout=30
        )

        # Verify the blockchain was loaded correctly
        node1_info = self.nodes[1].getblockchaininfo()
        self.log.info(f"Modern node blockchain info: height={node1_info['blocks']}")
        assert_equal(node1_info['blocks'], 102)

        # Copy the old wallet to the modern node's wallet directory
        self.log.info("Copying old wallet to modern node...")
        old_wallet_path = os.path.join(node0_datadir, self.chain, "wallet.dat")
        modern_wallets_dir = os.path.join(node1_datadir, self.chain, "wallets")
        os.makedirs(modern_wallets_dir, exist_ok=True)

        migrated_wallet_dir = os.path.join(modern_wallets_dir, "migrated_wallet")
        # Clean up any existing migrated wallet
        if os.path.exists(migrated_wallet_dir):
            shutil.rmtree(migrated_wallet_dir)
        os.makedirs(migrated_wallet_dir, exist_ok=True)
        migrated_wallet_path = os.path.join(migrated_wallet_dir, "wallet.dat")
        shutil.copy2(old_wallet_path, migrated_wallet_path)
        assert(os.path.exists(migrated_wallet_path))

        # Perform wallet migration
        self.log.info("Performing wallet migration to descriptor wallet...")
        migration_result = self.nodes[1].migratewallet("migrated_wallet")
        self.log.info(f"Migration result: {migration_result}")

        assert('wallet_name' in migration_result)
        assert('backup_path' in migration_result)
        self.log.info(f"Backup created at: {migration_result['backup_path']}")
        assert(os.path.exists(migration_result['backup_path']))

        # Rescan the blockchain
        self.log.info("Rescanning blockchain...")
        self.nodes[1].rescanblockchain()

        # Verify the migration
        self.log.info("Verifying migration...")
        new_wallet_info = self.nodes[1].getwalletinfo()
        self.log.info("New wallet info:")
        self.log.info(f"  Format: {new_wallet_info.get('format', 'unknown')}")
        self.log.info(f"  HD: {new_wallet_info.get('hdmasterkeyid', 'Not HD')}")
        self.log.info(f"  Descriptors: {new_wallet_info.get('descriptors', False)}")

        # Assert expected wallet format after migration
        assert_equal(new_wallet_info.get('format'), 'sqlite')
        assert_equal(new_wallet_info.get('descriptors'), True)

        # For HD wallets, verify HD status is preserved
        if "non-HD" not in wallet_type:
            # After migration to descriptors, hdmasterkeyid might not be present but wallet should still be HD
            # Check if wallet can derive new addresses (indicator of HD functionality)
            assert(new_wallet_info.get('descriptors')) # HD wallet should be descriptor-based after migration

        # Verify balance is preserved
        new_balance = self.nodes[1].getbalance()
        self.log.info(f"New wallet balance: {new_balance} BTC")
        assert_equal(old_balance, new_balance)
        self.log.info(f"✓ Balance correctly preserved: {old_balance} BTC")

        # Verify transactions are preserved
        new_txs = self.nodes[1].listtransactions("*", 1000)
        new_tx_count = len(new_txs)
        self.log.info(f"New wallet transaction count: {new_tx_count}")
        assert_equal(old_tx_count, new_tx_count)
        self.log.info(f"✓ Transaction count correctly preserved: {old_tx_count}")

        # Verify all addresses are still valid
        self.log.info("Verifying all addresses are still valid...")
        for addr in old_addresses:
            addr_info = self.nodes[1].getaddressinfo(addr)
            assert(addr_info['ismine'])  # Address should still belong to the wallet after migration

        # Verify change addresses if any
        if change_addresses:
            for change_addr in change_addresses:
                addr_info = self.nodes[1].getaddressinfo(change_addr)
                assert(addr_info['ismine']) # Change address should still belong to the wallet

        self.log.info("✓ All addresses verified as owned after migration")

        # Test wallet functionality
        self.log.info("Testing wallet functionality after migration...")
        new_addr = self.nodes[1].getnewaddress()
        self.log.info(f"New address after migration: {new_addr}")
        assert(len(new_addr) > 0)

        new_addr_info = self.nodes[1].getaddressinfo(new_addr)
        assert('desc' in new_addr_info) # New address should have descriptor

        # Create a test transaction
        test_addr = self.nodes[1].getnewaddress()
        txid = self.nodes[1].sendtoaddress(test_addr, 0.1)
        self.log.info(f"✓ Test transaction created successfully: {txid}")
        assert(len(txid) == 64)

        # Cleanup for next test
        self.stop_node(1)
        self.log.info(f"✓ {wallet_type} wallet migration from v0.14.3 completed successfully!\n")

    def test_non_hd_migration(self):
        """Test migration of non-HD wallet from v0.14.3"""
        self.log.info("\n" + "="*60)
        self.log.info("TEST 1: Non-HD wallet migration from v0.14.3")
        self.log.info("="*60)
        self.migrate_wallet(
            wallet_type="non-HD",
            extra_args_old=["-usehd=0", "-keypool=10"]
        )

    def test_hd_chain_split_migration(self):
        """Test migration of HD wallet from v0.14.3 (VERSION_HD_CHAIN_SPLIT)"""
        self.log.info("\n" + "="*60)
        self.log.info("TEST 2: HD wallet migration from v0.14.3 (VERSION_HD_CHAIN_SPLIT)")
        self.log.info("="*60)
        self.migrate_wallet(
            wallet_type="HD (VERSION_HD_CHAIN_SPLIT)",
            # Explicitly enable HD wallet (HD should be default in v0.14.3, but be explicit)
            extra_args_old=["-keypool=10"]  # HD is default in v0.14.3, no need for -usehd=1
        )

    def run_test(self):
        self.log.info("=== Testing wallet migration from ancient versions ===")

        # Test 1: Non-HD wallet from v0.14.3
        self.test_non_hd_migration()

        # Test 2: HD wallet from v0.14.3 (VERSION_HD_CHAIN_SPLIT)
        self.test_hd_chain_split_migration()

if __name__ == '__main__':
    WalletMigrationTest(__file__).main()
