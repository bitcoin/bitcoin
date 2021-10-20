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
        addr = 'bcrt1qgfmtskhnyknjaqk0v9qmksssdrkg427rw4ppxd'
        priv = 'cQ6npZ7QAGxHe48VKrzQ1pNTWTvm1cMbLXULMEE7ysQANC1rePtg'
        
        self.log.info("Improrting priv key")
        self.nodes[0].importprivkey(priv)
        assert_equal(self.nodes[0].getbalance(), 5)

        node2addr = self.nodes[1].getnewaddress()
        self.log.info("attempt to spend coinbase in genesis block")
        amount = 3
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

        assert_approx(float(self.nodes[0].getbalance()), 2., vspan=0.001)

if __name__ == '__main__':
    SpendGenesisCoinbase().main()
