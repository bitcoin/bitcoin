#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the upgradewallet RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

class UpgradeWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.uses_wallet = None

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_latest_wallet_upgrade(self, node):
        """Test upgradewallet on a wallet already at FEATURE_LATEST."""
        wallet_name = "test_latest"
        node.createwallet(wallet_name)
        wallet = node.get_wallet_rpc(wallet_name)
        info = wallet.getwalletinfo()
        assert_equal(info["walletversion"], 169900)
        # upgradewallet should report no upgrade needed
        result = wallet.upgradewallet()
        self.log.info(f"Upgrade result (already latest): {result}")
        assert_equal(result["previous_version"], 169900)
        assert_equal(result["current_version"], 169900)
        assert "Already at latest version" in result["result"]

    def test_downgrade_wallet_error(self, node):
        """Test that trying to downgrade wallet to a lower version raises an error."""
        wallet_name = "test_downgrade"
        node.createwallet(wallet_name)
        wallet = node.get_wallet_rpc(wallet_name)
        info = wallet.getwalletinfo()
        current_version = info["walletversion"]
        self.log.info(f"Current wallet version: {current_version}")
        # Try to downgrade to a lower version (should fail)
        lower_version = 40000  # FEATURE_WALLETCRYPT
        result = wallet.upgradewallet(lower_version)
        assert_equal(result["error"], "Cannot downgrade wallet from version 169900 to version 40000. Wallet version unchanged.")

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Testing upgradewallet RPC - already latest wallet case")
        self.test_latest_wallet_upgrade(node)
        self.log.info("Testing upgradewallet RPC - downgrade wallet case")
        self.test_downgrade_wallet_error(node)

if __name__ == '__main__':
    UpgradeWalletTest(__file__).main()
