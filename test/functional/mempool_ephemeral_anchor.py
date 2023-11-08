#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import copy
from decimal import Decimal

from test_framework.messages import (
    COIN,
    CTxInWitness,
    CTxOut,
    MAX_BIP125_RBF_SEQUENCE,
)
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    DEFAULT_FEE,
    MiniWallet,
)

ANCHOR_SCRIPT = CScript([OP_TRUE, b'\x4e\x73'])

class EphemeralAnchorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            "-whitelist=noban@127.0.0.1",  # immediate tx relay
        ]] * self.num_nodes
        self.setup_clean_chain = True

    def assert_mempool_contents(self, expected=None, unexpected=None):
        """Assert that all transactions in expected are in the mempool,
        and all transactions in unexpected are not in the mempool.
        """
        if not expected:
            expected = []
        if not unexpected:
            unexpected = []
        assert set(unexpected).isdisjoint(expected)
        mempool = self.nodes[0].getrawmempool(verbose=False)
        for tx in expected:
            assert tx.rehash() in mempool
        for tx in unexpected:
            assert tx.rehash() not in mempool

    def insert_additional_outputs(self, parent_result, additional_outputs):
        # Modify transaction as needed to add ephemeral anchor
        parent_tx = parent_result["tx"]
        additional_sum = 0
        for additional_output in additional_outputs:
            parent_tx.vout.append(additional_output)
            additional_sum += additional_output.nValue

        # Steal value from destination and recompute fields
        parent_tx.vout[0].nValue -= additional_sum
        parent_result["txid"] = parent_tx.rehash()
        parent_result["wtxid"] = parent_tx.getwtxid()
        parent_result["hex"] = parent_tx.serialize().hex()
        parent_result["new_utxo"] = {**parent_result["new_utxo"],  "txid": parent_result["txid"], "value": Decimal(parent_tx.vout[0].nValue)/COIN}


    def spend_ephemeral_anchor_witness(self, child_result, child_inputs):
        child_tx = child_result["tx"]
        child_tx.wit.vtxinwit = [copy.deepcopy(child_tx.wit.vtxinwit[0]) if "anchor" not in x else CTxInWitness() for x in child_inputs]
        child_result["hex"] = child_tx.serialize().hex()


    def create_simple_package(self, parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, spend_anchor=1, additional_outputs=None, version=3):
        """Create a 1 parent 1 child package using the coin passed in as the parent's input. The
        parent has 1 output, used to fund 1 child transaction.
        All transactions signal BIP125 replaceability, but nSequence changes based on self.ctr. This
        prevents identical txids between packages when the parents spend the same coin and have the
        same fee (i.e. 0sat).

        returns tuple (hex serialized txns, CTransaction objects)
        """

        if additional_outputs is None:
            additional_outputs=[CTxOut(0, ANCHOR_SCRIPT)]

        child_inputs = []
        self.ctr += 1
        # Use fee_rate=0 because create_self_transfer will use the default fee_rate value otherwise.
        # Passing in fee>0 overrides fee_rate, so this still works for non-zero parent_fee.
        parent_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=parent_fee,
            utxo_to_spend=parent_coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
            version=version,
        )

        self.insert_additional_outputs(parent_result, additional_outputs)

        # Add inputs to child, depending on spend arg
        child_inputs.append(parent_result["new_utxo"])
        if spend_anchor:
            for vout, output in enumerate(additional_outputs):
                child_inputs.append({**parent_result["new_utxo"], 'vout': 1+vout, 'value': Decimal(output.nValue)/COIN, 'anchor': True})


        child_result = self.wallet.create_self_transfer_multi(
            utxos_to_spend=child_inputs,
            num_outputs=1,
            fee_per_output=int(child_fee * COIN),
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
            version=version,
        )

        if spend_anchor:
            self.spend_ephemeral_anchor_witness(child_result, child_inputs)

        package_hex = [parent_result["hex"], child_result["hex"]]
        package_txns = [parent_result["tx"], child_result["tx"]]
        return package_hex, package_txns

    def run_test(self):
        # Counter used to count the number of times we constructed packages. Since we're constructing parent transactions with the same
        # coins (to create conflicts), and giving them the same fee (i.e. 0, since their respective children are paying), we might
        # accidentally just create the exact same transaction again. To prevent this, set nSequences to MAX_BIP125_RBF_SEQUENCE - self.ctr.
        self.ctr = 0

        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 160)
        self.coins = self.wallet.get_utxos(mark_as_spent=False)
        # Mature coinbase transactions
        self.generate(self.wallet, 100)
        self.address = self.wallet.get_address()

        self.test_sponsor_swap()
        self.test_node_restart()
        self.test_fee_having_parent()
        self.test_multianchor()
        self.test_nonzero_anchor()
        self.test_prioritise_parent()
        self.test_non_v3()
        self.test_unspent_ephemeral()
        self.test_xor_rbf()

    def test_node_restart(self):
        self.log.info("Test that an ephemeral package is accepted on restart due to bypass_limits load")
        node = self.nodes[0]
        parent_coin = self.coins[-1]
        del self.coins[-1]

        # Enters mempool
        package_hex1, package_txns1 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=[])

        # Node restart; doesn't allow allow ephemeral transaction back in due to individual submission
        self.restart_node(0)
        assert_equal(node.getrawmempool(), [])

    def test_fee_having_parent(self):
        self.log.info("Test that a transaction with ephemeral anchor may not have base fee")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins[-1]
        del self.coins[-1]

        package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=1, child_fee=DEFAULT_FEE)
        res = node.submitpackage(package_hex0)
        assert_equal(res["tx-results"][package_txns0[0].getwtxid()]["error"], "invalid-ephemeral-fee")
        assert_equal(node.getrawmempool(), [])

        # But works with no parent fee
        package_hex1, package_txns1 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=[])

        self.generate(node, 1)

    def test_multianchor(self):
        self.log.info("Test that a transaction with multiple ephemeral anchors is nonstandard")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins[-1]
        del self.coins[-1]

        package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, additional_outputs=[CTxOut(0, ANCHOR_SCRIPT)] * 2)
        res = node.submitpackage(package_hex0)
        assert_equal(res["tx-results"][package_txns0[0].getwtxid()]["error"], "too-many-ephemeral-anchors")
        assert_equal(node.getrawmempool(), [])

        self.generate(node, 1)

    def test_nonzero_anchor(self):
        def inner_test_anchor_value(output_value):
            node = self.nodes[0]
            # Reuse the same coins so that the transactions conflict with one another.
            parent_coin = self.coins[-1]
            del self.coins[-1]

            package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, additional_outputs=[CTxOut(output_value, ANCHOR_SCRIPT)])
            node.submitpackage(package_hex0)
            self.assert_mempool_contents(expected=package_txns0, unexpected=[])

            self.generate(node, 1)

        self.log.info("Test that a transaction with ephemeral anchor may have any otherwise legal satoshi value")
        for i in range(5):
            inner_test_anchor_value(int(i*COIN/4))

    def test_prioritise_parent(self):
        self.log.info("Test that prioritizing a parent transaction with ephemeral anchor doesn't cause mempool rejection due to non-0 parent fee")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins[-1]
        del self.coins[-1]

        # De-prioritising to 0-fee doesn't matter; it's just the base fee that matters
        package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=1, child_fee=DEFAULT_FEE)
        parent_txid = node.decoderawtransaction(package_hex0[0])['txid']
        node.prioritisetransaction(txid=parent_txid, dummy=0, fee_delta=COIN)
        res = node.submitpackage(package_hex0)
        assert_equal(res["tx-results"][package_txns0[0].getwtxid()]["error"], "invalid-ephemeral-fee")
        assert_equal(node.getrawmempool(), [])

        # Also doesn't make it invalid if applied to the parent
        package_hex1, package_txns1 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE)
        parent_txid = node.decoderawtransaction(package_hex1[0])['txid']
        node.prioritisetransaction(txid=parent_txid, dummy=0, fee_delta=COIN)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=[])

        self.generate(node, 1)

    def test_non_v3(self):
        self.log.info("Test that v2 EA-having transaction is rejected")
        # N.B. Currently we never actually hit the "wrong version" check but min relay restriction
        # may be relaxed in the future for non-V3.

        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins[-1]
        del self.coins[-1]

        package_hex, package_txns = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, version=2)
        res = node.submitpackage(package_hex)
        assert_equal(res["tx-results"][package_txns[0].getwtxid()]["error"], "wrong-ephemeral-nversion")
        assert_equal(node.getrawmempool(), [])

    def test_unspent_ephemeral(self):
        self.log.info("Test that ephemeral outputs of any value are disallowed if not spent in a package")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins[-1]
        del self.coins[-1]

        # Submit whole package, but anchor are unspent
        package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, spend_anchor=
0)
        res = node.submitpackage(package_hex0)
        assert "missing-ephemeral-spends" in res["tx-results"][package_txns0[0].getwtxid()]["error"]
        assert_equal(node.getrawmempool(), [])

        # Individual submission also fails
        hex0_txid = node.decoderawtransaction(package_hex0[0])["txid"]
        node.prioritisetransaction(hex0_txid, 0, COIN)
        assert_raises_rpc_error(-26, "missing-ephemeral-spends", node.sendrawtransaction, package_hex0[0])
        assert_equal(node.getrawmempool(), [])

        # One more time
        package_hex3, package_txns3 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE)
        node.submitpackage(package_hex3)
        self.assert_mempool_contents(expected=package_txns3, unexpected=[])

        self.generate(node, 1)

    def test_xor_rbf(self):
        self.log.info("Test some child RBF behavior")
        node = self.nodes[0]
        num_parents = 2
        # Coins to create parents
        parent_coins = self.coins[:num_parents]
        del self.coins[:num_parents]

        # Coin to RBF own child
        child_coin = self.coins[0]
        del self.coins[0]

        package_hex = []
        package_txns = []


        child_inputs = []
        # Make two parents with one normal and one ephemeral output each
        for i, coin in enumerate(parent_coins):
            parent_result = self.wallet.create_self_transfer(
                fee_rate=0,
                fee=0,
                utxo_to_spend=parent_coins[i],
                sequence=MAX_BIP125_RBF_SEQUENCE,
                version=3
            )

            self.insert_additional_outputs(parent_result, [CTxOut(0, ANCHOR_SCRIPT)])

            child_inputs.append(parent_result["new_utxo"])
            child_inputs.append({**parent_result["new_utxo"], 'vout': 1, 'value': 0, 'anchor': True})

            package_hex.append(parent_result["hex"])
            package_txns.append(parent_result["tx"])


        # Append child_coin to possible spends
        child_inputs.append(child_coin)

        assert_equal(len(child_inputs), 5)

        # First child spends first parent's two inputs
        child_one = self.wallet.create_self_transfer_multi(
            utxos_to_spend=child_inputs[:2],
            num_outputs=1,
            fee_per_output=int(COIN),
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        self.spend_ephemeral_anchor_witness(child_one, child_inputs[:2])

        # Submit first parent and child together
        first_package_hex = [package_hex[0], child_one["hex"]]
        first_package_txns = [package_txns[0], child_one["tx"]]
        node.submitpackage(first_package_hex)
        self.assert_mempool_contents(expected=first_package_txns, unexpected=[])

        # Second child RBF spends first parent's two inputs, plus their own confirmed input
        second_inputs = child_inputs[:2] + [child_inputs[-1]]
        child_two = self.wallet.create_self_transfer_multi(
            utxos_to_spend=second_inputs,
            num_outputs=1,
            fee_per_output=int(COIN)*2,
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        self.spend_ephemeral_anchor_witness(child_two, second_inputs)

        second_package_hex = [package_hex[0], child_two["hex"]]
        second_package_txns = [package_txns[0], child_two["tx"]]
        node.submitpackage(second_package_hex)
        self.assert_mempool_contents(expected=second_package_txns, unexpected=[])

        # Third makes first parent childless via child's confirmed input double-spend
        # spending the second parent's ephemeral anchor and not the other output
        third_inputs = [child_inputs[3], child_inputs[-1]]
        child_three = self.wallet.create_self_transfer_multi(
            utxos_to_spend=third_inputs,
            num_outputs=1,
            fee_per_output=int(COIN)*3,
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        self.spend_ephemeral_anchor_witness(child_three, third_inputs)

        third_package_hex = [package_hex[1], child_three["hex"]]
        third_package_txns = [package_txns[1], child_three["tx"]]

        node.submitpackage(third_package_hex)
        # First parent not in mempool because it has been trimmed
        self.assert_mempool_contents(expected=third_package_txns, unexpected=[package_txns[0]])

        # This bump exercises the v3 section of CheckMinerScores
        fourth_inputs = child_inputs[:2] + [child_inputs[-1]]
        child_four = self.wallet.create_self_transfer_multi(
            utxos_to_spend=fourth_inputs,
            num_outputs=1,
            fee_per_output=int(COIN)*4,
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        self.spend_ephemeral_anchor_witness(child_four, fourth_inputs)

        fourth_package_hex = [package_hex[0], child_four["hex"]]
        fourth_package_txns = [package_txns[0], child_four["tx"]]
        node.submitpackage(fourth_package_hex)
        self.assert_mempool_contents(expected=fourth_package_txns, unexpected=[child_three["tx"]])

        # Mining everything
        self.generate(node, 1)
        assert_equal(node.getrawmempool(), [])


    def test_sponsor_swap(self):
        self.log.info("Test that EA txn is evicted when sponsoring tx doesn't spend anchor itself")
        node = self.nodes[0]

        # Coin to create 0-fee parent
        parent_coin = self.coins[0]
        del self.coins[0]

        # Fee coin for sponsor which will be RBF'd
        sponsor_coin = self.coins[0]
        del self.coins[0]

        package_hex = []
        package_txns = []

        parent_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=0,
            utxo_to_spend=parent_coin,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            version=3
        )

        self.insert_additional_outputs(parent_result, [CTxOut(0, ANCHOR_SCRIPT)])

        non_ea_coin = parent_result["new_utxo"]
        ea_coin = {**parent_result["new_utxo"], 'vout': 1, 'value': 0, 'anchor': True}

        package_hex.append(parent_result["hex"])
        package_txns.append(parent_result["tx"])

        # First child spends ephemeral anchor and sponsor
        first_spend = [ea_coin, sponsor_coin]
        child_one = self.wallet.create_self_transfer_multi(
            utxos_to_spend=first_spend,
            num_outputs=1,
            fee_per_output=int(COIN),
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        self.spend_ephemeral_anchor_witness(child_one, first_spend)

        # Submit parent and child together
        first_package_hex = [package_hex[0], child_one["hex"]]
        first_package_txns = [package_txns[0], child_one["tx"]]
        node.submitpackage(first_package_hex)
        self.assert_mempool_contents(expected=first_package_txns, unexpected=[])

        # Now we need to stage:
        # (a) 1-input-1-output spend of non_ea_coin
        # (b) 1-input-1-output double-spend of sponsor_coin
        # (c) 2-input-1-output spend of (a) and (b)
        #
        # This creates an ancestor package will would pass
        # ancestor package sanity checks to trigger package evaluation,
        # but (a) should be rejected for not spending ea_coin using
        # mempool-contextual checks. This is to catch
        # sub-package evaluation of just (a) allowing it through.

        a_spend = [non_ea_coin]
        child_a = self.wallet.create_self_transfer_multi(
            utxos_to_spend=a_spend,
            num_outputs=1,
            fee_per_output=int(COIN)*2,
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        b_spend = [sponsor_coin]
        child_b = self.wallet.create_self_transfer_multi(
            utxos_to_spend=b_spend,
            num_outputs=1,
            fee_per_output=int(COIN)*2,
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        # This tx should never work because of v3 package limits, but that's not
        # the error we're interested in exercising here
        c_spend = [child_a["new_utxos"][0], child_b["new_utxos"][0]]
        child_c = self.wallet.create_self_transfer_multi(
            utxos_to_spend=c_spend,
            num_outputs=1,
            fee_per_output=int(COIN)*2,
            sequence=MAX_BIP125_RBF_SEQUENCE - 1,
            version=3
        )

        # Submit a + b + c ancestor package
        node.submitpackage([child_a["hex"], child_b["hex"], child_c["hex"]])

        # Only the sponsor RBF makes it into mempool, parent is evicted and other pkg tx have been rejected for V3 violations
        assert_equal(node.getrawmempool(), [child_b["txid"]])

        # Mining everything
        self.generate(node, 1)
        assert_equal(node.getrawmempool(), [])


if __name__ == "__main__":
    EphemeralAnchorTest().main()
