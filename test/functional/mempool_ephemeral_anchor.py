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

class EphemeralAnchorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
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


    def create_simple_package(self, parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, spend_anchor=1, additional_outputs=None):
        """Create a 1 parent 1 child package using the coin passed in as the parent's input. The
        parent has 1 output, used to fund 1 child transaction.
        All transactions signal BIP125 replaceability, but nSequence changes based on self.ctr. This
        prevents identical txids between packages when the parents spend the same coin and have the
        same fee (i.e. 0sat).

        returns tuple (hex serialized txns, CTransaction objects)
        """

        if additional_outputs is None:
            additional_outputs=[CTxOut(0, CScript([OP_TRUE]))]

        child_inputs = []
        self.ctr += 1
        # Use fee_rate=0 because create_self_transfer will use the default fee_rate value otherwise.
        # Passing in fee>0 overrides fee_rate, so this still works for non-zero parent_fee.
        parent_result = self.wallet.create_self_transfer(
            fee_rate=0,
            fee=parent_fee,
            utxo_to_spend=parent_coin,
            sequence=MAX_BIP125_RBF_SEQUENCE - self.ctr,
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

        self.test_fee_having_parent()
        self.test_multianchor()
        self.test_nonzero_anchor()
        self.test_prioritise_parent()

    def test_fee_having_parent(self):
        self.log.info("Test that a transaction with ephemeral anchor may not have base fee")
        node = self.nodes[0]
        # Reuse the same coins so that the transactions conflict with one another.
        parent_coin = self.coins[-1]
        del self.coins[-1]

        package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=1, child_fee=DEFAULT_FEE)
        assert_raises_rpc_error(-26, "invalid-ephemeral-fee", node.submitpackage, package_hex0)
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

        package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, additional_outputs=[CTxOut(0, CScript([OP_TRUE]))] * 2)
        assert_raises_rpc_error(-26, "too-many-ephemeral-anchors", node.submitpackage, package_hex0)
        assert_equal(node.getrawmempool(), [])

        self.generate(node, 1)

    def test_nonzero_anchor(self):
        def inner_test_anchor_value(output_value):
            node = self.nodes[0]
            # Reuse the same coins so that the transactions conflict with one another.
            parent_coin = self.coins[-1]
            del self.coins[-1]

            package_hex0, package_txns0 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE, additional_outputs=[CTxOut(output_value, CScript([OP_TRUE]))])
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
        assert_raises_rpc_error(-26, "invalid-ephemeral-fee", node.submitpackage, package_hex0)
        assert_equal(node.getrawmempool(), [])

        # Also doesn't make it invalid if applied to the parent
        package_hex1, package_txns1 = self.create_simple_package(parent_coin=parent_coin, parent_fee=0, child_fee=DEFAULT_FEE)
        parent_txid = node.decoderawtransaction(package_hex1[0])['txid']
        node.prioritisetransaction(txid=parent_txid, dummy=0, fee_delta=COIN)
        node.submitpackage(package_hex1)
        self.assert_mempool_contents(expected=package_txns1, unexpected=[])

        self.generate(node, 1)


if __name__ == "__main__":
    EphemeralAnchorTest().main()
