#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test MiniWallet."""
import random
import string

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_greater_than_or_equal,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


class FeatureFrameworkMiniWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def test_tx_padding(self):
        """Verify that MiniWallet's transaction padding (`target_weight` parameter)
           works accurately enough (i.e. at most 3 WUs higher) with all modes."""
        for mode_name, wallet in self.wallets:
            self.log.info(f"Test tx padding with MiniWallet mode {mode_name}...")
            utxo = wallet.get_utxo(mark_as_spent=False)
            for target_weight in [1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 4000000,
                                  989,  2001, 4337, 13371, 23219, 49153, 102035, 223419, 3999989]:
                tx = wallet.create_self_transfer(utxo_to_spend=utxo, target_weight=target_weight)["tx"]
                self.log.debug(f"-> target weight: {target_weight}, actual weight: {tx.get_weight()}")
                assert_greater_than_or_equal(tx.get_weight(), target_weight)
                assert_greater_than_or_equal(target_weight + 3, tx.get_weight())

    def test_wallet_tagging(self):
        """Verify that tagged wallet instances are able to send funds."""
        self.log.info(f"Test tagged wallet instances...")
        node = self.nodes[0]
        untagged_wallet = self.wallets[0][1]
        for i in range(10):
            tag = ''.join(random.choice(string.ascii_letters) for _ in range(20))
            self.log.debug(f"-> ({i}) tag name: {tag}")
            tagged_wallet = MiniWallet(node, tag_name=tag)
            untagged_wallet.send_to(from_node=node, scriptPubKey=tagged_wallet.get_scriptPubKey(), amount=100000)
            tagged_wallet.rescan_utxos()
            tagged_wallet.send_self_transfer(from_node=node)
        self.generate(node, 1)  # clear mempool

    def run_test(self):
        node = self.nodes[0]
        self.wallets = [
            ("ADDRESS_OP_TRUE", MiniWallet(node, mode=MiniWalletMode.ADDRESS_OP_TRUE)),
            ("RAW_OP_TRUE",     MiniWallet(node, mode=MiniWalletMode.RAW_OP_TRUE)),
            ("RAW_P2PK",        MiniWallet(node, mode=MiniWalletMode.RAW_P2PK)),
        ]
        for _, wallet in self.wallets:
            self.generate(wallet, 10)
        self.generate(wallet, COINBASE_MATURITY)

        self.test_tx_padding()
        self.test_wallet_tagging()


if __name__ == '__main__':
    FeatureFrameworkMiniWalletTest(__file__).main()
