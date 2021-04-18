#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RBF code."""

from decimal import Decimal

from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.script import CScript, OP_DROP
from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, satoshi_round
from test_framework.script_util import DUMMY_P2WPKH_SCRIPT, DUMMY_2_P2WPKH_SCRIPT

MAX_REPLACEMENT_LIMIT = 100

def txToHex(tx):
    return tx.serialize().hex()

def make_utxo(node, amount, confirmed=True, scriptPubKey=DUMMY_P2WPKH_SCRIPT):
    """Create a txout with a given amount and scriptPubKey

    Mines coins as needed.

    confirmed - txouts created will be confirmed in the blockchain;
                unconfirmed otherwise.
    """
    fee = 1*COIN
    while node.getbalance() < satoshi_round((amount + fee)/COIN):
        node.generate(100)

    new_addr = node.getnewaddress()
    txid = node.sendtoaddress(new_addr, satoshi_round((amount+fee)/COIN))
    tx1 = node.getrawtransaction(txid, 1)
    txid = int(txid, 16)
    i = None

    for i, txout in enumerate(tx1['vout']):
        if txout['scriptPubKey']['addresses'] == [new_addr]:
            break
    assert i is not None

    tx2 = CTransaction()
    tx2.vin = [CTxIn(COutPoint(txid, i))]
    tx2.vout = [CTxOut(amount, scriptPubKey)]
    tx2.rehash()

    signed_tx = node.signrawtransactionwithwallet(txToHex(tx2))

    txid = node.sendrawtransaction(signed_tx['hex'], 0)

    # If requested, ensure txouts are confirmed.
    if confirmed:
        mempool_size = len(node.getrawmempool())
        while mempool_size > 0:
            node.generate(1)
            new_size = len(node.getrawmempool())
            # Error out if we have something stuck in the mempool, as this
            # would likely be a bug.
            assert new_size < mempool_size
            mempool_size = new_size

    return COutPoint(int(txid, 16), 0)


class ReplaceByFeeTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [
            [
                "-acceptnonstdtxn=1",
                "-maxorphantx=1000",
                "-limitancestorcount=50",
                "-limitancestorsize=101",
                "-limitdescendantcount=200",
                "-limitdescendantsize=101",
            ],
        ]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Leave IBD
        self.nodes[0].generate(1)

        make_utxo(self.nodes[0], 1*COIN)

        # Ensure nodes are synced
        self.sync_all()

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

        self.log.info("Running test opt-in...")
        self.test_opt_in()

        self.log.info("Running test RPC...")
        self.test_rpc()

        self.log.info("Running test prioritised transactions...")
        self.test_prioritised_transactions()

        self.log.info("Passed")

    def test_simple_doublespend(self):
        """Simple doublespend"""
        tx0_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        # make_utxo may have generated a bunch of blocks, so we need to sync
        # before we can spend the coins generated, or else the resulting
        # transactions might not be accepted by our peers.
        self.sync_all()

        tx1a = CTransaction()
        tx1a.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1a_hex = txToHex(tx1a)
        tx1a_txid = self.nodes[0].sendrawtransaction(tx1a_hex, 0)

        self.sync_all()

        # Should fail because we haven't changed the fee
        tx1b = CTransaction()
        tx1b.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1b.vout = [CTxOut(1 * COIN, DUMMY_2_P2WPKH_SCRIPT)]
        tx1b_hex = txToHex(tx1b)

        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

        # Extra 0.1 WCN fee
        tx1b = CTransaction()
        tx1b.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1b.vout = [CTxOut(int(0.9 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx1b_hex = txToHex(tx1b)
        # Works when enabled
        tx1b_txid = self.nodes[0].sendrawtransaction(tx1b_hex, 0)

        mempool = self.nodes[0].getrawmempool()

        assert tx1a_txid not in mempool
        assert tx1b_txid in mempool

        assert_equal(tx1b_hex, self.nodes[0].getrawtransaction(tx1b_txid))

    def test_doublespend_chain(self):
        """Doublespend of a long chain"""

        initial_nValue = 50*COIN
        tx0_outpoint = make_utxo(self.nodes[0], initial_nValue)

        prevout = tx0_outpoint
        remaining_value = initial_nValue
        chain_txids = []
        while remaining_value > 10*COIN:
            remaining_value -= 1*COIN
            tx = CTransaction()
            tx.vin = [CTxIn(prevout, nSequence=0)]
            tx.vout = [CTxOut(remaining_value, CScript([1, OP_DROP] * 15 + [1]))]
            tx_hex = txToHex(tx)
            txid = self.nodes[0].sendrawtransaction(tx_hex, 0)
            chain_txids.append(txid)
            prevout = COutPoint(int(txid, 16), 0)

        # Whether the double-spend is allowed is evaluated by including all
        # child fees - 40 WCN - so this attempt is rejected.
        dbl_tx = CTransaction()
        dbl_tx.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        dbl_tx.vout = [CTxOut(initial_nValue - 30 * COIN, DUMMY_P2WPKH_SCRIPT)]
        dbl_tx_hex = txToHex(dbl_tx)

        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)

        # Accepted with sufficient fee
        dbl_tx = CTransaction()
        dbl_tx.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        dbl_tx.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        dbl_tx_hex = txToHex(dbl_tx)
        self.nodes[0].sendrawtransaction(dbl_tx_hex, 0)

        mempool = self.nodes[0].getrawmempool()
        for doublespent_txid in chain_txids:
            assert doublespent_txid not in mempool

    def test_doublespend_tree(self):
        """Doublespend of a big tree of transactions"""

        initial_nValue = 50*COIN
        tx0_outpoint = make_utxo(self.nodes[0], initial_nValue)

        def branch(prevout, initial_value, max_txs, tree_width=5, fee=0.0001*COIN, _total_txs=None):
            if _total_txs is None:
                _total_txs = [0]
            if _total_txs[0] >= max_txs:
                return

            txout_value = (initial_value - fee) // tree_width
            if txout_value < fee:
                return

            vout = [CTxOut(txout_value, CScript([i+1]))
                    for i in range(tree_width)]
            tx = CTransaction()
            tx.vin = [CTxIn(prevout, nSequence=0)]
            tx.vout = vout
            tx_hex = txToHex(tx)

            assert len(tx.serialize()) < 100000
            txid = self.nodes[0].sendrawtransaction(tx_hex, 0)
            yield tx
            _total_txs[0] += 1

            txid = int(txid, 16)

            for i, txout in enumerate(tx.vout):
                for x in branch(COutPoint(txid, i), txout_value,
                                  max_txs,
                                  tree_width=tree_width, fee=fee,
                                  _total_txs=_total_txs):
                    yield x

        fee = int(0.0001*COIN)
        n = MAX_REPLACEMENT_LIMIT
        tree_txs = list(branch(tx0_outpoint, initial_nValue, n, fee=fee))
        assert_equal(len(tree_txs), n)

        # Attempt double-spend, will fail because too little fee paid
        dbl_tx = CTransaction()
        dbl_tx.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        dbl_tx.vout = [CTxOut(initial_nValue - fee * n, DUMMY_P2WPKH_SCRIPT)]
        dbl_tx_hex = txToHex(dbl_tx)
        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)

        # 1 WCN fee is enough
        dbl_tx = CTransaction()
        dbl_tx.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        dbl_tx.vout = [CTxOut(initial_nValue - fee * n - 1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        dbl_tx_hex = txToHex(dbl_tx)
        self.nodes[0].sendrawtransaction(dbl_tx_hex, 0)

        mempool = self.nodes[0].getrawmempool()

        for tx in tree_txs:
            tx.rehash()
            assert tx.hash not in mempool

        # Try again, but with more total transactions than the "max txs
        # double-spent at once" anti-DoS limit.
        for n in (MAX_REPLACEMENT_LIMIT+1, MAX_REPLACEMENT_LIMIT*2):
            fee = int(0.0001*COIN)
            tx0_outpoint = make_utxo(self.nodes[0], initial_nValue)
            tree_txs = list(branch(tx0_outpoint, initial_nValue, n, fee=fee))
            assert_equal(len(tree_txs), n)

            dbl_tx = CTransaction()
            dbl_tx.vin = [CTxIn(tx0_outpoint, nSequence=0)]
            dbl_tx.vout = [CTxOut(initial_nValue - 2 * fee * n, DUMMY_P2WPKH_SCRIPT)]
            dbl_tx_hex = txToHex(dbl_tx)
            # This will raise an exception
            assert_raises_rpc_error(-26, "too many potential replacements", self.nodes[0].sendrawtransaction, dbl_tx_hex, 0)

            for tx in tree_txs:
                tx.rehash()
                self.nodes[0].getrawtransaction(tx.hash)

    def test_replacement_feeperkb(self):
        """Replacement requires fee-per-KB to be higher"""
        tx0_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        tx1a = CTransaction()
        tx1a.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1a_hex = txToHex(tx1a)
        self.nodes[0].sendrawtransaction(tx1a_hex, 0)

        # Higher fee, but the fee per KB is much lower, so the replacement is
        # rejected.
        tx1b = CTransaction()
        tx1b.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1b.vout = [CTxOut(int(0.001*COIN), CScript([b'a'*999000]))]
        tx1b_hex = txToHex(tx1b)

        # This will raise an exception due to insufficient fee
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

    def test_spends_of_conflicting_outputs(self):
        """Replacements that spend conflicting tx outputs are rejected"""
        utxo1 = make_utxo(self.nodes[0], int(1.2*COIN))
        utxo2 = make_utxo(self.nodes[0], 3*COIN)

        tx1a = CTransaction()
        tx1a.vin = [CTxIn(utxo1, nSequence=0)]
        tx1a.vout = [CTxOut(int(1.1 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx1a_hex = txToHex(tx1a)
        tx1a_txid = self.nodes[0].sendrawtransaction(tx1a_hex, 0)

        tx1a_txid = int(tx1a_txid, 16)

        # Direct spend an output of the transaction we're replacing.
        tx2 = CTransaction()
        tx2.vin = [CTxIn(utxo1, nSequence=0), CTxIn(utxo2, nSequence=0)]
        tx2.vin.append(CTxIn(COutPoint(tx1a_txid, 0), nSequence=0))
        tx2.vout = tx1a.vout
        tx2_hex = txToHex(tx2)

        # This will raise an exception
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, tx2_hex, 0)

        # Spend tx1a's output to test the indirect case.
        tx1b = CTransaction()
        tx1b.vin = [CTxIn(COutPoint(tx1a_txid, 0), nSequence=0)]
        tx1b.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1b_hex = txToHex(tx1b)
        tx1b_txid = self.nodes[0].sendrawtransaction(tx1b_hex, 0)
        tx1b_txid = int(tx1b_txid, 16)

        tx2 = CTransaction()
        tx2.vin = [CTxIn(utxo1, nSequence=0), CTxIn(utxo2, nSequence=0),
                   CTxIn(COutPoint(tx1b_txid, 0))]
        tx2.vout = tx1a.vout
        tx2_hex = txToHex(tx2)

        # This will raise an exception
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, tx2_hex, 0)

    def test_new_unconfirmed_inputs(self):
        """Replacements that add new unconfirmed inputs are rejected"""
        confirmed_utxo = make_utxo(self.nodes[0], int(1.1*COIN))
        unconfirmed_utxo = make_utxo(self.nodes[0], int(0.1*COIN), False)

        tx1 = CTransaction()
        tx1.vin = [CTxIn(confirmed_utxo)]
        tx1.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1_hex = txToHex(tx1)
        self.nodes[0].sendrawtransaction(tx1_hex, 0)

        tx2 = CTransaction()
        tx2.vin = [CTxIn(confirmed_utxo), CTxIn(unconfirmed_utxo)]
        tx2.vout = tx1.vout
        tx2_hex = txToHex(tx2)

        # This will raise an exception
        assert_raises_rpc_error(-26, "replacement-adds-unconfirmed", self.nodes[0].sendrawtransaction, tx2_hex, 0)

    def test_too_many_replacements(self):
        """Replacements that evict too many transactions are rejected"""
        # Try directly replacing more than MAX_REPLACEMENT_LIMIT
        # transactions

        # Start by creating a single transaction with many outputs
        initial_nValue = 10*COIN
        utxo = make_utxo(self.nodes[0], initial_nValue)
        fee = int(0.0001*COIN)
        split_value = int((initial_nValue-fee)/(MAX_REPLACEMENT_LIMIT+1))

        outputs = []
        for _ in range(MAX_REPLACEMENT_LIMIT+1):
            outputs.append(CTxOut(split_value, CScript([1])))

        splitting_tx = CTransaction()
        splitting_tx.vin = [CTxIn(utxo, nSequence=0)]
        splitting_tx.vout = outputs
        splitting_tx_hex = txToHex(splitting_tx)

        txid = self.nodes[0].sendrawtransaction(splitting_tx_hex, 0)
        txid = int(txid, 16)

        # Now spend each of those outputs individually
        for i in range(MAX_REPLACEMENT_LIMIT+1):
            tx_i = CTransaction()
            tx_i.vin = [CTxIn(COutPoint(txid, i), nSequence=0)]
            tx_i.vout = [CTxOut(split_value - fee, DUMMY_P2WPKH_SCRIPT)]
            tx_i_hex = txToHex(tx_i)
            self.nodes[0].sendrawtransaction(tx_i_hex, 0)

        # Now create doublespend of the whole lot; should fail.
        # Need a big enough fee to cover all spending transactions and have
        # a higher fee rate
        double_spend_value = (split_value-100*fee)*(MAX_REPLACEMENT_LIMIT+1)
        inputs = []
        for i in range(MAX_REPLACEMENT_LIMIT+1):
            inputs.append(CTxIn(COutPoint(txid, i), nSequence=0))
        double_tx = CTransaction()
        double_tx.vin = inputs
        double_tx.vout = [CTxOut(double_spend_value, CScript([b'a']))]
        double_tx_hex = txToHex(double_tx)

        # This will raise an exception
        assert_raises_rpc_error(-26, "too many potential replacements", self.nodes[0].sendrawtransaction, double_tx_hex, 0)

        # If we remove an input, it should pass
        double_tx = CTransaction()
        double_tx.vin = inputs[0:-1]
        double_tx.vout = [CTxOut(double_spend_value, CScript([b'a']))]
        double_tx_hex = txToHex(double_tx)
        self.nodes[0].sendrawtransaction(double_tx_hex, 0)

    def test_opt_in(self):
        """Replacing should only work if orig tx opted in"""
        tx0_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        # Create a non-opting in transaction
        tx1a = CTransaction()
        tx1a.vin = [CTxIn(tx0_outpoint, nSequence=0xffffffff)]
        tx1a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1a_hex = txToHex(tx1a)
        tx1a_txid = self.nodes[0].sendrawtransaction(tx1a_hex, 0)

        # This transaction isn't shown as replaceable
        assert_equal(self.nodes[0].getmempoolentry(tx1a_txid)['bip125-replaceable'], False)

        # Shouldn't be able to double-spend
        tx1b = CTransaction()
        tx1b.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1b.vout = [CTxOut(int(0.9 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx1b_hex = txToHex(tx1b)

        # This will raise an exception
        assert_raises_rpc_error(-26, "txn-mempool-conflict", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

        tx1_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        # Create a different non-opting in transaction
        tx2a = CTransaction()
        tx2a.vin = [CTxIn(tx1_outpoint, nSequence=0xfffffffe)]
        tx2a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx2a_hex = txToHex(tx2a)
        tx2a_txid = self.nodes[0].sendrawtransaction(tx2a_hex, 0)

        # Still shouldn't be able to double-spend
        tx2b = CTransaction()
        tx2b.vin = [CTxIn(tx1_outpoint, nSequence=0)]
        tx2b.vout = [CTxOut(int(0.9 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx2b_hex = txToHex(tx2b)

        # This will raise an exception
        assert_raises_rpc_error(-26, "txn-mempool-conflict", self.nodes[0].sendrawtransaction, tx2b_hex, 0)

        # Now create a new transaction that spends from tx1a and tx2a
        # opt-in on one of the inputs
        # Transaction should be replaceable on either input

        tx1a_txid = int(tx1a_txid, 16)
        tx2a_txid = int(tx2a_txid, 16)

        tx3a = CTransaction()
        tx3a.vin = [CTxIn(COutPoint(tx1a_txid, 0), nSequence=0xffffffff),
                    CTxIn(COutPoint(tx2a_txid, 0), nSequence=0xfffffffd)]
        tx3a.vout = [CTxOut(int(0.9*COIN), CScript([b'c'])), CTxOut(int(0.9*COIN), CScript([b'd']))]
        tx3a_hex = txToHex(tx3a)

        tx3a_txid = self.nodes[0].sendrawtransaction(tx3a_hex, 0)

        # This transaction is shown as replaceable
        assert_equal(self.nodes[0].getmempoolentry(tx3a_txid)['bip125-replaceable'], True)

        tx3b = CTransaction()
        tx3b.vin = [CTxIn(COutPoint(tx1a_txid, 0), nSequence=0)]
        tx3b.vout = [CTxOut(int(0.5 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx3b_hex = txToHex(tx3b)

        tx3c = CTransaction()
        tx3c.vin = [CTxIn(COutPoint(tx2a_txid, 0), nSequence=0)]
        tx3c.vout = [CTxOut(int(0.5 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx3c_hex = txToHex(tx3c)

        self.nodes[0].sendrawtransaction(tx3b_hex, 0)
        # If tx3b was accepted, tx3c won't look like a replacement,
        # but make sure it is accepted anyway
        self.nodes[0].sendrawtransaction(tx3c_hex, 0)

    def test_prioritised_transactions(self):
        # Ensure that fee deltas used via prioritisetransaction are
        # correctly used by replacement logic

        # 1. Check that feeperkb uses modified fees
        tx0_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        tx1a = CTransaction()
        tx1a.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1a_hex = txToHex(tx1a)
        tx1a_txid = self.nodes[0].sendrawtransaction(tx1a_hex, 0)

        # Higher fee, but the actual fee per KB is much lower.
        tx1b = CTransaction()
        tx1b.vin = [CTxIn(tx0_outpoint, nSequence=0)]
        tx1b.vout = [CTxOut(int(0.001*COIN), CScript([b'a'*740000]))]
        tx1b_hex = txToHex(tx1b)

        # Verify tx1b cannot replace tx1a.
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx1b_hex, 0)

        # Use prioritisetransaction to set tx1a's fee to 0.
        self.nodes[0].prioritisetransaction(txid=tx1a_txid, fee_delta=int(-0.1*COIN))

        # Now tx1b should be able to replace tx1a
        tx1b_txid = self.nodes[0].sendrawtransaction(tx1b_hex, 0)

        assert tx1b_txid in self.nodes[0].getrawmempool()

        # 2. Check that absolute fee checks use modified fee.
        tx1_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        tx2a = CTransaction()
        tx2a.vin = [CTxIn(tx1_outpoint, nSequence=0)]
        tx2a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx2a_hex = txToHex(tx2a)
        self.nodes[0].sendrawtransaction(tx2a_hex, 0)

        # Lower fee, but we'll prioritise it
        tx2b = CTransaction()
        tx2b.vin = [CTxIn(tx1_outpoint, nSequence=0)]
        tx2b.vout = [CTxOut(int(1.01 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx2b.rehash()
        tx2b_hex = txToHex(tx2b)

        # Verify tx2b cannot replace tx2a.
        assert_raises_rpc_error(-26, "insufficient fee", self.nodes[0].sendrawtransaction, tx2b_hex, 0)

        # Now prioritise tx2b to have a higher modified fee
        self.nodes[0].prioritisetransaction(txid=tx2b.hash, fee_delta=int(0.1*COIN))

        # tx2b should now be accepted
        tx2b_txid = self.nodes[0].sendrawtransaction(tx2b_hex, 0)

        assert tx2b_txid in self.nodes[0].getrawmempool()

    def test_rpc(self):
        us0 = self.nodes[0].listunspent()[0]
        ins = [us0]
        outs = {self.nodes[0].getnewaddress() : Decimal(1.0000000)}
        rawtx0 = self.nodes[0].createrawtransaction(ins, outs, 0, True)
        rawtx1 = self.nodes[0].createrawtransaction(ins, outs, 0, False)
        json0  = self.nodes[0].decoderawtransaction(rawtx0)
        json1  = self.nodes[0].decoderawtransaction(rawtx1)
        assert_equal(json0["vin"][0]["sequence"], 4294967293)
        assert_equal(json1["vin"][0]["sequence"], 4294967295)

        rawtx2 = self.nodes[0].createrawtransaction([], outs)
        frawtx2a = self.nodes[0].fundrawtransaction(rawtx2, {"replaceable": True})
        frawtx2b = self.nodes[0].fundrawtransaction(rawtx2, {"replaceable": False})

        json0  = self.nodes[0].decoderawtransaction(frawtx2a['hex'])
        json1  = self.nodes[0].decoderawtransaction(frawtx2b['hex'])
        assert_equal(json0["vin"][0]["sequence"], 4294967293)
        assert_equal(json1["vin"][0]["sequence"], 4294967294)

if __name__ == '__main__':
    ReplaceByFeeTest().main()
