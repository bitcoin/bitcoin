#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test simulaterawtransaction.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_raises_rpc_error,
)

class SimulateTxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self, split=False):
        self.setup_nodes()

    def run_test(self):
        node = self.nodes[0]

        node.generate(1) # Leave IBD

        node.createwallet(wallet_name='w0')
        node.createwallet(wallet_name='w1')
        w0 = node.get_wallet_rpc('w0')
        w1 = node.get_wallet_rpc('w1')

        node.generatetoaddress(nblocks=COINBASE_MATURITY + 1, address=w0.getnewaddress())
        assert_equal(w0.getbalance(), 50.0)
        assert_equal(w1.getbalance(), 0.0)

        address1 = w1.getnewaddress()
        address2 = w1.getnewaddress()

        # Make address1 watch-only in w0
        w0.importpubkey(pubkey=w1.getaddressinfo(address1)["pubkey"])
        all = {"include_watchonly": True}

        tx1 = node.createrawtransaction([], [{address1: 5.0}])
        tx2 = node.createrawtransaction([], [{address2: 10.0}])

        # w0 should be unaffected (include_watchonly=False) or see +5 (include_watchonly=True) for tx1
        assert_equal(w0.simulaterawtransaction([tx1])["balance_change"], 0.0)
        assert_equal(w0.simulaterawtransaction([tx1], all)["balance_change"], 5.0)

        # w1 should see +5 balance for tx1
        assert_equal(w1.simulaterawtransaction([tx1])["balance_change"], 5.0)

        # w0 should be unaffected (include_watchonly=False) or see +5 (include_watchonly=True) for both transactions
        assert_equal(w0.simulaterawtransaction([tx1, tx2])["balance_change"], 0.0)
        assert_equal(w0.simulaterawtransaction([tx1, tx2], all)["balance_change"], 5.0)

        # w1 should see +15 balance for both transactions
        assert_equal(w1.simulaterawtransaction([tx1, tx2])["balance_change"], 15.0)

        # w0 funds transaction; it should now see a decrease in (tx fee and payment), and w1 should see the same as above
        funding = w0.fundrawtransaction(tx1)
        tx1 = funding["hex"]
        bitcoin_fee = float(funding["fee"])

        # w0 sees fee + 5 btc decrease, or fee only for watchonly case
        assert_approx(w0.simulaterawtransaction([tx1])["balance_change"], -(5.0 + bitcoin_fee))
        assert_approx(w0.simulaterawtransaction([tx1], all)["balance_change"], -(bitcoin_fee))

        # w1 sees same as before
        assert_equal(w1.simulaterawtransaction([tx1])["balance_change"], 5.0)

        # same inputs (tx) more than once should error
        assert_raises_rpc_error(-8, "Transaction(s) are spending the same output more than once", w0.simulaterawtransaction, [tx1,tx1])

        # send tx1 to avoid reusing same UTXO below
        node.sendrawtransaction(w0.signrawtransactionwithwallet(tx1)["hex"])
        self.sync_all()

        # w0 funds transaction 2; it should now see a decrease in (tx fee and payment), and w1 should see the same as above
        funding = w0.fundrawtransaction(tx2)
        tx2 = funding["hex"]
        bitcoin_fee2 = float(funding["fee"])

        # w0 sees fees + 15 btc decrease, or fees + 10 btc decrease for watch only case
        assert_approx(w0.simulaterawtransaction([tx1, tx2])["balance_change"], -(5.0 + bitcoin_fee + 10.0 + bitcoin_fee2))
        assert_approx(w0.simulaterawtransaction([tx1, tx2], all)["balance_change"], -(bitcoin_fee + 10.0 + bitcoin_fee2))

        # w1 sees same as before
        assert_equal(w1.simulaterawtransaction([tx1, tx2])["balance_change"], 15.0)

if __name__ == '__main__':
    SimulateTxTest().main()
