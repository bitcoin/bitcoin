#!/usr/bin/env python3
# Copyright (c) 2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction wiping using the wipewallettxes RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class WipeWalletTxesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Test that wipewallettxes removes txes and rescanblockchain is able to recover them")
        self.nodes[0].generate(101)
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getwalletinfo()["txcount"], 103)
        self.nodes[0].wipewallettxes()
        assert_equal(self.nodes[0].getwalletinfo()["txcount"], 0)
        self.nodes[0].rescanblockchain()
        assert_equal(self.nodes[0].getwalletinfo()["txcount"], 103)

        self.log.info("Test that wipewallettxes removes txes but keeps confirmed ones when asked to")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        assert_equal(self.nodes[0].getwalletinfo()["txcount"], 104)
        self.nodes[0].wipewallettxes(True)
        assert_equal(self.nodes[0].getwalletinfo()["txcount"], 103)
        self.nodes[0].rescanblockchain()
        assert_equal(self.nodes[0].getwalletinfo()["txcount"], 103)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", self.nodes[0].gettransaction, txid)


if __name__ == '__main__':
    WipeWalletTxesTest().main()
