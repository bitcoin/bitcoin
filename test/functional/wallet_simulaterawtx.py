#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test simulaterawtransaction.
"""

from decimal import Decimal
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

        self.generate(node, 1, sync_fun=self.no_op) # Leave IBD

        node.createwallet(wallet_name='w0')
        node.createwallet(wallet_name='w1')
        node.createwallet(wallet_name='w2', disable_private_keys=True)
        w0 = node.get_wallet_rpc('w0')
        w1 = node.get_wallet_rpc('w1')
        w2 = node.get_wallet_rpc('w2')

        self.generatetoaddress(node, COINBASE_MATURITY + 1, w0.getnewaddress())
        assert_equal(w0.getbalance(), 50.0)
        assert_equal(w1.getbalance(), 0.0)

        address1 = w1.getnewaddress()
        address2 = w1.getnewaddress()

        # Add address1 as watch-only to w2
        w2.importpubkey(pubkey=w1.getaddressinfo(address1)["pubkey"])

        tx1 = node.createrawtransaction([], [{address1: 5.0}])
        tx2 = node.createrawtransaction([], [{address2: 10.0}])

        # w0 should be unaffected, w2 should see +5 for tx1
        assert_equal(w0.simulaterawtransaction([tx1])["balance_change"], 0.0)
        assert_equal(w2.simulaterawtransaction([tx1])["balance_change"], 5.0)

        # w1 should see +5 balance for tx1

        tx1hex = node.decoderawtransaction(tx1)

        details = [
            {
                "txid" : tx1hex["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
        ]
        rpc_output = {
            "balance_change" : 5.0,
            "details" : details,
            "duplicates" : []
        }
        assert_equal(w1.simulaterawtransaction([tx1]), rpc_output)


        # w0 should be unaffected, w2 should see +5 for both transactions
        assert_equal(w0.simulaterawtransaction([tx1, tx2])["balance_change"], 0.0)
        assert_equal(w2.simulaterawtransaction([tx1, tx2])["balance_change"], 5.0)
        rpc_output = {
            "balance_change" : 5.0,
            "details" : details,
            "duplicates" : []
        }
        # w1 should see +15 balance for both transactions
        details = [
            {
                "txid" : tx1hex["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
            {
                "txid" : node.decoderawtransaction(tx2)["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
        ]

        rpc_output = {
            "balance_change" : 15.0,
            "details" : details,
            "duplicates" : []
        }
        assert_equal(w1.simulaterawtransaction([tx1, tx2]), rpc_output)

        # w0 funds transaction; it should now see a decrease in (tx fee and payment), and w1 should see the same as above
        funding = w0.fundrawtransaction(tx1)
        tx1 = funding["hex"]
        tx1changepos = funding["changepos"]
        bitcoin_fee = Decimal(funding["fee"])

        # w0 sees (fee + 5) btc decrease, w2 sees + 5 btc
        assert_approx(w0.simulaterawtransaction([tx1])["balance_change"], -(Decimal("5") + bitcoin_fee))
        assert_approx(w2.simulaterawtransaction([tx1])["balance_change"], Decimal("5"))
        tx1ob = node.decoderawtransaction(tx1)
        # w1 sees same as before
        assert_equal(w1.simulaterawtransaction([tx1])["balance_change"], 5.0)
        # redundant transactions should be shown
        # Testcase : w1.simulaterawtransaction([tx1,tx1])
        details = [
            {
                "txid" : tx1ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
        ]

        rpc_output = {
            "balance_change" : 5.0,
            "details" : details,
            "duplicates" : [1]
        }
        assert_equal(w1.simulaterawtransaction([tx1,tx1]), rpc_output)
        tx1hex = tx1ob["txid"]
        tx1vout = 1 - tx1changepos
        # tx3 spends new w1 UTXO paying to w0
        tx3 = node.createrawtransaction([{"txid": tx1hex, "vout": tx1vout}], {w0.getnewaddress(): 4.9999})
        # tx4 spends new w1 UTXO paying to w1
        tx4 = node.createrawtransaction([{"txid": tx1hex, "vout": tx1vout}], {w1.getnewaddress(): 4.9999})

        # on their own, both should fail due to missing input(s)
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w0.simulaterawtransaction, [tx3])
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w1.simulaterawtransaction, [tx3])
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w0.simulaterawtransaction, [tx4])
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w1.simulaterawtransaction, [tx4])

        # they should succeed when including tx1:
        #       wallet                  tx3                             tx4
        #       w0                      -5 - bitcoin_fee + 4.9999       -5 - bitcoin_fee
        #       w1                      0                               +4.9999
        assert_approx(w0.simulaterawtransaction([tx1, tx3])["balance_change"], -Decimal("5") - bitcoin_fee + Decimal("4.9999"))
        assert_approx(w1.simulaterawtransaction([tx1, tx3])["balance_change"], 0)
        assert_approx(w0.simulaterawtransaction([tx1, tx4])["balance_change"], -Decimal("5") - bitcoin_fee)
        assert_approx(w1.simulaterawtransaction([tx1, tx4])["balance_change"], Decimal("4.9999"))
        # send tx1 to avoid reusing same UTXO below
        node.sendrawtransaction(w0.signrawtransactionwithwallet(tx1)["hex"])
        self.generate(node, 1, sync_fun=self.no_op) # Confirm tx to trigger error below
        self.sync_all()

        # w0 funds transaction 2; it should now see a decrease in (tx fee and payment), and w1 should see the same as above
        funding2 = w0.fundrawtransaction(tx2)
        tx2 = funding2["hex"]
        bitcoin_fee2 = Decimal(funding2["fee"])
        tx2ob = node.decoderawtransaction(tx2)
        assert_approx(w0.simulaterawtransaction([tx2])["balance_change"], -(Decimal("10") + bitcoin_fee2))
        assert_approx(w1.simulaterawtransaction([tx2])["balance_change"], +(Decimal("10")))
        assert_approx(w2.simulaterawtransaction([tx2])["balance_change"], 0)

        # w0-w2 error due to tx1 already being mined
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w0.simulaterawtransaction, [tx1, tx2])
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w1.simulaterawtransaction, [tx1, tx2])
        assert_raises_rpc_error(-8, "One or more transaction inputs are missing or have been spent already", w2.simulaterawtransaction, [tx1, tx2])

        node.sendrawtransaction(w0.signrawtransactionwithwallet(tx2)["hex"])

        # tx2 -> tx3 -> (tx4,tx5) (spending UTXO generated by tx4)
        # tx2 is a unconfirmed wallet transaction
        # tx3 is spending an UTXO generated by tx2
        # tx4 and tx5 are spending the same UTXO which is generated by tx3
        tx3 = node.createrawtransaction([{"txid": tx2ob["txid"], "vout": 0}], {w0.getnewaddress(): 4.9999})
        tx3ob = node.decoderawtransaction(tx3)
        tx4 = node.createrawtransaction([{"txid": tx3ob["txid"], "vout": 0}], {w0.getnewaddress(): 4.9999})
        tx4ob = node.decoderawtransaction(tx4)
        tx5 = node.createrawtransaction([{"txid": tx3ob["txid"], "vout": 0}], {w0.getnewaddress(): 4.9998})
        tx5ob = node.decoderawtransaction(tx5)

        # Test case : simulaterawtransaction([tx2,tx2,tx2])
        # No conflicts in wallet, transaction at index 1 and index 2 are duplicates of transaction at index 0 (0-based indexing)
        duplicates = [1, 2]
        details = [
            {
                "txid" : tx2ob["txid"],
                "wallet_conflicts" : [tx2ob["txid"]],
                "simulated_tx_conflicts" : []
            },
        ]
        sim_result = w0.simulaterawtransaction([tx2, tx2, tx2])
        assert_equal(sim_result["details"], details)
        assert_equal(sim_result["duplicates"], duplicates)

        # Testcase : simulaterawtransaction([tx2, tx3, tx4])
        # tx2 has a wallet conflict with tx2 because tx2 is already in the wallet but no conflicts with any other transaction provided by the user(namely, tx3 and tx4)
        # tx3 and tx4 do not have any wallet conflicts or conflicts with any other transaction provided by the user
        details = [
            {
                "txid" : tx2ob["txid"],
                "wallet_conflicts" : [tx2ob["txid"]],
                "simulated_tx_conflicts" : []
            },
            {
                "txid" : tx3ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
            {
                "txid" : tx4ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
        ]
        sim_result = w0.simulaterawtransaction([tx2, tx3, tx4])
        assert_equal(sim_result["details"], details)
        assert_equal(sim_result["duplicates"], [])

        # Testcase : simulaterawtransaction([tx2,tx3,tx4,tx5])
        # last two transactions are trying to spend the same UTXO
        details = [
            {
                "txid" : tx2ob["txid"],
                "wallet_conflicts" : [tx2ob["txid"]],
                "simulated_tx_conflicts" : []
            },
            {
                "txid" : tx3ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
            {
                "txid" : tx4ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : [3]
            },
            {
                "txid" : tx5ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : [2]
            },
        ]
        sim_result = w0.simulaterawtransaction([tx2, tx3, tx4, tx5])
        assert_equal(sim_result["details"], details)
        assert_equal(sim_result["duplicates"], [])

        # Testcase : simulaterawtransaction([tx3,tx4,tx5])
        # last two transactions are trying to spend the same UTXO
        details = [
            {
                "txid" : tx3ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : []
            },
            {
                "txid" : tx4ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : [2]
            },
            {
                "txid" : tx5ob["txid"],
                "wallet_conflicts" : [],
                "simulated_tx_conflicts" : [1]
            },
        ]
        sim_result = w0.simulaterawtransaction([tx3,tx4,tx5])
        assert_equal(sim_result["details"], details)
        assert_equal(sim_result["duplicates"],[])
        # Testcase : tx6, tx7 and tx8 are in the wallet and tx9 conflicts with them
        # tx6, tx7 and tx8 are paying to w1, tx9 is trying to spend their input
        tx6 = node.createrawtransaction([], {w0.getnewaddress(): 6.0})
        tx7 = node.createrawtransaction([], {w0.getnewaddress(): 7.0})
        tx8 = node.createrawtransaction([], {w0.getnewaddress(): 8.0})

        funding6 = w0.fundrawtransaction(tx6)
        tx6 = funding6["hex"]
        tx6 = w0.signrawtransactionwithwallet(tx6)["hex"]
        node.sendrawtransaction(tx6)

        funding7 = w0.fundrawtransaction(tx7)
        tx7 = funding7["hex"]
        tx7 = w0.signrawtransactionwithwallet(tx7)["hex"]
        node.sendrawtransaction(tx7)

        funding8 = w0.fundrawtransaction(tx8)
        tx8 = funding8["hex"]
        tx8 = w0.signrawtransactionwithwallet(tx8)["hex"]
        node.sendrawtransaction(tx8)
        tx6ob = node.decoderawtransaction(tx6)
        tx7ob = node.decoderawtransaction(tx7)
        tx8ob = node.decoderawtransaction(tx8)
        tx9 = node.createrawtransaction([{"txid": tx6ob["vin"][0]["txid"], "vout": tx6ob["vin"][0]["vout"]}, {"txid": tx7ob["vin"][0]["txid"], "vout": tx7ob["vin"][0]["vout"]}, {"txid": tx8ob["vin"][0]["txid"], "vout": tx8ob["vin"][0]["vout"]}], {w1.getnewaddress(): 5.9999, w1.getnewaddress(): 6.9999, w1.getnewaddress(): 7.9999})
        tx9ob = node.decoderawtransaction(tx9)
        details = [
            {
                "txid" : tx9ob["txid"],
                "wallet_conflicts" : [tx6ob["txid"], tx7ob["txid"], tx8ob["txid"]],
                "simulated_tx_conflicts" : []
            }
        ]
        sim_result =  w0.simulaterawtransaction([tx9])["details"]
        sim_result[0]["wallet_conflicts"].sort()
        details[0]["wallet_conflicts"].sort()
        assert_equal(sim_result, details)

        # Testcase: tx9, tx10, tx11 and tx12 conflict with each other and tx6 which is already in wallet, tx9 conflicts also with tx6, tx7 and tx8
        tx10 = node.createrawtransaction([{"txid": tx6ob["vin"][0]["txid"], "vout": tx6ob["vin"][0]["vout"]}], {w1.getnewaddress(): 5.9999})
        tx10ob = node.decoderawtransaction(tx10)
        tx11 = node.createrawtransaction([{"txid": tx6ob["vin"][0]["txid"], "vout": tx6ob["vin"][0]["vout"]}], {w1.getnewaddress(): 5.9999})
        tx11ob = node.decoderawtransaction(tx11)
        tx12 = node.createrawtransaction([{"txid": tx6ob["vin"][0]["txid"], "vout": tx6ob["vin"][0]["vout"]}], {w1.getnewaddress(): 5.9999})
        tx12ob = node.decoderawtransaction(tx12)
        details = [
            {
                "txid" : tx9ob["txid"],
                "wallet_conflicts" : [tx6ob["txid"], tx7ob["txid"], tx8ob["txid"]],
                "simulated_tx_conflicts" : [1, 2, 3]
            },
            {
                "txid" : tx10ob["txid"],
                "wallet_conflicts" : [tx6ob["txid"]],
                "simulated_tx_conflicts" : [0, 2, 3]
            },
            {
                "txid" : tx11ob["txid"],
                "wallet_conflicts" : [tx6ob["txid"]],
                "simulated_tx_conflicts" : [0, 1, 3]
            },
            {
                "txid" : tx12ob["txid"],
                "wallet_conflicts" : [tx6ob["txid"]],
                "simulated_tx_conflicts" : [0, 1 ,2]
            },
        ]
        sim_result =  w0.simulaterawtransaction([tx9, tx10, tx11, tx12])["details"]
        sim_result[0]["wallet_conflicts"].sort()
        details[0]["wallet_conflicts"].sort()
        assert_equal(sim_result, details)

if __name__ == '__main__':
    SimulateTxTest().main()

