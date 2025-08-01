#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test how the wallet deals with TRUC transactions"""

from decimal import Decimal, getcontext

from test_framework.authproxy import JSONRPCException
from test_framework.messages import (
    COIN,
    CTransaction,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_RETURN
)

from test_framework.script_util import bulk_vout

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)

from test_framework.mempool_util import (
    TRUC_MAX_VSIZE,
    TRUC_CHILD_MAX_VSIZE,
)

def cleanup(func):
    def wrapper(self, *args):
        try:
            func(self, *args)
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

    def send_tx(self, from_node, inputs, outputs, version):
        raw_tx = from_node.createrawtransaction(inputs=inputs, outputs=outputs, version=version)
        if inputs == []:
            raw_tx = from_node.fundrawtransaction(raw_tx, {'include_unsafe' : True})["hex"]
        raw_tx = from_node.signrawtransactionwithwallet(raw_tx)["hex"]
        txid = from_node.sendrawtransaction(raw_tx)
        self.sync_mempools()
        return txid

    def bulk_tx(self, tx, amount, target_vsize):
        tx.vout.append(CTxOut(nValue=(amount * COIN), scriptPubKey=CScript([OP_RETURN])))
        bulk_vout(tx, target_vsize)

    def run_test_with_swapped_versions(self, test_func):
        test_func(2, 3)
        test_func(3, 2)

    def run_test(self):
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)

        self.nodes[0].createwallet("alice")
        self.alice = self.nodes[0].get_wallet_rpc("alice")

        self.nodes[1].createwallet("bob")
        self.bob = self.nodes[1].get_wallet_rpc("bob")

        self.nodes[2].createwallet("charlie")
        self.charlie = self.nodes[2].get_wallet_rpc("charlie")

        self.generatetoaddress(self.nodes[0], 100, self.charlie.getnewaddress())

        self.run_test_with_swapped_versions(self.tx_spends_unconfirmed_tx_with_wrong_version)
        self.run_test_with_swapped_versions(self.v2_tx_spends_confirmed_v3_tx)
        self.run_test_with_swapped_versions(self.spend_inputs_with_different_versions)
        self.spend_inputs_with_different_versions_default_version()
        self.v3_utxos_appear_in_listunspent()
        self.truc_tx_with_conflicting_sibling()
        self.truc_tx_with_conflicting_sibling_change()
        self.v3_tx_evicted_from_mempool_by_sibling()
        self.v3_conflict_removed_from_mempool()
        self.mempool_conflicts_removed_when_v3_conflict_removed()
        self.max_tx_weight()
        self.max_tx_child_weight()
        self.user_input_weight_not_overwritten()
        self.createpsbt_v3()
        self.send_v3()
        self.sendall_v3()
        self.sendall_with_unconfirmed_v3()
        self.walletcreatefundedpsbt_v3()

    @cleanup
    def tx_spends_unconfirmed_tx_with_wrong_version(self, version_a, version_b):
        self.log.info(f"Test unavailable funds when v{version_a} tx spends unconfirmed v{version_b} tx")

        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, inputs, outputs, version_a)

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 0)

        inputs = []
        outputs = {self.alice.getnewaddress() : 1.0}

        raw_tx_v2 = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=version_b)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            raw_tx_v2, {'include_unsafe': True}
        )

    @cleanup
    def v2_tx_spends_confirmed_v3_tx(self, version_a, version_b):
        self.log.info(f"Test unavailable funds when v{version_a} tx spends confirmed v{version_b} tx")

        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, inputs, outputs, version_a)

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 0)

        inputs = []
        outputs = {self.alice.getnewaddress() : 1.0}

        self.generate(self.nodes[2], 1)

        self.send_tx(self.bob, inputs, outputs, version_b)

    @cleanup
    def v3_utxos_appear_in_listunspent(self):
        self.log.info("Test that unconfirmed v3 utxos still appear in listunspent")

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0}
        parent_txid = self.send_tx(self.charlie, inputs, outputs, 3)
        assert_equal(self.alice.listunspent(minconf=0)[0]["txid"], parent_txid)

    @cleanup
    def truc_tx_with_conflicting_sibling(self):
        # unconfirmed v3 tx to alice & bob
        self.log.info("Test v3 transaction with conflicting sibling")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, inputs, outputs, 3)
        parent_txid = self.charlie.getrawmempool()[0]

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)}
        self.send_tx(self.alice, inputs, outputs, 3)

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
    def truc_tx_with_conflicting_sibling_change(self):
        # unconfirmed v3 tx to alice & bob
        self.log.info("Test v3 transaction with conflicting sibling")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, inputs, outputs, 3)
        parent_txid = self.charlie.getrawmempool()[0]

        # bob spends her output with a v3 transaction
        bob_unspent = self.bob.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : bob_unspent['vout']},]
        outputs = {self.bob.getnewaddress() : bob_unspent['amount'] - Decimal(0.00000120)} # two outputs
        self.send_tx(self.bob, inputs, outputs, 3)

        # alice tries to spend money
        inputs=[]
        outputs = {self.alice.getnewaddress() : 1.999}
        alice_tx = self.bob.createrawtransaction(inputs=inputs, outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.alice.fundrawtransaction,
            alice_tx
        )

    @cleanup
    def spend_inputs_with_different_versions(self, version_a, version_b):
        self.log.info(f"Test spending a pre-selected v{version_a} input with a v{version_b} transaction")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0}
        parent_txid = self.send_tx(self.charlie, inputs, outputs, version_a)

        # alice spends her output
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs, version=version_b)

        assert_raises_rpc_error(
            -4,
            f"Can't spend unconfirmed version {version_a} pre-selected input with a version {version_b} tx",
            self.alice.fundrawtransaction,
            alice_tx
        )

    @cleanup
    def spend_inputs_with_different_versions_default_version(self):
        self.log.info("Test spending a pre-selected v3 input with a v2 transaction")
        self.generate(self.nodes[2], 1)

        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0}
        parent_txid = self.send_tx(self.charlie, inputs, outputs, 3)

        # alice spends her output
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_tx = self.alice.createrawtransaction(inputs=inputs, outputs=outputs) # don't set the version here

        assert_raises_rpc_error(
            -4,
            "Can't spend unconfirmed version 3 pre-selected input with a version 2 tx",
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
        parent_txid = self.send_tx(self.charlie, inputs, outputs, 3)

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)} # two outputs
        alice_txid = self.send_tx(self.alice, inputs, outputs, 3)

        # bob tries to spend money
        bob_unspent = self.bob.listunspent(minconf=0)[0]
        inputs=[{'txid' : parent_txid, 'vout' : bob_unspent['vout']},]
        outputs = {self.bob.getnewaddress() : bob_unspent['amount'] - Decimal(0.00010120)} # two outputs
        bob_txid = self.send_tx(self.bob, inputs, outputs, 3)

        assert_equal(self.alice.gettransaction(alice_txid)['mempoolconflicts'], [bob_txid])

        self.log.info("Test that re-submitting Alice's transaction with a higher fee removes bob's tx as a mempool conflict")
        inputs=[{'txid' : parent_txid, 'vout' : alice_unspent['vout']},]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00030120)} # two outputs
        alice_txid = self.send_tx(self.alice, inputs, outputs, 3)
        assert_equal(self.alice.gettransaction(alice_txid)['mempoolconflicts'], [])

    @cleanup
    def v3_conflict_removed_from_mempool(self):
        self.log.info("Test a v3 conflict being removed")
        self.generate(self.nodes[2], 1)
        # send a v2 output to alice and confirm it
        txid = self.charlie.sendall([self.alice.getnewaddress()])["txid"]
        assert_equal(self.charlie.gettransaction(txid, verbose=True)["decoded"]["version"], 2)
        self.generate(self.nodes[2], 1)
        # create a v3 tx to alice and bob
        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, inputs, outputs, 3)

        alice_v2_unspent = self.alice.listunspent(minconf=1)[0]
        alice_unspent = [unspent for unspent in self.alice.listunspent(minconf=0) if unspent["confirmations"] == 0][0]

        # alice spends both of her outputs
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0}, {'txid' : alice_unspent['txid'], 'vout' : alice_unspent['vout']}]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] + alice_unspent['amount'] - Decimal(0.00005120)}
        self.send_tx(self.alice, inputs, outputs, 3)
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
        self.send_tx(self.alice, inputs, outputs, 2)
        # bob can now create a transaction
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        self.send_tx(self.bob, inputs, outputs, 3)

    @cleanup
    def mempool_conflicts_removed_when_v3_conflict_removed(self):
        # send a v2 output to alice and confirm it
        txid = self.charlie.sendall([self.alice.getnewaddress()])["txid"]
        assert_equal(self.charlie.gettransaction(txid, verbose=True)["decoded"]["version"], 2)
        self.generate(self.nodes[2], 1)
        # create a v3 tx to alice and bob
        inputs=[]
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, inputs, outputs, 3)

        alice_v2_unspent = self.alice.listunspent(minconf=1)[0]
        alice_unspent = [unspent for unspent in self.alice.listunspent(minconf=0) if unspent["confirmations"] == 0][0]
        # bob spends his utxo
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_txid = self.send_tx(self.bob, inputs, outputs, 3)
        # alice spends both of her utxos, replacing bob's tx
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0}, {'txid' : alice_unspent['txid'], 'vout' : alice_unspent['vout']}]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] + alice_unspent['amount'] - Decimal(0.00005120)}
        alice_txid = self.send_tx(self.alice, inputs, outputs, 3)
        self.sync_mempools()
        # bob's tx now has a mempool conflict
        assert_equal(self.bob.gettransaction(bob_txid)['mempoolconflicts'], [alice_txid])
        # alice fee-bumps her tx so it only spends the v2 utxo
        inputs = [{'txid' : alice_v2_unspent['txid'], 'vout' : 0},]
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] - Decimal(0.00015120)}
        self.send_tx(self.alice, inputs, outputs, 2)
        # bob's tx now has non conflicts and can be rebroadcast
        assert_equal(self.bob.gettransaction(bob_txid)['mempoolconflicts'], [])

    @cleanup
    def max_tx_weight(self):
        self.log.info("Test max v3 transaction weight.")
        self.generate(self.nodes[2], 1)

        tx = CTransaction()
        tx.version = 3 # make this a truc tx
        # increase tx weight almost to the max truc size
        self.bulk_tx(tx, 5, TRUC_MAX_VSIZE - 100)

        assert_raises_rpc_error(
            -4,
            "The inputs size exceeds the maximum weight. Please try sending a smaller amount or manually consolidating your wallet's UTXOs",
            self.charlie.fundrawtransaction,
            tx.serialize_with_witness().hex(),
            {'include_unsafe' : True}
        )

        tx.version = 2
        self.charlie.fundrawtransaction(tx.serialize_with_witness().hex())

    @cleanup
    def max_tx_child_weight(self):
        self.log.info("Test max v3 transaction child weight.")
        self.generate(self.nodes[2], 1)

        inputs = []
        outputs = {self.alice.getnewaddress() : 10}
        self.send_tx(self.charlie, inputs, outputs, 3)

        tx = CTransaction()
        tx.version = 3

        self.bulk_tx(tx, 5, TRUC_CHILD_MAX_VSIZE - 100)

        assert_raises_rpc_error(
            -4,
            "The inputs size exceeds the maximum weight. Please try sending a smaller amount or manually consolidating your wallet's UTXOs",
            self.alice.fundrawtransaction,
            tx.serialize_with_witness().hex(),
            {'include_unsafe' : True}
        )

        self.generate(self.nodes[2], 1)
        self.alice.fundrawtransaction(tx.serialize_with_witness().hex())

    @cleanup
    def user_input_weight_not_overwritten(self):
        self.log.info("Test that the user-input tx weight is not overwritten by the truc maximum")
        self.generate(self.nodes[2], 1)

        tx = CTransaction()
        tx.version = 3

        self.bulk_tx(tx, 5, int(TRUC_MAX_VSIZE/2))

        assert_raises_rpc_error(
            -4,
            "Maximum transaction weight is less than transaction weight without inputs",
            self.charlie.fundrawtransaction,
            tx.serialize_with_witness().hex(),
            {'include_unsafe' : True, 'max_tx_weight' : int(TRUC_MAX_VSIZE/2)}
        )

    @cleanup
    def createpsbt_v3(self):
        self.log.info("Test setting version to 3 with createpsbt")
        self.generate(self.nodes[2], 1)

        inputs = []
        outputs = {self.alice.getnewaddress() : 10}
        psbt = self.charlie.createpsbt(inputs=inputs, outputs=outputs, version=3)
        assert_equal(self.charlie.decodepsbt(psbt)["tx"]["version"], 3)

    @cleanup
    def send_v3(self):
        self.log.info("Test setting version to 3 with `send`")
        self.generate(self.nodes[2], 1)

        outputs = {self.alice.getnewaddress() : 10}
        tx_hex = self.charlie.send(outputs=outputs, add_to_wallet=False, version=3)["hex"]
        assert_equal(self.charlie.decoderawtransaction(tx_hex)["version"], 3)

    @cleanup
    def sendall_v3(self):
        self.log.info("Test setting version to 3 with sendall")
        self.generate(self.nodes[2], 1)

        tx_hex = self.charlie.sendall(recipients=[self.alice.getnewaddress()], version=3, add_to_wallet=False)["hex"]
        assert_equal(self.charlie.decoderawtransaction(tx_hex)["version"], 3)

    @cleanup
    def sendall_with_unconfirmed_v3(self):
        self.log.info("Test setting version to 3 with sendall + unconfirmed inputs")

        inputs=[]

        # confirmed v2 utxos
        outputs = {self.charlie.getnewaddress() : 2.0}
        confirmed_v2 = self.send_tx(self.charlie, inputs, outputs, 2)

        # confirmed v3 utxos
        outputs = {self.charlie.getnewaddress() : 2.0}
        confirmed_v3 = self.send_tx(self.charlie, inputs, outputs, 3)

        self.generate(self.nodes[2], 1)

        # unconfirmed v2 utxos
        outputs = {self.charlie.getnewaddress() : 2.0}
        unconfirmed_v2 = self.send_tx(self.charlie, inputs, outputs, 2)

        # unconfirmed v3 utxos
        outputs = {self.charlie.getnewaddress() : 2.0}
        unconfirmed_v3 = self.send_tx(self.charlie, inputs, outputs, 3)

        # Test that the only unconfirmed inputs this v3 tx spends are v3
        tx_hex = self.charlie.sendall([self.bob.getnewaddress()], version=3, add_to_wallet=False)["hex"]

        decoded_tx = self.charlie.decoderawtransaction(tx_hex)
        decoded_vin_txids = [txin["txid"] for txin in decoded_tx["vin"]]

        assert_equal(decoded_tx["version"], 3)

        assert confirmed_v3 in decoded_vin_txids
        assert confirmed_v2 in decoded_vin_txids
        assert unconfirmed_v3 in decoded_vin_txids
        assert unconfirmed_v2 not in decoded_vin_txids

        # Test that the only unconfirmed inputs this v2 tx spends are v2
        tx_hex = self.charlie.sendall([self.bob.getnewaddress()], version=2, add_to_wallet=False)["hex"]

        decoded_tx = self.charlie.decoderawtransaction(tx_hex)
        decoded_vin_txids = [txin["txid"] for txin in decoded_tx["vin"]]

        assert_equal(decoded_tx["version"], 2)

        assert confirmed_v3 in decoded_vin_txids
        assert confirmed_v2 in decoded_vin_txids
        assert unconfirmed_v2 in decoded_vin_txids
        assert unconfirmed_v3 not in decoded_vin_txids

    @cleanup
    def walletcreatefundedpsbt_v3(self):
        self.log.info("Test setting version to 3 with walletcreatefundedpsbt")
        self.log.info("Test setting version to 3 with createpsbt")
        self.generate(self.nodes[2], 1)

        inputs = []
        outputs = {self.alice.getnewaddress() : 10}
        psbt = self.charlie.walletcreatefundedpsbt(inputs=inputs, outputs=outputs, version=3)["psbt"]
        assert_equal(self.charlie.decodepsbt(psbt)["tx"]["version"], 3)

if __name__ == '__main__':
    WalletV3Test(__file__).main()
