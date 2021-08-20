#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getbalances with a transaction included.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_raises_rpc_error,
)

class GetBalancesTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self, split=False):
        self.setup_nodes()

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]
        self.connect_nodes(0, 1)

        self.generate(node0, 1, sync_fun=self.no_op) # Leave IBD

        node0.createwallet(wallet_name='w0')
        w0 = node0.get_wallet_rpc('w0')
        node1.createwallet(wallet_name='w1')
        w1 = node1.get_wallet_rpc('w1')

        self.generatetoaddress(node0, COINBASE_MATURITY + 1, w0.getnewaddress())
        assert_equal(w0.getbalance(), 50.0)
        assert_equal(w1.getbalance(), 0.0)

        address1 = w1.getnewaddress()
        address2 = w1.getnewaddress()

        tx1 = node1.createrawtransaction([], [{address1: 5.0}])
        tx2 = node1.createrawtransaction([], [{address2: 10.0}])

        # node0 should be unaffected
        assert_equal(w0.getbalances([tx1])["tx"]["changes"], 0.0)

        # node1 should see +5 balance
        assert_equal(w1.getbalances([tx1])["tx"]["changes"], 5.0)

        # node1 should see +15 balance for both transactions
        assert_equal(w1.getbalances([tx1, tx2])["tx"]["changes"], 15.0)

        # w0 funds transaction; it should now see a decrease in (tx fee and payment), and w1 should see the same as above
        funding = w0.fundrawtransaction(tx1)
        tx1 = funding["hex"]
        bitcoin_fee = float(funding["fee"])

        # node0 sees fee + 5 btc decrease
        assert_approx(w0.getbalances([tx1])["tx"]["changes"], -(5.0 + bitcoin_fee))

        # node1 sees same as before
        assert_equal(w1.getbalances([tx1])["tx"]["changes"], 5.0)

        # same inputs (tx) more than once should error
        assert_raises_rpc_error(-8, "Transaction(s) are spending the same output more than once", w0.getbalances, [tx1,tx1])

        # send tx1 to avoid reusing same UTXO below
        node1.sendrawtransaction(w0.signrawtransactionwithwallet(tx1)["hex"])
        self.sync_all()

        # w0 funds transaction 2; it should now see a decrease in (tx fee and payment), and w1 should see the same as above
        funding = w0.fundrawtransaction(tx2)
        tx2 = funding["hex"]
        bitcoin_fee2 = float(funding["fee"])

        # node0 sees fees + 15 btc decrease
        assert_approx(w0.getbalances([tx1, tx2])["tx"]["changes"], -(5.0 + bitcoin_fee + 10.0 + bitcoin_fee2))

        # node1 sees same as before
        assert_equal(w1.getbalances([tx1, tx2])["tx"]["changes"], 15.0)

if __name__ == '__main__':
    GetBalancesTxTest().main()
