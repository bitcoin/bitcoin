#!/usr/bin/env python3
# Copyright (c) 2016-2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Wallet encryption"""

import time
import subprocess

from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_array_result,
    assert_equal,
)

from test_framework.wallet_util import WalletUnlock

class WalletCodex32Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def recover_unencrypted_wallet(self):

        self.nodes[1].createwallet(wallet_name="w1")
        wallet_w1 = self.nodes[1].get_wallet_rpc("w1")
        w1_fist_address = wallet_w1.getnewaddress()
        w1_second_address = wallet_w1.getnewaddress()

        txid = self.nodes[0].sendtoaddress(w1_fist_address, 0.1)
        blockhash = self.generate(self.nodes[0], 1)[0]
        blockheight = self.nodes[0].getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(wallet_w1.listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        codex32_seed = wallet_w1.exposesecret()[0]
        self.log.info(codex32_seed)

        self.nodes[1].recoverwalletfromseed("w1_recover", codex32_seed)

        wallet_w1_recover = self.nodes[1].get_wallet_rpc("w1_recover")
        assert_equal(w1_second_address, wallet_w1_recover.getnewaddress())

        self.log.info(wallet_w1_recover.listtransactions())

        assert_array_result(wallet_w1_recover.listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

    def recover_encrypted_wallet(self):
        self.nodes[1].createwallet(wallet_name="enc_w1", passphrase="123456")
        enc_wallet_w1 = self.nodes[1].get_wallet_rpc("enc_w1")
        w1_fist_address = enc_wallet_w1.getnewaddress()
        w1_second_address = enc_wallet_w1.getnewaddress()

        txid = self.nodes[0].sendtoaddress(w1_fist_address, 0.1)
        blockhash = self.generate(self.nodes[0], 1)[0]
        blockheight = self.nodes[0].getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(enc_wallet_w1.listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        enc_wallet_w1.walletpassphrase("123456", 2)

        codex32_seed = enc_wallet_w1.exposesecret()[0]
        self.log.info(codex32_seed)

        self.nodes[1].recoverwalletfromseed(wallet_name="enc_w1_recover", codex32_seed=codex32_seed, passphrase="123456")

        enc_wallet_w1_recover = self.nodes[1].get_wallet_rpc("enc_w1_recover")
        assert_equal(w1_second_address, enc_wallet_w1_recover.getnewaddress())

        self.log.info(enc_wallet_w1_recover.listtransactions())

        assert_array_result(enc_wallet_w1_recover.listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})


    def run_test(self):
        self.recover_unencrypted_wallet()
        self.recover_encrypted_wallet()


if __name__ == '__main__':
    WalletCodex32Test(__file__).main()
