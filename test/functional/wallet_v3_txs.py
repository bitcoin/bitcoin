#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test how the wallet deals with v3 transactions"""

from decimal import Decimal, getcontext

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)

def cleanup(func):
    def wrapper(self):
        try:
            func(self)
        finally:
            self.sync_mempools()
            self.generate(self.nodes[2], 1)
            try:
                self.alice.sendall([self.charlie.getnewaddress()])
            except JSONRPCException as e:
                assert "Total value of UTXO pool too low to pay for transaction" in e.error['message']
            try:
                self.bob.sendall([self.charlie.getnewaddress()])
            except JSONRPCException as e:
                assert "Total value of UTXO pool too low to pay for transaction" in e.error['message']
            self.sync_mempools()
            self.generate(self.nodes[2], 1)
            assert_equal(0, self.alice.getbalances()["mine"]["untrusted_pending"])
            assert_equal(0, self.bob.getbalances()["mine"]["untrusted_pending"])
            assert_equal(0, self.alice.getbalances()["mine"]["trusted"])
            assert_equal(0, self.bob.getbalances()["mine"]["trusted"])
            assert_equal(0, self.alice.getbalances()["mine"]["immature"])
            assert_equal(0, self.bob.getbalances()["mine"]["immature"])
            assert_equal(self.alice.getrawmempool(), [])
            assert_equal(self.bob.getrawmempool(), [])

    return wrapper

class WalletV3Test(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        getcontext().prec=10
        self.num_nodes = 3
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

    def run_test(self):
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.connect_nodes(1, 2)

        self.nodes[0].createwallet("alice")
        self.alice = self.nodes[0].get_wallet_rpc("alice")

        self.nodes[1].createwallet("bob")
        self.bob = self.nodes[1].get_wallet_rpc("bob")

        self.nodes[2].createwallet("charlie")
        self.charlie = self.nodes[2].get_wallet_rpc("charlie")

        self.generatetoaddress(self.nodes[0], 100, self.charlie.getnewaddress())

        self.v3_tx_spends_unconfirmed_v2_tx()
        self.v2_tx_spends_unconfirmed_v3_tx()
        self.v3_utxos_appear_in_listunspent()
        self.truc_tx_with_conflicting_sibling()
        self.spend_v3_input_with_v2()
        self.spend_v2_input_with_v3()
        self.v3_tx_evicted_from_mempool_by_sibling()
        self.v3_conflict_removed_from_mempool()
        self.mempool_conflicts_removed_when_v3_conflict_removed()

    @cleanup
    def v3_tx_spends_unconfirmed_v2_tx(self):
        self.log.info("Test unavailable funds when v3 tx spends unconfirmed v2 tx")

        self.generate(self.nodes[2], 1)

        # by default, sendall uses tx version 2
        self.charlie.sendall([self.bob.getnewaddress()])
        assert_equal(self.charlie.getbalances()["mine"]["trusted"], 0)

        self.sync_mempools()

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 49)

        inputs = []
        outputs = {self.alice.getnewaddress() : 1.0}

        raw_tx_v3 = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            raw_tx_v3, {'include_unsafe': True}
        )

    @cleanup
    def v2_tx_spends_unconfirmed_v3_tx(self):
        self.log.info("Test unavailable funds when v3 tx spends unconfirmed v2 tx")

        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.bob.getnewaddress() : 2.0}
        raw_tx_v3 = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        raw_tx_v3 = self.charlie.fundrawtransaction(raw_tx_v3)["hex"]
        raw_tx_v3 = self.charlie.signrawtransactionwithwallet(raw_tx_v3)["hex"]
        self.charlie.sendrawtransaction(raw_tx_v3)

        self.sync_mempools()

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 0)

        inputs = []
        outputs = {self.alice.getnewaddress() : 1.0}

        raw_tx_v2 = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=2)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            raw_tx_v2, {'include_unsafe': True}
        )

    @cleanup
    def v3_utxos_appear_in_listunspent(self):
        self.log.info("Test that unconfirmed v3 utxos still appear in listunspent")

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        parent_tx = self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()
        assert_equal(self.alice.listunspent(minconf=0)[0]["txid"], parent_tx)

    @cleanup
    def truc_tx_with_conflicting_sibling(self):
        # unconfirmed v3 tx to alice & bob
        self.log.info("Test v3 transaction with conflicting sibling")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()
        parent_txid = self.charlie.getrawmempool()[0]

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        alice_tx = self.alice.signrawtransactionwithwallet(alice_tx)

        self.alice.sendrawtransaction(alice_tx["hex"])
        self.sync_mempools()

        # bob tries to spend money
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_tx = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            bob_tx, {'include_unsafe': True}
        )

    @cleanup
    def spend_v3_input_with_v2(self):
        self.log.info("Test spending a pre-selected v3 input with a v2 transaction")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()
        parent_txid = self.charlie.getrawmempool()[0]

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=2)

        assert_raises_rpc_error(
            -4,
            "Can't spend unconfirmed version 3 pre-selected input with a version 2 tx",
            self.alice.fundrawtransaction,
            alice_tx
        )

    @cleanup
    def spend_v2_input_with_v3(self):
        self.log.info("Test spending a pre-selected v2 input with a v3 transaction")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=2)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()
        parent_txid = self.charlie.getrawmempool()[0]

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Can't spend unconfirmed version 2 pre-selected input with a version 3 tx",
            self.alice.fundrawtransaction,
            alice_tx
        )

    @cleanup
    def v3_tx_evicted_from_mempool_by_sibling(self):
        # unconfirmed v3 tx to alice & bob
        self.log.info("Test v3 transaction with conflicting sibling")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()
        parent_txid = self.charlie.getrawmempool()[0]

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        alice_tx = self.alice.signrawtransactionwithwallet(alice_tx)

        alice_txid = self.alice.sendrawtransaction(alice_tx["hex"])
        self.sync_mempools()

        # bob tries to spend money
        bob_unspent = self.bob.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : bob_unspent['vout']},]
        outputs = {self.bob.getnewaddress() : bob_unspent['amount'] - Decimal(0.00010120)} # two outputs
        bob_tx = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        bob_tx = self.bob.signrawtransactionwithwallet(bob_tx)

        bob_txid = self.alice.sendrawtransaction(bob_tx["hex"])

        assert_equal(self.alice.gettransaction(alice_txid)['mempoolconflicts'], [bob_txid])

    @cleanup
    def v3_conflict_removed_from_mempool(self):
        self.log.info("Test a v3 conflict being removed")
        self.generate(self.nodes[2], 1)
        # send a v2 output to alice
        self.charlie.sendall([self.alice.getnewaddress()])["txid"]
        self.generate(self.nodes[2], 1)
        # create a v3 tx to alice and bob
        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()

        alice_v2_unspent = [unspent for unspent in self.alice.listunspent(minconf=0) if unspent["confirmations"] != 0][0]
        alice_unspent = [unspent for unspent in self.alice.listunspent(minconf=0) if unspent["confirmations"] == 0][0]
        bob_unspent = self.bob.listunspent(minconf=0)[0]

        # alice spends both of her outputs
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0}, {'txid' : alice_unspent['txid'], 'vout' : alice_unspent['vout']}]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] + alice_unspent['amount'] - Decimal(0.00005120)}
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        alice_tx = self.alice.signrawtransactionwithwallet(alice_tx)["hex"]
        self.alice.sendrawtransaction(alice_tx)
        self.sync_mempools()
        # bob can't create a transaction
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_tx = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            bob_tx, {'include_unsafe': True}
        )
        # alice fee-bumps her tx so it only spends the v2 utxo
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0},]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] - Decimal(0.00015120)}
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=2)
        alice_tx = self.alice.signrawtransactionwithwallet(alice_tx)["hex"]
        self.alice.sendrawtransaction(alice_tx)
        self.sync_mempools()
        # bob can now create a transaction
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_tx = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        bob_tx = self.bob.fundrawtransaction(bob_tx, {'include_unsafe': True})["hex"]
        bob_tx = self.bob.signrawtransactionwithwallet(bob_tx)["hex"]
        self.bob.sendrawtransaction(bob_tx)

    @cleanup
    def mempool_conflicts_removed_when_v3_conflict_removed(self):
        # send a v2 output to alice
        self.charlie.sendall([self.alice.getnewaddress()])["txid"]
        self.generate(self.nodes[2], 1)
        # create a v3 tx to alice and bob
        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        parent_tx = self.charlie.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        parent_tx = self.charlie.fundrawtransaction(parent_tx)
        parent_tx = self.charlie.signrawtransactionwithwallet(parent_tx["hex"])
        self.charlie.sendrawtransaction(parent_tx["hex"])
        self.sync_mempools()

        alice_v2_unspent = [unspent for unspent in self.alice.listunspent(minconf=0) if unspent["confirmations"] != 0][0]
        alice_unspent = [unspent for unspent in self.alice.listunspent(minconf=0) if unspent["confirmations"] == 0][0]
        bob_unspent = self.bob.listunspent(minconf=0)[0]
        # bob spends his utxo
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_tx = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        bob_tx = self.bob.fundrawtransaction(bob_tx, {'include_unsafe': True})["hex"]
        bob_tx = self.bob.signrawtransactionwithwallet(bob_tx)["hex"]
        bob_txid = self.bob.sendrawtransaction(bob_tx)
        # alice spends both of her utxos, replacing bob's tx
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0}, {'txid' : alice_unspent['txid'], 'vout' : alice_unspent['vout']}]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] + alice_unspent['amount'] - Decimal(0.00005120)}
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=3)
        alice_tx = self.alice.signrawtransactionwithwallet(alice_tx)["hex"]
        alice_txid = self.alice.sendrawtransaction(alice_tx)
        self.sync_mempools()
        # bob's tx now has a mempool conflict
        assert_equal(self.bob.gettransaction(bob_txid)['mempoolconflicts'], [alice_txid])
        # alice fee-bumps her tx so it only spends the v2 utxo
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0},]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] - Decimal(0.00015120)}
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=2)
        alice_tx = self.alice.signrawtransactionwithwallet(alice_tx)["hex"]
        self.alice.sendrawtransaction(alice_tx)
        self.sync_mempools()
        # bob's tx now has non conflicts and can be rebroadcast
        assert_equal(self.bob.gettransaction(bob_txid)['mempoolconflicts'], [])

if __name__ == '__main__':
    WalletV3Test(__file__).main()
