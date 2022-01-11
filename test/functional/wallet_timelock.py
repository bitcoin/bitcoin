#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import (
    tx_from_hex,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE


class WalletLocktimeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        mtp_tip = node.getblockheader(node.getbestblockhash())["mediantime"]
        tx = node.sendtoaddress(ADDRESS_BCRT1_UNSPENDABLE, 5)
        tx = node.getrawtransaction(tx)
        tx = tx_from_hex(tx)
        tx.nLockTime = mtp_tip - 1
        tx = node.signrawtransactionwithwallet(tx.serialize().hex())["hex"]
        self.generateblock(node, ADDRESS_BCRT1_UNSPENDABLE, [tx])
        balance_before = node.getbalances()["mine"]["trusted"]
        coin_before = node.listunspent(maxconf=1)
        node.setmocktime(mtp_tip - 1)
        assert_equal(node.getbalances()["mine"]["trusted"], balance_before)
        assert_equal(node.listunspent(maxconf=1), coin_before)


if __name__ == "__main__":
    WalletLocktimeTest().main()
