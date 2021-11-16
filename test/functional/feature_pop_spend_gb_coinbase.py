#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2021 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.pop import mine_until_pop_active
from test_framework.util import (
    connect_nodes,
    assert_equal,
    assert_approx
)
from test_framework.pop_const import NETWORK_ID

import time


class SpendGenesisCoinbase(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-txindex'], ['-txindex']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        self.nodes[1].generate(nblocks=200)
        connect_nodes(self.nodes[0], 1)
        self.sync_all()

    def run_test(self):
        """Main test logic"""
        assert_equal(self.nodes[0].getbalance(), 0)
        self.log.info("Improrting priv keys")
        self.nodes[0].importprivkey("cU1QaDgufU6FH24feZJ3f7RToVVbriuw7cZ91427mgbVA4Sk1kAi")
        self.nodes[0].importprivkey("cUNUxiqMtffurfGu9BKocHTvvnujynZh1Yrc4CVhgTBeQJfC9MqU")
        self.nodes[0].importprivkey("cTD9hmreLGnNPBPTz4TY4YbKCXZXfC6cyEXhcZTCzzycVmWwS643")
        expectedAmount = 30
        remaining = 1
        assert_equal(self.nodes[0].getbalance(), expectedAmount)

        node2addr = self.nodes[1].getnewaddress()
        self.log.info("attempt to spend coinbase in genesis block")
        amount = expectedAmount - remaining
        txid = self.nodes[0].sendtoaddress(node2addr, amount)

        self.log.info("node0 sent {} to node1 in tx={}".format(amount, txid))
        self.sync_mempools()

        confirmations = 10
        self.nodes[1].generate(nblocks=confirmations)
        self.sync_all()

        try:
            self.nodes[1].getrawtransaction(txid)
        except:
            raise Exception("node1 does not know about tx={}. it's a sign that coinbase tx can not be spent.".format(txid))

        assert_approx(float(self.nodes[0].getbalance()), remaining, vspan=0.001)

        balance = self.nodes[1].getbalance()
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), balance - remaining)
        self.nodes[1].generate(nblocks=confirmations)
        assert_approx(float(self.nodes[0].getbalance()), float((balance - remaining) + remaining), vspan=0.001)

if __name__ == '__main__':
    SpendGenesisCoinbase().main()
