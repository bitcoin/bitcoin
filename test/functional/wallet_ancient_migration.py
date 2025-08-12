#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet migration from ancient versions (v0.14.3) to descriptor wallets.

Previous releases are required by this test, see test/README.md.
"""
import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_is_hash_string,
    dumb_sync_blocks,
)

# Wallet version constants from ancient Bitcoin Core (pre-0.15)
FEATURE_COMPRPUBKEY = 60000  # Non-HD wallet version
FEATURE_HD = 130000          # HD wallet version


class WalletAncientMigrationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_network(self):
        # Use dedicated old/new node pairs for each migration scenario to avoid
        # resetting node datadirs between sub-tests.
        self.add_nodes(self.num_nodes, versions=[140300, None, 140300, None])

    def get_vout_addresses(self, decoded_tx):
        for vout in decoded_tx["vout"]:
            script_pub_key = vout["scriptPubKey"]
            if "address" in script_pub_key:
                yield script_pub_key["address"]
            else:
                yield from script_pub_key.get("addresses", [])

    def run_migration_test(self, wallet_type, old_node_idx, new_node_idx, extra_args_old, expected_version, expect_hd):
        """Test migration of a v0.14.3 wallet to descriptor wallet."""
        old_node = self.nodes[old_node_idx]
        new_node = self.nodes[new_node_idx]

        self.log.info(f"Testing {wallet_type} wallet migration")
        self.start_node(old_node_idx, extra_args=extra_args_old)

        # Create addresses and verify wallet version
        old_addresses = [old_node.getnewaddress() for _ in range(4)]
        old_wallet_info = old_node.getwalletinfo()
        assert_equal(old_wallet_info['walletversion'], expected_version)
        assert 'keypoolsize_hd_internal' not in old_wallet_info  # Verify single-chain wallet
        assert_equal('hdmasterkeyid' in old_wallet_info, expect_hd)

        # Generate blocks and create transaction history
        self.generatetoaddress(old_node, COINBASE_MATURITY + 1, old_addresses[0], sync_fun=self.no_op)
        send_txs = []
        for i, amount in enumerate([0.1, 0.2, 0.3], start=1):
            send_txs.append((old_node.sendtoaddress(old_addresses[i], amount), old_addresses[i]))
        self.generatetoaddress(old_node, 1, old_addresses[0], sync_fun=self.no_op)

        old_balance = old_node.getbalance()
        old_txs = old_node.listtransactions("*", 1000)
        assert_greater_than(old_balance, 0)
        assert_greater_than(len(old_txs), 0)

        # Collect all addresses including change; listtransactions omits change
        # addresses for self-sends, so decode the sent transactions explicitly.
        change_addresses = set()
        for txid, destination in send_txs:
            decoded_tx = old_node.decoderawtransaction(old_node.gettransaction(txid)["hex"])
            for address in self.get_vout_addresses(decoded_tx):
                if address != destination:
                    change_addresses.add(address)

        all_addresses = {tx['address'] for tx in old_txs if 'address' in tx} | change_addresses
        assert_greater_than(len(change_addresses), 0)
        assert_greater_than(len(all_addresses), 0)

        # Sync blocks to modern node via RPC (avoids filesystem copy and reindex)
        self.start_node(new_node_idx)
        dumb_sync_blocks(src=old_node, dst=new_node)
        self.stop_node(old_node_idx)

        # Copy and migrate wallet
        self.log.info("Migrating wallet to descriptor format")
        old_wallet_path = old_node.chain_path / "wallet.dat"
        migrated_wallet_dir = new_node.wallets_path / "migrated_wallet"
        migrated_wallet_dir.mkdir(parents=True)
        shutil.copy2(old_wallet_path, migrated_wallet_dir / "wallet.dat")

        migration_result = new_node.migratewallet("migrated_wallet")
        assert 'wallet_name' in migration_result
        assert 'backup_path' in migration_result

        # Verify migration results
        self.log.info("Verifying migration")
        new_wallet_info = new_node.getwalletinfo()
        assert_equal(new_wallet_info['format'], 'sqlite')
        assert_equal(new_wallet_info['descriptors'], True)
        assert_equal(new_node.getbalance(), old_balance)
        assert_equal(len(new_node.listtransactions("*", 1000)), len(old_txs))

        # Verify all addresses are still owned
        for addr in all_addresses:
            assert new_node.getaddressinfo(addr)['ismine']

        # Test post-migration functionality
        new_addr = new_node.getnewaddress()
        txid = new_node.sendtoaddress(new_addr, 0.1)
        assert_is_hash_string(txid)

        self.stop_node(new_node_idx)
        self.log.info(f"{wallet_type} wallet migration successful")

    def run_test(self):
        self.log.info("Testing wallet migration from v0.14.3")

        self.run_migration_test(
            wallet_type="non-HD",
            old_node_idx=0,
            new_node_idx=1,
            extra_args_old=["-usehd=0", "-keypool=10"],
            expected_version=FEATURE_COMPRPUBKEY,
            expect_hd=False,
        )

        self.run_migration_test(
            wallet_type="HD (VERSION_HD_BASE)",
            old_node_idx=2,
            new_node_idx=3,
            extra_args_old=["-keypool=10"],
            expected_version=FEATURE_HD,
            expect_hd=True,
        )


if __name__ == '__main__':
    WalletAncientMigrationTest(__file__).main()
