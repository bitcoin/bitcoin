#!/usr/bin/env python3
# Copyright (c) 2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test -mnemonicbits wallet option."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

class WalletMnemonicbitsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-usehd=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Test -mnemonicbits")

        self.log.info("Invalid -mnemonicbits should rise an error")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(self.extra_args[0] + ['-mnemonicbits=123'], "Error: Invalid '-mnemonicbits'. Allowed values: 128, 160, 192, 224, 256.")
        self.start_node(0)
        assert_equal(len(self.nodes[0].dumphdinfo()["mnemonic"].split()), 12)  # 12 words by default

        self.log.info("Can have multiple wallets with different mnemonic length loaded at the same time")
        self.restart_node(0, extra_args=self.extra_args[0] + ["-mnemonicbits=160"])
        self.nodes[0].createwallet("wallet_160")
        self.restart_node(0, extra_args=self.extra_args[0] + ["-mnemonicbits=192"])
        self.nodes[0].createwallet("wallet_192")
        self.restart_node(0, extra_args=self.extra_args[0] + ["-mnemonicbits=224"])
        self.nodes[0].createwallet("wallet_224")
        self.restart_node(0, extra_args=self.extra_args[0] + ["-mnemonicbits=256"])
        self.nodes[0].loadwallet("wallet_160")
        self.nodes[0].loadwallet("wallet_192")
        self.nodes[0].loadwallet("wallet_224")
        self.nodes[0].createwallet("wallet_256", False, True)  # blank
        self.nodes[0].get_wallet_rpc("wallet_256").upgradetohd()
        assert_equal(len(self.nodes[0].get_wallet_rpc(self.default_wallet_name).dumphdinfo()["mnemonic"].split()), 12)  # 12 words by default
        assert_equal(len(self.nodes[0].get_wallet_rpc("wallet_160").dumphdinfo()["mnemonic"].split()), 15)              # 15 words
        assert_equal(len(self.nodes[0].get_wallet_rpc("wallet_192").dumphdinfo()["mnemonic"].split()), 18)              # 18 words
        assert_equal(len(self.nodes[0].get_wallet_rpc("wallet_224").dumphdinfo()["mnemonic"].split()), 21)              # 21 words
        assert_equal(len(self.nodes[0].get_wallet_rpc("wallet_256").dumphdinfo()["mnemonic"].split()), 24)              # 24 words


if __name__ == '__main__':
    WalletMnemonicbitsTest().main ()
