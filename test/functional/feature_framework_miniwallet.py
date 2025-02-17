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
    assert_equal,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


class FeatureFrameworkMiniWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def test_tx_padding(self):
        """Verify that MiniWallet's transaction padding (`target_vsize` parameter)
           works accurately with all modes."""
        for mode_name, wallet in self.wallets:
            self.log.info(f"Test tx padding with MiniWallet mode {mode_name}...")
            utxo = wallet.get_utxo(mark_as_spent=False)
            for target_vsize in [250, 500, 1250, 2500, 5000, 12500, 25000, 50000, 1000000,
                                 248, 501, 1085, 3343, 5805, 12289, 25509, 55855,  999998]:
                tx = wallet.create_self_transfer(utxo_to_spend=utxo, target_vsize=target_vsize)
                assert_equal(tx['tx'].get_vsize(), target_vsize)
                child_tx = wallet.create_self_transfer_multi(utxos_to_spend=[tx["new_utxo"]], target_vsize=target_vsize)
                assert_equal(child_tx['tx'].get_vsize(), target_vsize)


    def test_wallet_tagging(self):
        """Verify that tagged wallet instances are able to send funds."""
        self.log.info("Test tagged wallet instances...")
        node = self.nodes[0]
        untagged_wallet = self.wallets[0][1]
        for i in range(10):
            tag = ''.join(random.choice(string.ascii_letters) for _ in range(20))
            self.log.debug(f"-> ({i}) tag name: {tag}")
            tagged_wallet = MiniWallet(node, tag_name=tag)
            untagged_wallet.send_to(from_node=node, scriptPubKey=tagged_wallet.get_output_script(), amount=100000)
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
