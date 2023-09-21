#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RBF code."""

from decimal import Decimal

from test_framework.messages import (
    MAX_BIP125_RBF_SEQUENCE,
    COIN,
    SEQUENCE_FINAL,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE

MAX_REPLACEMENT_LIMIT = 100
class ReplaceByFeeTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            [
                "-limitclustercount=200",
                "-limitancestorcount=50",
                "-limitancestorsize=101",
                "-limitdescendantcount=200",
                "-limitdescendantsize=101",
            ],
            # second node has default mempool parameters
            [
            ],
        ]
        self.supports_cli = False

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

        # TODO: rework too many replacements test to use direct conflicts only
        #self.log.info("Running test too many replacements...")
        #self.test_too_many_replacements()

        #self.log.info("Running test too many replacements using default mempool params...")
        #self.test_too_many_replacements_with_default_mempool_params()

        self.log.info("Running test opt-in...")
        self.test_opt_in()

        self.log.info("Running test RPC...")
        self.test_rpc()

        self.log.info("Running test prioritised transactions...")
        self.test_prioritised_transactions()

        self.log.info("Running test no inherited signaling...")
        self.test_no_inherited_signaling()

        self.log.info("Running test replacement relay fee...")
        self.test_replacement_relay_fee()

        self.log.info("Running test full replace by fee...")
        self.test_fullrbf()

        self.log.info("Passed")

    def make_utxo(self, node, amount, *, confirmed=True, scriptPubKey=None):
        """Create a txout with a given amount and scriptPubKey

        confirmed - txout created will be confirmed in the blockchain;
                    unconfirmed otherwise.
        """
        tx = self.wallet.send_to(from_node=node, scriptPubKey=scriptPubKey or self.wallet.get_scriptPubKey(), amount=amount)

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
        tx = self.wallet.create_self_transfer()["tx"]
        tx1a_txid = self.nodes[0].sendrawtransaction(tx.serialize().hex())

        # Should fail because we haven't changed the fee
        tx.vout[0].scriptPubKey[-1] ^= 1

        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx.serialize().hex(), 0)

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
        while remaining_value > 1 * COIN:
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
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)

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
        n = MAX_REPLACEMENT_LIMIT
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

        # Try again, but with more total transactions than the "max txs
        # double-spent at once" anti-DoS limit.
        # TODO: rework using direct conflict test
        '''
        for n in (MAX_REPLACEMENT_LIMIT + 1, MAX_REPLACEMENT_LIMIT * 2):
            fee = int(0.00001 * COIN)
            tx0_outpoint = self.make_utxo(self.nodes[0], initial_nValue)
            tree_txs = list(branch(tx0_outpoint, initial_nValue, n, fee=fee))
            assert_equal(len(tree_txs), n)

            dbl_tx_hex = self.wallet.create_self_transfer(
                utxo_to_spend=tx0_outpoint,
                sequence=0,
                fee=2 * (Decimal(fee) / COIN) * n,
            )["hex"]
            # This will raise an exception
            assert_raises_rpc_error(-26, "too many potential replacements", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)

            for txid in tree_txs:
                self.nodes[0].getrawtransaction(txid)
        '''

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

        tx1a_utxo = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=utxo1,
            sequence=0,
            fee=Decimal("0.1"),
        )["new_utxo"]

        # Direct spend an output of the transaction we're replacing.
        tx2_hex = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[utxo1, utxo2, tx1a_utxo],
            sequence=0,
            amount_per_output=int(COIN * tx1a_utxo["value"]),
        )["hex"]

        # This will raise an exception
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, tx2_hex, 0)

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
        assert_raises_rpc_error(-26, "too many potential replacements", self.nodes[0].sendrawtransaction, double_tx_hex, 0)

        # If we remove an input, it should pass
        double_tx.vin.pop()
        double_tx_hex = double_tx.serialize().hex()
        self.nodes[0].sendrawtransaction(double_tx_hex, 0)

    def test_too_many_replacements_with_default_mempool_params(self):
        """
        Test rule 5 (do not allow replacements that cause more than 100
        evictions) without having to rely on non-default mempool parameters.

        In order to do this, create a number of "root" UTXOs, and then hang
        enough transactions off of each root UTXO to exceed the MAX_REPLACEMENT_LIMIT.
        Then create a conflicting RBF replacement transaction.
        """
        # Clear mempools to avoid cross-node sync failure.
        for node in self.nodes:
            self.generate(node, 1)
        normal_node = self.nodes[1]
        wallet = MiniWallet(normal_node)

        # This has to be chosen so that the total number of transactions can exceed
        # MAX_REPLACEMENT_LIMIT without having any one tx graph run into the descendant
        # limit; 10 works.
        num_tx_graphs = 10

        # (Number of transactions per graph, rule 5 failure expected)
        cases = [
            # Test the base case of evicting fewer than MAX_REPLACEMENT_LIMIT
            # transactions.
            ((MAX_REPLACEMENT_LIMIT // num_tx_graphs) - 1, False),

            # Test hitting the rule 5 eviction limit.
            (MAX_REPLACEMENT_LIMIT // num_tx_graphs, True),
        ]

        for (txs_per_graph, failure_expected) in cases:
            self.log.debug(f"txs_per_graph: {txs_per_graph}, failure: {failure_expected}")
            # "Root" utxos of each txn graph that we will attempt to double-spend with
            # an RBF replacement.
            root_utxos = []

            # For each root UTXO, create a package that contains the spend of that
            # UTXO and `txs_per_graph` children tx.
            for graph_num in range(num_tx_graphs):
                root_utxos.append(wallet.get_utxo())

                optin_parent_tx = wallet.send_self_transfer_multi(
                    from_node=normal_node,
                    sequence=MAX_BIP125_RBF_SEQUENCE,
                    utxos_to_spend=[root_utxos[graph_num]],
                    num_outputs=txs_per_graph,
                )
                assert_equal(True, normal_node.getmempoolentry(optin_parent_tx['txid'])['bip125-replaceable'])
                new_utxos = optin_parent_tx['new_utxos']

                for utxo in new_utxos:
                    # Create spends for each output from the "root" of this graph.
                    child_tx = wallet.send_self_transfer(
                        from_node=normal_node,
                        utxo_to_spend=utxo,
                    )

                    assert normal_node.getmempoolentry(child_tx['txid'])

            num_txs_invalidated = len(root_utxos) + (num_tx_graphs * txs_per_graph)

            if failure_expected:
                assert num_txs_invalidated > MAX_REPLACEMENT_LIMIT
            else:
                assert num_txs_invalidated <= MAX_REPLACEMENT_LIMIT

            # Now attempt to submit a tx that double-spends all the root tx inputs, which
            # would invalidate `num_txs_invalidated` transactions.
            tx_hex = wallet.create_self_transfer_multi(
                utxos_to_spend=root_utxos,
                fee_per_output=10_000_000,  # absurdly high feerate
            )["hex"]

            if failure_expected:
                assert_raises_rpc_error(
                    -26, "too many potential replacements", normal_node.sendrawtransaction, tx_hex, 0)
            else:
                txid = normal_node.sendrawtransaction(tx_hex, 0)
                assert normal_node.getmempoolentry(txid)

        # Clear the mempool once finished, and rescan the other nodes' wallet
        # to account for the spends we've made on `normal_node`.
        self.generate(normal_node, 1)
        self.wallet.rescan_utxos()

    def test_opt_in(self):
        """Replacing should only work if orig tx opted in"""
        tx0_outpoint = self.make_utxo(self.nodes[0], int(1.1 * COIN))

        # Create a non-opting in transaction
        tx1a_utxo = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx0_outpoint,
            sequence=SEQUENCE_FINAL,
            fee=Decimal("0.1"),
        )["new_utxo"]

        # This transaction isn't shown as replaceable
        assert_equal(self.nodes[0].getmempoolentry(tx1a_utxo["txid"])['bip125-replaceable'], False)

        # Shouldn't be able to double-spend
        tx1b_hex = self.wallet.create_self_transfer(
            utxo_to_spend=tx0_outpoint,
            sequence=0,
            fee=Decimal("0.2"),
        )["hex"]

        # This will raise an exception
        assert_raises_rpc_error(-26, "txn-mempool-conflict", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

        tx1_outpoint = self.make_utxo(self.nodes[0], int(1.1 * COIN))

        # Create a different non-opting in transaction
        tx2a_utxo = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx1_outpoint,
            sequence=0xfffffffe,
            fee=Decimal("0.1"),
        )["new_utxo"]

        # Still shouldn't be able to double-spend
        tx2b_hex = self.wallet.create_self_transfer(
            utxo_to_spend=tx1_outpoint,
            sequence=0,
            fee=Decimal("0.2"),
        )["hex"]

        # This will raise an exception
        assert_raises_rpc_error(-26, "txn-mempool-conflict", self.nodes[0].sendrawtransaction, tx2b_hex, 0)

        # Now create a new transaction that spends from tx1a and tx2a
        # opt-in on one of the inputs
        # Transaction should be replaceable on either input

        tx3a_txid = self.wallet.send_self_transfer_multi(
            from_node=self.nodes[0],
            utxos_to_spend=[tx1a_utxo, tx2a_utxo],
            sequence=[SEQUENCE_FINAL, 0xfffffffd],
            fee_per_output=int(0.1 * COIN),
        )["txid"]

        # This transaction is shown as replaceable
        assert_equal(self.nodes[0].getmempoolentry(tx3a_txid)['bip125-replaceable'], True)

        self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx1a_utxo,
            sequence=0,
            fee=Decimal("0.4"),
        )

        # If tx3b was accepted, tx3c won't look like a replacement,
        # but make sure it is accepted anyway
        self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=tx2a_utxo,
            sequence=0,
            fee=Decimal("0.4"),
        )

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

        if self.is_specified_wallet_compiled():
            self.init_wallet(node=0)
            rawtx2 = self.nodes[0].createrawtransaction([], outs)
            frawtx2a = self.nodes[0].fundrawtransaction(rawtx2, {"replaceable": True})
            frawtx2b = self.nodes[0].fundrawtransaction(rawtx2, {"replaceable": False})

            json0 = self.nodes[0].decoderawtransaction(frawtx2a['hex'])
            json1 = self.nodes[0].decoderawtransaction(frawtx2b['hex'])
            assert_equal(json0["vin"][0]["sequence"], 4294967293)
            assert_equal(json1["vin"][0]["sequence"], 4294967294)

    def test_no_inherited_signaling(self):
        confirmed_utxo = self.wallet.get_utxo()

        # Create an explicitly opt-in parent transaction
        optin_parent_tx = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=confirmed_utxo,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee_rate=Decimal('0.01'),
        )
        assert_equal(True, self.nodes[0].getmempoolentry(optin_parent_tx['txid'])['bip125-replaceable'])

        replacement_parent_tx = self.wallet.create_self_transfer(
            utxo_to_spend=confirmed_utxo,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee_rate=Decimal('0.02'),
        )

        # Test if parent tx can be replaced.
        res = self.nodes[0].testmempoolaccept(rawtxs=[replacement_parent_tx['hex']])[0]

        # Parent can be replaced.
        assert_equal(res['allowed'], True)

        # Create an opt-out child tx spending the opt-in parent
        parent_utxo = self.wallet.get_utxo(txid=optin_parent_tx['txid'])
        optout_child_tx = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=parent_utxo,
            sequence=SEQUENCE_FINAL,
            fee_rate=Decimal('0.01'),
        )

        # Reports true due to inheritance
        assert_equal(True, self.nodes[0].getmempoolentry(optout_child_tx['txid'])['bip125-replaceable'])

        replacement_child_tx = self.wallet.create_self_transfer(
            utxo_to_spend=parent_utxo,
            sequence=SEQUENCE_FINAL,
            fee_rate=Decimal('0.02'),
        )

        # Broadcast replacement child tx
        # BIP 125 :
        # 1. The original transactions signal replaceability explicitly or through inheritance as described in the above
        # Summary section.
        # The original transaction (`optout_child_tx`) doesn't signal RBF but its parent (`optin_parent_tx`) does.
        # The replacement transaction (`replacement_child_tx`) should be able to replace the original transaction.
        # See CVE-2021-31876 for further explanations.
        assert_equal(True, self.nodes[0].getmempoolentry(optin_parent_tx['txid'])['bip125-replaceable'])
        assert_raises_rpc_error(-26, 'txn-mempool-conflict', self.nodes[0].sendrawtransaction, replacement_child_tx["hex"], 0)

        self.log.info('Check that the child tx can still be replaced (via a tx that also replaces the parent)')
        replacement_parent_tx = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=confirmed_utxo,
            sequence=SEQUENCE_FINAL,
            fee_rate=Decimal('0.03'),
        )
        # Check that child is removed and update wallet utxo state
        assert_raises_rpc_error(-5, 'Transaction not in mempool', self.nodes[0].getmempoolentry, optout_child_tx['txid'])
        self.wallet.get_utxo(txid=optout_child_tx['txid'])

    def test_replacement_relay_fee(self):
        tx = self.wallet.send_self_transfer(from_node=self.nodes[0])['tx']

        # Higher fee, higher feerate, different txid, but the replacement does not provide a relay
        # fee conforming to node's `incrementalrelayfee` policy of 1000 sat per KB.
        assert_equal(self.nodes[0].getmempoolinfo()["incrementalrelayfee"], Decimal("0.00001"))
        tx.vout[0].nValue -= 1
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx.serialize().hex())

    def test_fullrbf(self):

        confirmed_utxo = self.make_utxo(self.nodes[0], int(2 * COIN))
        self.restart_node(0, extra_args=["-mempoolfullrbf=1"])
        assert self.nodes[0].getmempoolinfo()["fullrbf"]

        # Create an explicitly opt-out transaction
        optout_tx = self.wallet.send_self_transfer(
            from_node=self.nodes[0],
            utxo_to_spend=confirmed_utxo,
            sequence=MAX_BIP125_RBF_SEQUENCE + 1,
            fee_rate=Decimal('0.01'),
        )
        assert_equal(False, self.nodes[0].getmempoolentry(optout_tx['txid'])['bip125-replaceable'])

        conflicting_tx = self.wallet.create_self_transfer(
                utxo_to_spend=confirmed_utxo,
                sequence=SEQUENCE_FINAL,
                fee_rate=Decimal('0.02'),
        )

        # Send the replacement transaction, conflicting with the optout_tx.
        self.nodes[0].sendrawtransaction(conflicting_tx['hex'], 0)

        # Optout_tx is not anymore in the mempool.
        assert optout_tx['txid'] not in self.nodes[0].getrawmempool()
        assert conflicting_tx['txid'] in self.nodes[0].getrawmempool()

if __name__ == '__main__':
    ReplaceByFeeTest().main()
