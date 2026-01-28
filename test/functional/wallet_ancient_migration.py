#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet migration from ancient versions (v0.14.3) to descriptor wallets.

Previous releases are required by this test, see test/README.md.
"""
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_datadir_path,
    initialize_datadir,
)

# Wallet version constants (from src/wallet/walletutil.h)
FEATURE_COMPRPUBKEY = 60000  # Non-HD wallet version
FEATURE_HD = 130000          # HD wallet version (VERSION_HD_BASE)

COINBASE_MATURITY = 100


class WalletAncientMigrationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.nodes = []

    def reset_nodes(self):
        """Reset nodes and data directories between sub-tests."""
        for i, node in enumerate(self.nodes):
            if node.running:
                self.stop_node(i)

        for i in range(self.num_nodes):
            datadir = get_datadir_path(self.options.tmpdir, i)
            shutil.rmtree(datadir, ignore_errors=True)
            initialize_datadir(self.options.tmpdir, i, self.chain, self.disable_autoconnect)

        self.nodes = []

    def run_migration_test(self, wallet_type, extra_args_old, expected_version, expect_hd):
        """Test migration of a v0.14.3 wallet to descriptor wallet."""
        self.log.info(f"Testing {wallet_type} wallet migration")
        self.reset_nodes()

        self.add_nodes(self.num_nodes, versions=[140300, None])
        self.start_node(0, extra_args=extra_args_old)

        # Create addresses and verify wallet version
        old_addresses = [self.nodes[0].getnewaddress() for _ in range(5)]
        old_wallet_info = self.nodes[0].getwalletinfo()
        assert_equal(old_wallet_info['walletversion'], expected_version)
        assert 'keypoolsize_hd_internal' not in old_wallet_info  # Verify single-chain wallet
        assert_equal('hdmasterkeyid' in old_wallet_info, expect_hd)

        # Generate blocks and create transaction history
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, old_addresses[0], sync_fun=self.no_op)
        for i, amount in enumerate([0.1, 0.2, 0.3], start=1):
            self.nodes[0].sendtoaddress(old_addresses[i], amount)
        self.generatetoaddress(self.nodes[0], 1, old_addresses[0], sync_fun=self.no_op)

        old_balance = self.nodes[0].getbalance()
        old_txs = self.nodes[0].listtransactions("*", 1000)
        assert old_balance > 0
        assert len(old_txs) > 0

        # Collect all addresses including change
        all_addresses = {tx['address'] for tx in old_txs if 'address' in tx}

        self.stop_node(0)

        # Copy blockchain to modern node and reindex
        self.log.info("Copying blockchain to modern node")
        shutil.rmtree(self.nodes[1].blocks_path, ignore_errors=True)
        shutil.copytree(self.nodes[0].blocks_path, self.nodes[1].blocks_path)

        expected_height = COINBASE_MATURITY + 2
        self.start_node(1, extra_args=["-reindex-chainstate"])
        self.wait_until(lambda: self.nodes[1].getblockcount() == expected_height)
        assert_equal(self.nodes[1].getblockchaininfo()['blocks'], expected_height)

        # Copy and migrate wallet
        self.log.info("Migrating wallet to descriptor format")
        old_wallet_path = self.nodes[0].chain_path / "wallet.dat"
        migrated_wallet_dir = self.nodes[1].wallets_path / "migrated_wallet"
        shutil.rmtree(migrated_wallet_dir, ignore_errors=True)
        migrated_wallet_dir.mkdir(parents=True)
        shutil.copy2(old_wallet_path, migrated_wallet_dir / "wallet.dat")

        migration_result = self.nodes[1].migratewallet("migrated_wallet")
        assert 'wallet_name' in migration_result
        assert 'backup_path' in migration_result

        # Verify migration results
        self.log.info("Verifying migration")
        new_wallet_info = self.nodes[1].getwalletinfo()
        assert_equal(new_wallet_info['format'], 'sqlite')
        assert_equal(new_wallet_info['descriptors'], True)
        assert_equal(self.nodes[1].getbalance(), old_balance)
        assert_equal(len(self.nodes[1].listtransactions("*", 1000)), len(old_txs))

        # Verify all addresses are still owned
        for addr in all_addresses:
            assert self.nodes[1].getaddressinfo(addr)['ismine']

        # Test post-migration functionality
        new_addr = self.nodes[1].getnewaddress()
        assert 'desc' in self.nodes[1].getaddressinfo(new_addr)
        txid = self.nodes[1].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        assert len(txid) == 64

        self.stop_node(1)
        self.log.info(f"{wallet_type} wallet migration successful")

    def run_test(self):
        self.log.info("Testing wallet migration from v0.14.3")

        self.run_migration_test(
            wallet_type="non-HD",
            extra_args_old=["-usehd=0", "-keypool=10"],
            expected_version=FEATURE_COMPRPUBKEY,
            expect_hd=False,
        )

        self.run_migration_test(
            wallet_type="HD (VERSION_HD_BASE)",
            extra_args_old=["-keypool=10"],
            expected_version=FEATURE_HD,
            expect_hd=True,
        )


if __name__ == '__main__':
    WalletAncientMigrationTest(__file__).main()
