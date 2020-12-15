#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test if on mainnet wallets can vanish inaccessible.

Verify that a bitcoind mainnet node will not loose wallets on new startup
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class WalletSanityTest(BitcoinTestFramework):

    def set_test_params(self):
        self.chain = 'main'
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info('test if we still have the original wallets key')
        self.log.info(self.nodes[0].listwallets())
        if self.nodes[0].listwallets() == []:
            self.nodes[0].createwallet('')
        assert_equal(self.nodes[0].listwallets(), [''])
        self.nodes[0].createwallet('w1')
        self.nodes[0].createwallet('w2')
        assert_equal(self.nodes[0].listwallets(), ['', 'w1', 'w2'])
        self.log.info(self.nodes[0].listwallets())
        n = self.nodes[0].get_wallet_rpc('')
        address = n.getnewaddress()
        assert_raises_rpc_error(-4, "Please use a different walletname", self.nodes[0].createwallet, 'wallets')
        n = self.nodes[0].get_wallet_rpc('')
        self.log.info('now restart node')
        self.restart_node(0)
        self.log.info(self.nodes[0].listwallets())
        n = self.nodes[0].get_wallet_rpc('')
        self.log.info('now check if we still have the wallet and the key')
        n.dumpprivkey(address)

if __name__ == '__main__':
    WalletSanityTest().main()
