#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
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

from test_framework.blocktools import (
    create_empty_fork,
)

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

# sweep alice and bob's wallets and clear the mempool
def cleanup(func):
    def wrapper(self, *args):
        try:
            self.generate(self.nodes[0], 1)
            func(self, *args)
        finally:
            self.generate(self.nodes[0], 1)
            try:
                self.alice.sendall([self.charlie.getnewaddress()])
            except JSONRPCException as e:
                assert "Total value of UTXO pool too low to pay for transaction" in e.error['message']
            try:
                self.bob.sendall([self.charlie.getnewaddress()])
            except JSONRPCException as e:
                assert "Total value of UTXO pool too low to pay for transaction" in e.error['message']
            self.generate(self.nodes[0], 1)

            for wallet in [self.alice, self.bob]:
                balance = wallet.getbalances()["mine"]
                for balance_type in ["untrusted_pending", "trusted", "immature"]:
                    assert_equal(balance[balance_type], 0)

            assert_equal(self.alice.getrawmempool(), [])
            assert_equal(self.bob.getrawmempool(), [])

    return wrapper

class WalletV3Test(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        getcontext().prec=10
        self.num_nodes = 1
        self.setup_clean_chain = True

    def send_tx(self, from_wallet, inputs, outputs, version):
        raw_tx = from_wallet.createrawtransaction(inputs=inputs, outputs=outputs, version=version)
        if inputs == []:
            raw_tx = from_wallet.fundrawtransaction(raw_tx, {'include_unsafe' : True})["hex"]
        raw_tx = from_wallet.signrawtransactionwithwallet(raw_tx)["hex"]
        txid = from_wallet.sendrawtransaction(raw_tx)
        return txid

    def bulk_tx(self, tx, amount, target_vsize):
        tx.vout.append(CTxOut(nValue=(amount * COIN), scriptPubKey=CScript([OP_RETURN])))
        bulk_vout(tx, target_vsize)

    def run_test_with_swapped_versions(self, test_func):
        test_func(2, 3)
        test_func(3, 2)

    def trigger_reorg(self, fork_blocks):
        """Trigger reorg of the fork blocks."""
        for block in fork_blocks:
            self.nodes[0].submitblock(block.serialize().hex())
        assert_equal(self.nodes[0].getbestblockhash(), fork_blocks[-1].hash_hex)

    def run_test(self):
        self.nodes[0].createwallet("alice")
        self.alice = self.nodes[0].get_wallet_rpc("alice")

        self.nodes[0].createwallet("bob")
        self.bob = self.nodes[0].get_wallet_rpc("bob")

        self.nodes[0].createwallet("charlie")
        self.charlie = self.nodes[0].get_wallet_rpc("charlie")

        self.generatetoaddress(self.nodes[0], 100, self.charlie.getnewaddress())

        self.run_test_with_swapped_versions(self.tx_spends_unconfirmed_tx_with_wrong_version)
        self.run_test_with_swapped_versions(self.va_tx_spends_confirmed_vb_tx)
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
        self.user_input_weight_not_overwritten_v3_child()
        self.createpsbt_v3()
        self.send_v3()
        self.sendall_v3()
        self.sendall_with_unconfirmed_v3()
        self.walletcreatefundedpsbt_v3()
        self.sendall_truc_weight_limit()
        self.sendall_truc_child_weight_limit()
        self.mix_non_truc_versions()
        self.cant_spend_multiple_unconfirmed_truc_outputs()
        self.test_spend_third_generation()
        self.test_coins_availability_reorg()

    @cleanup
    def tx_spends_unconfirmed_tx_with_wrong_version(self, version_a, version_b):
        self.log.info(f"Test unavailable funds when v{version_b} tx spends unconfirmed v{version_a} tx")

        outputs = {self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, version_a)

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 0)

        outputs = {self.alice.getnewaddress() : 1.0}

        raw_tx_v2 = self.bob.createrawtransaction(inputs=[], outputs=outputs, version=version_b)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            raw_tx_v2, {'include_unsafe': True}
        )

    @cleanup
    def va_tx_spends_confirmed_vb_tx(self, version_a, version_b):
        self.log.info(f"Test available funds when v{version_b} tx spends confirmed v{version_a} tx")

        outputs = {self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, version_a)

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 0)

        outputs = {self.alice.getnewaddress() : 1.0}

        self.generate(self.nodes[0], 1)

        self.send_tx(self.bob, [], outputs, version_b)

    @cleanup
    def v3_utxos_appear_in_listunspent(self):
        self.log.info("Test that unconfirmed v3 utxos still appear in listunspent")

        outputs = {self.alice.getnewaddress() : 2.0}
        parent_txid = self.send_tx(self.charlie, [], outputs, 3)
        assert_equal(self.alice.listunspent(minconf=0)[0]["txid"], parent_txid)

    @cleanup
    def truc_tx_with_conflicting_sibling(self):
        self.log.info("Test v3 transaction with conflicting sibling")

        # unconfirmed v3 tx to alice & bob
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 3)

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)}
        self.send_tx(self.alice, [alice_unspent], outputs, 3)

        # bob tries to spend money
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_tx = self.bob.createrawtransaction(inputs=[], outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            bob_tx, {'include_unsafe': True}
        )

    @cleanup
    def truc_tx_with_conflicting_sibling_change(self):
        self.log.info("Test v3 transaction with conflicting sibling change")

        outputs = {self.alice.getnewaddress() : 8.0}
        self.send_tx(self.charlie, [], outputs, 3)

        self.generate(self.nodes[0], 1)

        # unconfirmed v3 tx to alice & bob
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.alice, [], outputs, 3)

        # bob spends his output with a v3 transaction
        bob_unspent = self.bob.listunspent(minconf=0)[0]
        outputs = {self.bob.getnewaddress() : bob_unspent['amount'] - Decimal(0.00000120)}
        self.send_tx(self.bob, [bob_unspent], outputs, 3)

        # alice tries to spend money
        outputs = {self.alice.getnewaddress() : 1.999}
        alice_tx = self.alice.createrawtransaction(inputs=[], outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.alice.fundrawtransaction,
            alice_tx, {'include_unsafe': True}
        )

    @cleanup
    def spend_inputs_with_different_versions(self, version_a, version_b):
        self.log.info(f"Test spending a pre-selected v{version_a} input with a v{version_b} transaction")

        outputs = {self.alice.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, version_a)

        # alice spends her output
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)}
        alice_tx = self.alice.createrawtransaction(inputs=[alice_unspent], outputs=outputs, version=version_b)

        assert_raises_rpc_error(
            -4,
            f"Can't spend unconfirmed version {version_a} pre-selected input with a version {version_b} tx",
            self.alice.fundrawtransaction,
            alice_tx
        )

    @cleanup
    def spend_inputs_with_different_versions_default_version(self):
        self.log.info("Test spending a pre-selected v3 input with the default version of transaction")

        outputs = {self.alice.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 3)

        # alice spends her output
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - Decimal(0.00000120)}
        alice_tx = self.alice.createrawtransaction(inputs=[alice_unspent], outputs=outputs) # don't set the version here

        assert_raises_rpc_error(
            -4,
            "Can't spend unconfirmed version 3 pre-selected input with a version 2 tx",
            self.alice.fundrawtransaction,
            alice_tx
        )

    @cleanup
    def v3_tx_evicted_from_mempool_by_sibling(self):
        self.log.info("Test v3 transaction evicted because of conflicting sibling")

        # unconfirmed v3 tx to alice & bob
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 3)

        # alice spends her output with a v3 transaction
        alice_unspent = self.alice.listunspent(minconf=0)[0]
        alice_fee = Decimal(0.00000120)
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - alice_fee}
        alice_txid = self.send_tx(self.alice, [alice_unspent], outputs, 3)

        # bob tries to spend money
        bob_unspent = self.bob.listunspent(minconf=0)[0]
        outputs = {self.bob.getnewaddress() : bob_unspent['amount'] - Decimal(0.00010120)}
        bob_txid = self.send_tx(self.bob, [bob_unspent], outputs, 3)

        assert_equal(self.alice.gettransaction(alice_txid)['mempoolconflicts'], [bob_txid])

        self.log.info("Test that re-submitting Alice's transaction with a higher fee removes bob's tx as a mempool conflict")
        fee_delta = Decimal(0.00030120)
        outputs = {self.alice.getnewaddress() : alice_unspent['amount'] - fee_delta}
        alice_txid = self.send_tx(self.alice, [alice_unspent], outputs, 3)
        assert_equal(self.alice.gettransaction(alice_txid)['mempoolconflicts'], [])

    @cleanup
    def v3_conflict_removed_from_mempool(self):
        self.log.info("Test a v3 conflict being removed")
        # send a v2 output to alice and confirm it
        txid = self.charlie.sendall([self.alice.getnewaddress()])["txid"]
        assert_equal(self.charlie.gettransaction(txid, verbose=True)["decoded"]["version"], 2)
        self.generate(self.nodes[0], 1)
        # create a v3 tx to alice and bob
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 3)

        alice_v2_unspent = self.alice.listunspent(minconf=1)[0]
        alice_unspent = self.alice.listunspent(minconf=0, maxconf=0)[0]

        # alice spends both of her outputs
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] + alice_unspent['amount'] - Decimal(0.00005120)}
        self.send_tx(self.alice, [alice_v2_unspent, alice_unspent], outputs, 3)
        # bob can't create a transaction
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_tx = self.bob.createrawtransaction(inputs=[], outputs=outputs, version=3)

        assert_raises_rpc_error(
            -4,
            "Insufficient funds",
            self.bob.fundrawtransaction,
            bob_tx, {'include_unsafe': True}
        )
        # alice fee-bumps her tx so it only spends the v2 utxo
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] - Decimal(0.00015120)}
        self.send_tx(self.alice, [alice_v2_unspent], outputs, 2)
        # bob can now create a transaction
        outputs = {self.bob.getnewaddress() : 1.999}
        self.send_tx(self.bob, [], outputs, 3)

    @cleanup
    def mempool_conflicts_removed_when_v3_conflict_removed(self):
        self.log.info("Test that we remove v3 txs from mempool_conflicts correctly")
        # send a v2 output to alice and confirm it
        txid = self.charlie.sendall([self.alice.getnewaddress()])["txid"]
        assert_equal(self.charlie.gettransaction(txid, verbose=True)["decoded"]["version"], 2)
        self.generate(self.nodes[0], 1)
        # create a v3 tx to alice and bob
        outputs = {self.alice.getnewaddress() : 2.0, self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 3)

        alice_v2_unspent = self.alice.listunspent(minconf=1)[0]
        alice_unspent = self.alice.listunspent(minconf=0, maxconf=0)[0]
        # bob spends his utxo
        inputs=[]
        outputs = {self.bob.getnewaddress() : 1.999}
        bob_txid = self.send_tx(self.bob, inputs, outputs, 3)
        # alice spends both of her utxos, replacing bob's tx
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] + alice_unspent['amount'] - Decimal(0.00005120)}
        alice_txid = self.send_tx(self.alice, [alice_v2_unspent, alice_unspent], outputs, 3)
        # bob's tx now has a mempool conflict
        assert_equal(self.bob.gettransaction(bob_txid)['mempoolconflicts'], [alice_txid])
        # alice fee-bumps her tx so it only spends the v2 utxo
        outputs = {self.charlie.getnewaddress() : alice_v2_unspent['amount'] - Decimal(0.00015120)}
        self.send_tx(self.alice, [alice_v2_unspent], outputs, 2)
        # bob's tx now has non conflicts and can be rebroadcast
        bob_tx = self.bob.gettransaction(bob_txid)
        assert_equal(bob_tx['mempoolconflicts'], [])
        self.bob.sendrawtransaction(bob_tx['hex'])

    @cleanup
    def max_tx_weight(self):
        self.log.info("Test max v3 transaction weight.")

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

        outputs = {self.alice.getnewaddress() : 10}
        self.send_tx(self.charlie, [], outputs, 3)

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

        self.generate(self.nodes[0], 1)
        self.alice.fundrawtransaction(tx.serialize_with_witness().hex())

    @cleanup
    def user_input_weight_not_overwritten(self):
        self.log.info("Test that the user-input tx weight is not overwritten by the truc maximum")

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
    def user_input_weight_not_overwritten_v3_child(self):
        self.log.info("Test that the user-input tx weight is not overwritten by the truc child maximum")

        outputs = {self.alice.getnewaddress() : 10}
        self.send_tx(self.charlie, [], outputs, 3)

        tx = CTransaction()
        tx.version = 3

        self.bulk_tx(tx, 5, int(TRUC_CHILD_MAX_VSIZE/2))

        assert_raises_rpc_error(
            -4,
            "Maximum transaction weight is less than transaction weight without inputs",
            self.alice.fundrawtransaction,
            tx.serialize_with_witness().hex(),
            {'include_unsafe' : True, 'max_tx_weight' : int(TRUC_CHILD_MAX_VSIZE/2)}
        )

        self.generate(self.nodes[0], 1)
        self.alice.fundrawtransaction(tx.serialize_with_witness().hex())

    @cleanup
    def createpsbt_v3(self):
        self.log.info("Test setting version to 3 with createpsbt")

        outputs = {self.alice.getnewaddress() : 10}
        psbt = self.charlie.createpsbt(inputs=[], outputs=outputs, version=3)
        assert_equal(self.charlie.decodepsbt(psbt)["tx"]["version"], 3)

    @cleanup
    def send_v3(self):
        self.log.info("Test setting version to 3 with send")

        outputs = {self.alice.getnewaddress() : 10}
        tx_hex = self.charlie.send(outputs=outputs, add_to_wallet=False, version=3)["hex"]
        assert_equal(self.charlie.decoderawtransaction(tx_hex)["version"], 3)

    @cleanup
    def sendall_v3(self):
        self.log.info("Test setting version to 3 with sendall")

        tx_hex = self.charlie.sendall(recipients=[self.alice.getnewaddress()], version=3, add_to_wallet=False)["hex"]
        assert_equal(self.charlie.decoderawtransaction(tx_hex)["version"], 3)

    @cleanup
    def sendall_with_unconfirmed_v3(self):
        self.log.info("Test setting version to 3 with sendall + unconfirmed inputs")

        outputs = {self.alice.getnewaddress(): 2.00001 for _ in range(4)}

        self.send_tx(self.charlie, [], outputs, 2)
        self.generate(self.nodes[0], 1)

        unspents = self.alice.listunspent()

        # confirmed v2 utxos
        outputs = {self.alice.getnewaddress() : 2.0}
        confirmed_v2 = self.send_tx(self.alice, [unspents[0]], outputs, 2)

        # confirmed v3 utxos
        outputs = {self.alice.getnewaddress() : 2.0}
        confirmed_v3 = self.send_tx(self.alice, [unspents[1]], outputs, 3)

        self.generate(self.nodes[0], 1)

        # unconfirmed v2 utxos
        outputs = {self.alice.getnewaddress() : 2.0}
        unconfirmed_v2 = self.send_tx(self.alice, [unspents[2]], outputs, 2)

        # unconfirmed v3 utxos
        outputs = {self.alice.getnewaddress() : 2.0}
        unconfirmed_v3 = self.send_tx(self.alice, [unspents[3]], outputs, 3)

        # Test that the only unconfirmed inputs this v3 tx spends are v3
        tx_hex = self.alice.sendall([self.bob.getnewaddress()], version=3, add_to_wallet=False, minconf=0)["hex"]

        decoded_tx = self.alice.decoderawtransaction(tx_hex)
        decoded_vin_txids = [txin["txid"] for txin in decoded_tx["vin"]]

        assert_equal(decoded_tx["version"], 3)

        assert confirmed_v3 in decoded_vin_txids
        assert confirmed_v2 in decoded_vin_txids
        assert unconfirmed_v3 in decoded_vin_txids
        assert unconfirmed_v2 not in decoded_vin_txids

        # Test that the only unconfirmed inputs this v2 tx spends are v2
        tx_hex = self.alice.sendall([self.bob.getnewaddress()], version=2, add_to_wallet=False, minconf=0)["hex"]

        decoded_tx = self.alice.decoderawtransaction(tx_hex)
        decoded_vin_txids = [txin["txid"] for txin in decoded_tx["vin"]]

        assert_equal(decoded_tx["version"], 2)

        assert confirmed_v3 in decoded_vin_txids
        assert confirmed_v2 in decoded_vin_txids
        assert unconfirmed_v2 in decoded_vin_txids
        assert unconfirmed_v3 not in decoded_vin_txids

    @cleanup
    def walletcreatefundedpsbt_v3(self):
        self.log.info("Test setting version to 3 with walletcreatefundedpsbt")

        outputs = {self.alice.getnewaddress() : 10}
        psbt = self.charlie.walletcreatefundedpsbt(inputs=[], outputs=outputs, version=3)["psbt"]
        assert_equal(self.charlie.decodepsbt(psbt)["tx"]["version"], 3)

    @cleanup
    def sendall_truc_weight_limit(self):
        self.log.info("Test that sendall follows truc tx weight limit")
        self.charlie.sendall([self.alice.getnewaddress() for _ in range(300)], add_to_wallet=False, version=2)

        # check that error is only raised if version is 3
        assert_raises_rpc_error(
                -4,
                "Transaction too large" ,
                self.charlie.sendall,
                [self.alice.getnewaddress() for _ in range(300)],
                version=3
            )

    @cleanup
    def sendall_truc_child_weight_limit(self):
        self.log.info("Test that sendall follows spending unconfirmed truc tx weight limit")
        outputs = {self.charlie.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 3)

        self.charlie.sendall([self.alice.getnewaddress() for _ in range(50)], add_to_wallet=False)

        assert_raises_rpc_error(
                -4,
                "Transaction too large" ,
                self.charlie.sendall,
                [self.alice.getnewaddress() for _ in range(50)],
                version=3
            )

    @cleanup
    def mix_non_truc_versions(self):
        self.log.info("Test that we can mix non-truc versions when spending an unconfirmed output")

        outputs = {self.bob.getnewaddress() : 2.0}
        self.send_tx(self.charlie, [], outputs, 1)

        assert_equal(self.bob.getbalances()["mine"]["trusted"], 0)
        assert_greater_than(self.bob.getbalances()["mine"]["untrusted_pending"], 0)

        outputs = {self.alice.getnewaddress() : 1.0}

        raw_tx_v2 = self.bob.createrawtransaction(inputs=[], outputs=outputs, version=2)

        # does not throw an error
        self.bob.fundrawtransaction(raw_tx_v2, {'include_unsafe': True})["hex"]

    @cleanup
    def cant_spend_multiple_unconfirmed_truc_outputs(self):
        self.log.info("Test that we can't spend multiple unconfirmed truc outputs")

        outputs = {self.alice.getnewaddress(): 2.00001}
        self.send_tx(self.charlie, [], outputs, 3)
        self.send_tx(self.charlie, [], outputs, 3)

        assert_equal(len(self.alice.listunspent(minconf=0)), 2)

        outputs = {self.bob.getnewaddress() : 3.0}

        raw_tx = self.alice.createrawtransaction(inputs=[], outputs=outputs, version=3)

        assert_raises_rpc_error(
                -4,
                "Insufficient funds",
                self.alice.fundrawtransaction,
                raw_tx,
                {'include_unsafe' : True}
        )

    @cleanup
    def test_spend_third_generation(self):
        self.log.info("Test that we can't spend an unconfirmed TRUC output that already has an unconfirmed parent")

        # Generation 1: Consolidate all UTXOs into one output using sendall
        self.charlie.sendall([self.charlie.getnewaddress()], version=3)
        outputs1 = self.charlie.listunspent(minconf=0)
        assert_equal(len(outputs1), 1)

        # Generation 2: to ensure no change address is created, do another sendall
        self.charlie.sendall([self.charlie.getnewaddress()], version=3)
        outputs2 = self.charlie.listunspent(minconf=0)
        assert_equal(len(outputs2), 1)
        total_amount = sum([utxo['amount'] for utxo in outputs2])

        # Generation 3: try to send half of total amount to Alice
        outputs = {self.alice.getnewaddress(): total_amount / 2}
        assert_raises_rpc_error(
                -4,
                "Insufficient funds",
                self.charlie.send,
                outputs,
                version=3
        )

        # Also doesn't work with fundrawtransaction
        raw_tx = self.charlie.createrawtransaction(inputs=[], outputs=outputs, version=3)
        assert_raises_rpc_error(
                -4,
                "Insufficient funds",
                self.charlie.fundrawtransaction,
                raw_tx,
                {'include_unsafe' : True}
        )

        # Also doesn't work with sendall
        assert_raises_rpc_error(
                -6,
                "Total value of UTXO pool too low to pay for transaction",
                self.charlie.sendall,
                [self.alice.getnewaddress()],
                version=3
        )

    @cleanup
    def test_coins_availability_reorg(self):
        self.log.info("Test coin availability after reorg with v2 parent and truc child")

        # Prep fork blocks
        fork_blocks = create_empty_fork(self.nodes[0])

        # Send funds to alice so she can create transactions
        outputs = {self.alice.getnewaddress(): 5.0}
        self.send_tx(self.charlie, [], outputs, 2)
        self.generate(self.nodes[0], 1)

        # Alice creates a v2 transaction with 2 outputs
        alice_unspent = self.alice.listunspent()[0]
        v2_outputs = [
            {self.alice.getnewaddress(): 2.5},
            {self.alice.getnewaddress(): 2.4999},
        ]
        v2_txid = self.send_tx(self.alice, [alice_unspent], v2_outputs, 2)

        # Mine the v2 transaction in one block
        self.generate(self.nodes[0], 1)

        # Get the output from the v2 transaction for chaining
        v2_utxo = self.alice.listunspent(minconf=1)[0]
        assert_equal(v2_utxo["txid"], v2_txid)

        # Alice creates a truc child chaining from the v2 utxo
        truc_outputs = {self.alice.getnewaddress(): v2_utxo["amount"] - Decimal("0.0001")}
        truc_txid = self.send_tx(self.alice, [v2_utxo], truc_outputs, 3)

        # Mine the truc transaction in a second block
        self.generate(self.nodes[0], 1)

        # Verify both transactions are confirmed
        wallet_tx_v2 = self.alice.gettransaction(v2_txid)
        wallet_tx_truc = self.alice.gettransaction(truc_txid)

        assert_equal(wallet_tx_v2["confirmations"], 2)
        assert_equal(wallet_tx_truc["confirmations"], 1)

        # Check that their versions are correct
        assert_equal(self.alice.decoderawtransaction(wallet_tx_v2["hex"])["version"], 2)
        assert_equal(self.alice.decoderawtransaction(wallet_tx_truc["hex"])["version"], 3)

        # Check listunspent before reorg - should have the truc output
        unspent_before = self.alice.listunspent()
        truc_output_txids = [u["txid"] for u in unspent_before]
        assert truc_txid in truc_output_txids

        # Trigger the reorg
        self.trigger_reorg(fork_blocks)
        # The TRUC transaction is now in a cluster of size 3, which is only permitted in a reorg.
        assert_equal(self.nodes[0].getmempoolcluster(truc_txid)["txcount"], 3)

        # After reorg, both transactions should be back in mempool
        mempool = self.nodes[0].getrawmempool()
        assert v2_txid in mempool
        assert truc_txid in mempool

        # Check listunspent after reorg - the truc output should still appear
        # as unconfirmed since the transaction is in the mempool
        unspent_after = self.alice.listunspent(minconf=0)
        unspent_txids_after = [u["txid"] for u in unspent_after]
        assert truc_txid in unspent_txids_after

        total_unconfirmed_amount = sum([u["amount"] for u in self.alice.listunspent(minconf=0)])
        # We cannot create a transaction spending both outputs, regardless of version.
        output_too_high = {self.bob.getnewaddress(): total_unconfirmed_amount - Decimal("1")}
        for version in [2, 3]:
            raw_output_too_high = self.alice.createrawtransaction(inputs=[], outputs=output_too_high, version=version)
            assert_raises_rpc_error(
                    -4,
                    "Insufficient funds",
                    self.alice.fundrawtransaction,
                    raw_output_too_high,
                    {'include_unsafe' : True}
            )

        # Now try to create a v2 transaction - this triggers AvailableCoins with
        # check_version_trucness=true. The v2 parent has truc_child_in_mempool set
        # because the truc child is in mempool.
        new_v2_outputs = {self.bob.getnewaddress(): 0.1}
        raw_v2_child = self.alice.createrawtransaction(inputs=[], outputs=new_v2_outputs, version=2)
        raw_v2_with_v3_sibling = self.alice.fundrawtransaction(raw_v2_child, {'include_unsafe': True})

        # See that this transaction can be added to mempool
        signed_raw_v2_with_v3_sibling = self.alice.signrawtransactionwithwallet(raw_v2_with_v3_sibling["hex"])
        self.alice.sendrawtransaction(signed_raw_v2_with_v3_sibling["hex"])

        # This TRUC transaction is in a cluster of size 4
        assert_equal(self.nodes[0].getmempoolcluster(truc_txid)["txcount"], 4)



if __name__ == '__main__':
    WalletV3Test(__file__).main()
