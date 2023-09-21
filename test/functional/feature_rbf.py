#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RBF code."""

from decimal import Decimal

from test_framework.messages import (
    MAX_BIP125_RBF_SEQUENCE,
    COIN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    get_fee,
)
from test_framework.wallet import MiniWallet
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE

MAX_REPLACEMENT_LIMIT = 100
MAX_CLUSTER_LIMIT = 64

class ReplaceByFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [
                "-limitancestorcount=64",
                "-limitdescendantcount=64",
            ],
            # second node has default mempool parameters
            [
            ],
        ]
        self.uses_wallet = None

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.log.info("Running test simple doublespend...")
        self.test_simple_doublespend()

        self.log.info("Running test doublespend chain...")
        self.test_doublespend_chain()

        self.log.info("Running test doublespend tree...")
        self.test_doublespend_tree()

        self.log.info("Running test replacement feeperkb...")
        self.test_replacement_feeperkb()

        self.log.info("Running test spends of conflicting outputs...")
        self.test_spends_of_conflicting_outputs()

        self.log.info("Running test new unconfirmed inputs...")
        self.test_new_unconfirmed_inputs()

        self.log.info("Running test too many replacements...")
        self.test_too_many_replacements()

        self.log.info("Running test RPC...")
        self.test_rpc()

        self.log.info("Running test prioritised transactions...")
        self.test_prioritised_transactions()

        self.log.info("Running test replacement relay fee...")
        self.test_replacement_relay_fee()

        self.log.info("Running test full replace by fee...")
        self.test_fullrbf()

        self.log.info("Running test incremental relay feerates...")
        self.test_incremental_relay_feerates()

        self.log.info("Passed")

    def make_utxo(self, node, amount, *, confirmed=True, scriptPubKey=None):
        """Create a txout with a given amount and scriptPubKey

        confirmed - txout created will be confirmed in the blockchain;
                    unconfirmed otherwise.
        """
        tx = self.wallet.send_to(from_node=node, scriptPubKey=scriptPubKey or self.wallet.get_output_script(), amount=amount)

        if confirmed:
            mempool_size = len(node.getrawmempool())
            while mempool_size > 0:
                self.generate(node, 1)
                new_size = len(node.getrawmempool())
                # Error out if we have something stuck in the mempool, as this
                # would likely be a bug.
                assert new_size < mempool_size
                mempool_size = new_size

        return self.wallet.get_utxo(txid=tx["txid"], vout=tx["sent_vout"])

    def test_simple_doublespend(self):
        """Simple doublespend"""
        # we use MiniWallet to create a transaction template with inputs correctly set,
        # and modify the output (amount, scriptPubKey) according to our needs
        tx = self.wallet.create_self_transfer(fee_rate=Decimal("0.003"))["tx"]
        tx1a_txid = self.nodes[0].sendrawtransaction(tx.serialize().hex())

        # Should fail because we haven't changed the fee
        tx.vout[0].scriptPubKey[-1] ^= 1
        tx_hex = tx.serialize().hex()

        # This will raise an exception due to insufficient fee
        reject_reason = "insufficient fee"
        reject_details = f"{reject_reason}, rejecting replacement {tx.txid_hex}, not enough additional fees to relay; 0.00 < 0.00000011"
        res = self.nodes[0].testmempoolaccept(rawtxs=[tx_hex])[0]
        assert_equal(res["reject-reason"], reject_reason)
        assert_equal(res["reject-details"], reject_details)
        assert_raises_rpc_error(-26, f"{reject_details}", self.nodes[0].sendrawtransaction, tx_hex, 0)


        # Extra 0.1 BTC fee
        tx.vout[0].nValue -= int(0.1 * COIN)
        tx1b_hex = tx.serialize().hex()
        # Works when enabled
        tx1b_txid = self.nodes[0].sendrawtransaction(tx1b_hex, 0)

        mempool = self.nodes[0].getrawmempool()

        assert tx1a_txid not in mempool
        assert tx1b_txid in mempool

        assert_equal(tx1b_hex, self.nodes[0].getrawtransaction(tx1b_txid))

    def test_doublespend_chain(self):
        """Doublespend of a long chain"""

        initial_nValue = 5 * COIN
        tx0_outpoint = self.make_utxo(self.nodes[0], initial_nValue)

        prevout = tx0_outpoint
        remaining_value = initial_nValue
        chain_txids = []
        for _ in range(MAX_CLUSTER_LIMIT):
            if remaining_value <= 1 * COIN:
                break
            remaining_value -= int(0.1 * COIN)
            prevout = self.wallet.send_self_transfer(
                from_node=self.nodes[0],
                utxo_to_spend=prevout,
                sequence=0,
                fee=Decimal("0.1"),
            )["new_utxo"]
            chain_txids.append(prevout["txid"])

        # Whether the double-spend is allowed is evaluated by including all
        # child fees - 4 BTC - so this attempt is rejected.
        dbl_tx = self.wallet.create_self_transfer(
            utxo_to_spend=tx0_outpoint,
            sequence=0,
            fee=Decimal("3"),
        )["tx"]
        dbl_tx_hex = dbl_tx.serialize().hex()

        # This will raise an exception due to insufficient fee
        reject_reason = "insufficient fee"
        reject_details = f"{reject_reason}, rejecting replacement {dbl_tx.txid_hex}, less fees than conflicting txs; 3.00 < 4.00"
        res = self.nodes[0].testmempoolaccept(rawtxs=[dbl_tx_hex])[0]
        assert_equal(res["reject-reason"], reject_reason)
        assert_equal(res["reject-details"], reject_details)
        assert_raises_rpc_error(-26, f"{reject_details}", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)



        # Accepted with sufficient fee
        dbl_tx.vout[0].nValue = int(0.1 * COIN)
        dbl_tx_hex = dbl_tx.serialize().hex()
        self.nodes[0].sendrawtransaction(dbl_tx_hex, 0)

        mempool = self.nodes[0].getrawmempool()
        for doublespent_txid in chain_txids:
            assert doublespent_txid not in mempool

    def test_doublespend_tree(self):
        """Doublespend of a big tree of transactions"""

        initial_nValue = 5 * COIN
        tx0_outpoint = self.make_utxo(self.nodes[0], initial_nValue)

        def branch(prevout, initial_value, max_txs, tree_width=5, fee=0.00001 * COIN, _total_txs=None):
            if _total_txs is None:
                _total_txs = [0]
            if _total_txs[0] >= max_txs:
                return

            txout_value = (initial_value - fee) // tree_width
            if txout_value < fee:
                return

            tx = self.wallet.send_self_transfer_multi(
                utxos_to_spend=[prevout],
                from_node=self.nodes[0],
                sequence=0,
                num_outputs=tree_width,
                amount_per_output=txout_value,
            )

            yield tx["txid"]
            _total_txs[0] += 1

            for utxo in tx["new_utxos"]:
                for x in branch(utxo, txout_value,
                                  max_txs,
                                  tree_width=tree_width, fee=fee,
                                  _total_txs=_total_txs):
                    yield x

        fee = int(0.00001 * COIN)
        n = MAX_CLUSTER_LIMIT
        tree_txs = list(branch(tx0_outpoint, initial_nValue, n, fee=fee))
        assert_equal(len(tree_txs), n)

        # Attempt double-spend, will fail because too little fee paid
        dbl_tx_hex = self.wallet.create_self_transfer(
            utxo_to_spend=tx0_outpoint,
            sequence=0,
            fee=(Decimal(fee) / COIN) * n,
        )["hex"]
        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)

        # 0.1 BTC fee is enough
        dbl_tx_hex = self.wallet.create_self_transfer(
            utxo_to_spend=tx0_outpoint,
            sequence=0,
            fee=(Decimal(fee) / COIN) * n + Decimal("0.1"),
        )["hex"]
        self.nodes[0].sendrawtransaction(dbl_tx_hex, 0)

        mempool = self.nodes[0].getrawmempool()

        for txid in tree_txs:
            assert txid not in mempool

    def test_replacement_feeperkb(self):
        """Replacement requires fee-per-KB to be higher"""
        tx0_outpoint = self.make_utxo(self.nodes[0], int(1.1 * COIN))

        self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx0_outpoint,
            sequence=0,
            fee=Decimal("0.1"),
        )

        # Higher fee, but the fee per KB is much lower, so the replacement is
        # rejected.
        tx1b_hex = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx0_outpoint],
            sequence=0,
            num_outputs=100,
            amount_per_output=1000,
        )["hex"]

        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "does not improve feerate diagram", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

    def test_spends_of_conflicting_outputs(self):
        """Replacements that spend conflicting tx outputs are rejected"""
        utxo1 = self.make_utxo(self.nodes[0], int(1.2 * COIN))
        utxo2 = self.make_utxo(self.nodes[0], 3 * COIN)

        tx1a = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=utxo1,
            sequence=0,
            fee=Decimal("0.1"),
        )
        tx1a_utxo = tx1a["new_utxo"]

        # Direct spend an output of the transaction we're replacing.
        tx2 = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[utxo1, utxo2, tx1a_utxo],
            sequence=0,
            amount_per_output=int(COIN * tx1a_utxo["value"]),
        )["tx"]
        tx2_hex = tx2.serialize().hex()

        # This will raise an exception
        reject_reason = "bad-txns-spends-conflicting-tx"
        reject_details = f"{reject_reason}, {tx2.txid_hex} spends conflicting transaction {tx1a['tx'].txid_hex}"
        res = self.nodes[0].testmempoolaccept(rawtxs=[tx2_hex])[0]
        assert_equal(res["reject-reason"], reject_reason)
        assert_equal(res["reject-details"], reject_details)
        assert_raises_rpc_error(-26, f"{reject_details}", self.nodes[0].sendrawtransaction, tx2_hex, 0)


        # Spend tx1a's output to test the indirect case.
        tx1b_utxo = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx1a_utxo,
            sequence=0,
            fee=Decimal("0.1"),
        )["new_utxo"]

        tx2_hex = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[utxo1, utxo2, tx1b_utxo],
            sequence=0,
            amount_per_output=int(COIN * tx1a_utxo["value"]),
        )["hex"]

        # This will raise an exception
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, tx2_hex, 0)

    def test_new_unconfirmed_inputs(self):
        """Replacements that add new unconfirmed inputs may be accepted"""
        confirmed_utxo = self.make_utxo(self.nodes[0], int(1.1 * COIN))
        unconfirmed_utxo = self.make_utxo(self.nodes[0], int(0.2 * COIN), confirmed=False)

        self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=confirmed_utxo,
            sequence=0,
            fee=Decimal("0.1"),
        )

        tx2 = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[confirmed_utxo, unconfirmed_utxo],
            sequence=0,
            amount_per_output=1 * COIN,
        )["tx"]
        tx2_hex = tx2.serialize().hex()

        # This will not raise an exception
        tx2_id = self.nodes[0].sendrawtransaction(tx2_hex, 0)
        assert tx2_id in self.nodes[0].getrawmempool()

    def test_too_many_replacements(self):
        """Replacements that evict too many transactions are rejected"""
        # Try directly replacing more than MAX_REPLACEMENT_LIMIT
        # transactions

        # Start by creating a single transaction with many outputs
        initial_nValue = 10 * COIN
        utxo = self.make_utxo(self.nodes[0], initial_nValue)
        fee = int(0.0001 * COIN)
        split_value = int((initial_nValue - fee) / (MAX_REPLACEMENT_LIMIT + 1))

        splitting_tx_utxos = self.wallet.send_self_transfer_multi(
            from_node=self.nodes[0],
            utxos_to_spend=[utxo],
            sequence=0,
            num_outputs=MAX_REPLACEMENT_LIMIT + 1,
            amount_per_output=split_value,
        )["new_utxos"]

        self.generate(self.nodes[0], 1)

        # Now spend each of those outputs individually
        for utxo in splitting_tx_utxos:
            self.wallet.send_self_transfer(
                from_node=self.nodes[0],
                utxo_to_spend=utxo,
                sequence=0,
                fee=Decimal(fee) / COIN,
            )

        # Now create doublespend of the whole lot; should fail.
        # Need a big enough fee to cover all spending transactions and have
        # a higher fee rate
        double_spend_value = (split_value - 100 * fee) * (MAX_REPLACEMENT_LIMIT + 1)
        double_tx = self.wallet.create_self_transfer_multi(
            utxos_to_spend=splitting_tx_utxos,
            sequence=0,
            amount_per_output=double_spend_value,
        )["tx"]
        double_tx_hex = double_tx.serialize().hex()

        # This will raise an exception
        reject_reason = "too many potential replacements"
        reject_details = f"{reject_reason}, rejecting replacement {double_tx.txid_hex}; too many conflicting clusters ({MAX_REPLACEMENT_LIMIT + 1} > {MAX_REPLACEMENT_LIMIT})"
        res = self.nodes[0].testmempoolaccept(rawtxs=[double_tx_hex])[0]
        assert_equal(res["reject-reason"], reject_reason)
        assert_equal(res["reject-details"], reject_details)
        assert_raises_rpc_error(-26, f"{reject_details}", self.nodes[0].sendrawtransaction, double_tx_hex, 0)


        # If we remove an input, it should pass
        double_tx.vin.pop()
        double_tx_hex = double_tx.serialize().hex()
        self.nodes[0].sendrawtransaction(double_tx_hex, 0)

    def test_prioritised_transactions(self):
        # Ensure that fee deltas used via prioritisetransaction are
        # correctly used by replacement logic

        # 1. Check that feeperkb uses modified fees
        tx0_outpoint = self.make_utxo(self.nodes[0], int(1.1 * COIN))

        tx1a_txid = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx0_outpoint,
            sequence=0,
            fee=Decimal("0.1"),
        )["txid"]

        # Higher fee, but the actual fee per KB is much lower.
        tx1b_hex = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[tx0_outpoint],
            sequence=0,
            num_outputs=100,
            amount_per_output=int(0.00001 * COIN),
        )["hex"]

        # Verify tx1b cannot replace tx1a.
        assert_raises_rpc_error(-26, "does not improve feerate diagram", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

        # Use prioritisetransaction to set tx1a's fee to 0.
        self.nodes[0].prioritisetransaction(txid=tx1a_txid, fee_delta=int(-0.1 * COIN))

        # Now tx1b should be able to replace tx1a
        tx1b_txid = self.nodes[0].sendrawtransaction(tx1b_hex, 0)

        assert tx1b_txid in self.nodes[0].getrawmempool()

        # 2. Check that absolute fee checks use modified fee.
        tx1_outpoint = self.make_utxo(self.nodes[0], int(1.1 * COIN))

        # tx2a
        self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx1_outpoint,
            sequence=0,
            fee=Decimal("0.1"),
        )

        # Lower fee, but we'll prioritise it
        tx2b = self.wallet.create_self_transfer(
            utxo_to_spend=tx1_outpoint,
            sequence=0,
            fee=Decimal("0.09"),
        )

        # Verify tx2b cannot replace tx2a.
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx2b["hex"], 0)

        # Now prioritise tx2b to have a higher modified fee
        self.nodes[0].prioritisetransaction(txid=tx2b["txid"], fee_delta=int(0.1 * COIN))

        # tx2b should now be accepted
        tx2b_txid = self.nodes[0].sendrawtransaction(tx2b["hex"], 0)

        assert tx2b_txid in self.nodes[0].getrawmempool()

    def test_rpc(self):
        us0 = self.wallet.get_utxo()
        ins = [us0]
        outs = {ADDRESS_BCRT1_UNSPENDABLE: Decimal(1.0000000)}
        rawtx0 = self.nodes[0].createrawtransaction(ins, outs, 0, True)
        rawtx1 = self.nodes[0].createrawtransaction(ins, outs, 0, False)
        json0 = self.nodes[0].decoderawtransaction(rawtx0)
        json1 = self.nodes[0].decoderawtransaction(rawtx1)
        assert_equal(json0["vin"][0]["sequence"], 4294967293)
        assert_equal(json1["vin"][0]["sequence"], 4294967295)

        if self.is_wallet_compiled():
            self.init_wallet(node=0)
            rawtx2 = self.nodes[0].createrawtransaction([], outs)
            frawtx2a = self.nodes[0].fundrawtransaction(rawtx2, {"replaceable": True})
            frawtx2b = self.nodes[0].fundrawtransaction(rawtx2, {"replaceable": False})

            json0 = self.nodes[0].decoderawtransaction(frawtx2a['hex'])
            json1 = self.nodes[0].decoderawtransaction(frawtx2b['hex'])
            assert_equal(json0["vin"][0]["sequence"], 4294967293)
            assert_equal(json1["vin"][0]["sequence"], 4294967294)

    def test_replacement_relay_fee(self):
        tx = self.wallet.send_self_transfer(from_node=self.nodes[0])['tx']

        # Higher fee, higher feerate, different txid, but the replacement does not provide a relay
        # fee conforming to node's `incrementalrelayfee` policy of 100 sat per KB.
        assert_equal(self.nodes[0].getmempoolinfo()["incrementalrelayfee"], Decimal("0.000001"))
        tx.vout[0].nValue -= 1
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx.serialize().hex())

    def test_incremental_relay_feerates(self):
        self.log.info("Test that incremental relay fee is applied correctly in RBF for various settings...")
        node = self.nodes[0]
        for incremental_setting in (0, 5, 10, 50, 100, 234, 1000, 5000, 21000):
            incremental_setting_decimal = incremental_setting / Decimal(COIN)
            self.log.info(f"-> Test -incrementalrelayfee={incremental_setting:.8f}sat/kvB...")
            self.restart_node(0, extra_args=[f"-incrementalrelayfee={incremental_setting_decimal:.8f}", "-persistmempool=0"])

            # When incremental relay feerate is higher than min relay feerate, min relay feerate is automatically increased.
            min_relay_feerate = node.getmempoolinfo()["minrelaytxfee"]
            assert_greater_than_or_equal(min_relay_feerate, incremental_setting_decimal)

            low_feerate = min_relay_feerate * 2
            confirmed_utxo = self.wallet.get_utxo(confirmed_only=True)
            # Use different versions to avoid creating an identical transaction when failed_replacement_tx is created.
            # Use a target vsize that is small, but something larger than the minimum so that we can create a transaction that is 1vB smaller later.
            replacee_tx = self.wallet.create_self_transfer(utxo_to_spend=confirmed_utxo, fee_rate=low_feerate, version=3, target_vsize=200)
            node.sendrawtransaction(replacee_tx['hex'])

            replacement_placeholder_tx = self.wallet.create_self_transfer(utxo_to_spend=confirmed_utxo, target_vsize=200)
            replacement_expected_size = replacement_placeholder_tx['tx'].get_vsize()
            replacement_required_fee = get_fee(replacement_expected_size, incremental_setting_decimal) + replacee_tx['fee']

            # Show that replacement fails when paying 1 satoshi shy of the required fee
            failed_replacement_tx = self.wallet.create_self_transfer(utxo_to_spend=confirmed_utxo, fee=replacement_required_fee - Decimal("0.00000001"), version=2, target_vsize=200)
            assert_raises_rpc_error(-26, "insufficient fee", node.sendrawtransaction, failed_replacement_tx['hex'])
            replacement_tx = self.wallet.create_self_transfer(utxo_to_spend=confirmed_utxo, fee=replacement_required_fee, version=2, target_vsize=200)

            if incremental_setting == 0:
                # When incremental relay feerate is 0, additional fees are not required, but higher feerate is still required.
                assert_raises_rpc_error(-26, "insufficient fee", node.sendrawtransaction, replacement_tx['hex'])
                replacement_tx_smaller = self.wallet.create_self_transfer(utxo_to_spend=confirmed_utxo, fee=replacement_required_fee, version=2, target_vsize=199)
                node.sendrawtransaction(replacement_tx_smaller['hex'])
            else:
                node.sendrawtransaction(replacement_tx['hex'])

    def test_fullrbf(self):
        # BIP125 signaling is not respected

        confirmed_utxo = self.make_utxo(self.nodes[0], int(2 * COIN))
        assert self.nodes[0].getmempoolinfo()["fullrbf"]

        # Create an explicitly opt-out BIP125 transaction, which will be ignored
        optout_tx = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=confirmed_utxo,
            sequence=MAX_BIP125_RBF_SEQUENCE + 1,
            fee_rate=Decimal('0.01'),
        )
        assert_equal(False, self.nodes[0].getmempoolentry(optout_tx['txid'])['bip125-replaceable'])

        conflicting_tx = self.wallet.create_self_transfer(
                utxo_to_spend=confirmed_utxo,
                fee_rate=Decimal('0.02'),
        )

        # Send the replacement transaction, conflicting with the optout_tx.
        self.nodes[0].sendrawtransaction(conflicting_tx['hex'], 0)

        # Optout_tx is not anymore in the mempool.
        assert optout_tx['txid'] not in self.nodes[0].getrawmempool()
        assert conflicting_tx['txid'] in self.nodes[0].getrawmempool()

if __name__ == '__main__':
    ReplaceByFeeTest(__file__).main()
