#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test fee estimation code."""
from decimal import Decimal
import os
import random

from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_1,
    OP_2,
    OP_DROP,
    OP_TRUE,
)
from test_framework.script_util import (
    script_to_p2sh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    satoshi_round,
)

# Construct 2 trivial P2SH's and the ScriptSigs that spend them
# So we can create many transactions without needing to spend
# time signing.
REDEEM_SCRIPT_1 = CScript([OP_1, OP_DROP])
REDEEM_SCRIPT_2 = CScript([OP_2, OP_DROP])
P2SH_1 = script_to_p2sh_script(REDEEM_SCRIPT_1)
P2SH_2 = script_to_p2sh_script(REDEEM_SCRIPT_2)

# Associated ScriptSig's to spend satisfy P2SH_1 and P2SH_2
SCRIPT_SIG = [CScript([OP_TRUE, REDEEM_SCRIPT_1]), CScript([OP_TRUE, REDEEM_SCRIPT_2])]


def small_txpuzzle_randfee(from_node, conflist, unconflist, amount, min_fee, fee_increment):
    """Create and send a transaction with a random fee.

    The transaction pays to a trivial P2SH script, and assumes that its inputs
    are of the same form.
    The function takes a list of confirmed outputs and unconfirmed outputs
    and attempts to use the confirmed list first for its inputs.
    It adds the newly created outputs to the unconfirmed list.
    Returns (raw transaction, fee)."""

    # It's best to exponentially distribute our random fees
    # because the buckets are exponentially spaced.
    # Exponentially distributed from 1-128 * fee_increment
    rand_fee = float(fee_increment) * (1.1892 ** random.randint(0, 28))
    # Total fee ranges from min_fee to min_fee + 127*fee_increment
    fee = min_fee - fee_increment + satoshi_round(rand_fee)
    tx = CTransaction()
    total_in = Decimal("0.00000000")
    while total_in <= (amount + fee) and len(conflist) > 0:
        t = conflist.pop(0)
        total_in += t["amount"]
        tx.vin.append(CTxIn(COutPoint(int(t["txid"], 16), t["vout"]), b""))
    if total_in <= amount + fee:
        while total_in <= (amount + fee) and len(unconflist) > 0:
            t = unconflist.pop(0)
            total_in += t["amount"]
            tx.vin.append(CTxIn(COutPoint(int(t["txid"], 16), t["vout"]), b""))
        if total_in <= amount + fee:
            raise RuntimeError(f"Insufficient funds: need {amount + fee}, have {total_in}")
    tx.vout.append(CTxOut(int((total_in - amount - fee) * COIN), P2SH_1))
    tx.vout.append(CTxOut(int(amount * COIN), P2SH_2))
    # These transactions don't need to be signed, but we still have to insert
    # the ScriptSig that will satisfy the ScriptPubKey.
    for inp in tx.vin:
        inp.scriptSig = SCRIPT_SIG[inp.prevout.n]
    txid = from_node.sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)
    unconflist.append({"txid": txid, "vout": 0, "amount": total_in - amount - fee})
    unconflist.append({"txid": txid, "vout": 1, "amount": amount})

    return (tx.serialize().hex(), fee)


def split_inputs(from_node, txins, txouts, initial_split=False):
    """Generate a lot of inputs so we can generate a ton of transactions.

    This function takes an input from txins, and creates and sends a transaction
    which splits the value into 2 outputs which are appended to txouts.
    Previously this was designed to be small inputs so they wouldn't have
    a high coin age when the notion of priority still existed."""

    prevtxout = txins.pop()
    tx = CTransaction()
    tx.vin.append(CTxIn(COutPoint(int(prevtxout["txid"], 16), prevtxout["vout"]), b""))

    half_change = satoshi_round(prevtxout["amount"] / 2)
    rem_change = prevtxout["amount"] - half_change - Decimal("0.00001000")
    tx.vout.append(CTxOut(int(half_change * COIN), P2SH_1))
    tx.vout.append(CTxOut(int(rem_change * COIN), P2SH_2))

    # If this is the initial split we actually need to sign the transaction
    # Otherwise we just need to insert the proper ScriptSig
    if (initial_split):
        completetx = from_node.signrawtransactionwithwallet(tx.serialize().hex())["hex"]
    else:
        tx.vin[0].scriptSig = SCRIPT_SIG[prevtxout["vout"]]
        completetx = tx.serialize().hex()
    txid = from_node.sendrawtransaction(hexstring=completetx, maxfeerate=0)
    txouts.append({"txid": txid, "vout": 0, "amount": half_change})
    txouts.append({"txid": txid, "vout": 1, "amount": rem_change})

def check_raw_estimates(node, fees_seen):
    """Call estimaterawfee and verify that the estimates meet certain invariants."""

    delta = 1.0e-6  # account for rounding error
    for i in range(1, 26):
        for _, e in node.estimaterawfee(i).items():
            feerate = float(e["feerate"])
            assert_greater_than(feerate, 0)

            if feerate + delta < min(fees_seen) or feerate - delta > max(fees_seen):
                raise AssertionError(f"Estimated fee ({feerate}) out of range ({min(fees_seen)},{max(fees_seen)})")

def check_smart_estimates(node, fees_seen):
    """Call estimatesmartfee and verify that the estimates meet certain invariants."""

    delta = 1.0e-6  # account for rounding error
    last_feerate = float(max(fees_seen))
    all_smart_estimates = [node.estimatesmartfee(i) for i in range(1, 26)]
    mempoolMinFee = node.getmempoolinfo()['mempoolminfee']
    minRelaytxFee = node.getmempoolinfo()['minrelaytxfee']
    for i, e in enumerate(all_smart_estimates):  # estimate is for i+1
        feerate = float(e["feerate"])
        assert_greater_than(feerate, 0)
        assert_greater_than_or_equal(feerate, float(mempoolMinFee))
        assert_greater_than_or_equal(feerate, float(minRelaytxFee))

        if feerate + delta < min(fees_seen) or feerate - delta > max(fees_seen):
            raise AssertionError(f"Estimated fee ({feerate}) out of range ({min(fees_seen)},{max(fees_seen)})")
        if feerate - delta > last_feerate:
            raise AssertionError(f"Estimated fee ({feerate}) larger than last fee ({last_feerate}) for lower number of confirms")
        last_feerate = feerate

        if i == 0:
            assert_equal(e["blocks"], 2)
        else:
            assert_greater_than_or_equal(i + 1, e["blocks"])

def check_estimates(node, fees_seen):
    check_raw_estimates(node, fees_seen)
    check_smart_estimates(node, fees_seen)


def send_tx(node, utxo, feerate):
    """Broadcast a 1in-1out transaction with a specific input and feerate (sat/vb)."""
    overhead, op, scriptsig, nseq, value, spk = 10, 36, 5, 4, 8, 24
    tx_size = overhead + op + scriptsig + nseq + value + spk
    fee = tx_size * feerate

    tx = CTransaction()
    tx.vin = [CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"]), SCRIPT_SIG[utxo["vout"]])]
    tx.vout = [CTxOut(int(utxo["amount"] * COIN) - fee, P2SH_1)]
    txid = node.sendrawtransaction(tx.serialize().hex())

    return txid


class EstimateFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        # mine non-standard txs (e.g. txs with "dust" outputs)
        # Force fSendTrickle to true (via whitelist.noban)
        self.extra_args = [
            ["-acceptnonstdtxn", "-whitelist=noban@127.0.0.1"],
            ["-acceptnonstdtxn", "-whitelist=noban@127.0.0.1", "-blockmaxweight=68000"],
            ["-acceptnonstdtxn", "-whitelist=noban@127.0.0.1", "-blockmaxweight=32000"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        """
        We'll setup the network to have 3 nodes that all mine with different parameters.
        But first we need to use one node to create a lot of outputs
        which we will use to generate our transactions.
        """
        self.add_nodes(3, extra_args=self.extra_args)
        # Use node0 to mine blocks for input splitting
        # Node1 mines small blocks but that are bigger than the expected transaction rate.
        # NOTE: the CreateNewBlock code starts counting block weight at 4,000 weight,
        # (68k weight is room enough for 120 or so transactions)
        # Node2 is a stingy miner, that
        # produces too small blocks (room for only 55 or so transactions)
        self.start_nodes()
        self.import_deterministic_coinbase_privkeys()
        self.stop_nodes()

    def transact_and_mine(self, numblocks, mining_node):
        min_fee = Decimal("0.00001")
        # We will now mine numblocks blocks generating on average 100 transactions between each block
        # We shuffle our confirmed txout set before each set of transactions
        # small_txpuzzle_randfee will use the transactions that have inputs already in the chain when possible
        # resorting to tx's that depend on the mempool when those run out
        for _ in range(numblocks):
            random.shuffle(self.confutxo)
            for _ in range(random.randrange(100 - 50, 100 + 50)):
                from_index = random.randint(1, 2)
                (txhex, fee) = small_txpuzzle_randfee(self.nodes[from_index], self.confutxo,
                                                      self.memutxo, Decimal("0.005"), min_fee, min_fee)
                tx_kbytes = (len(txhex) // 2) / 1000.0
                self.fees_per_kb.append(float(fee) / tx_kbytes)
            self.sync_mempools(wait=.1)
            mined = mining_node.getblock(self.generate(mining_node, 1)[0], True)["tx"]
            # update which txouts are confirmed
            newmem = []
            for utx in self.memutxo:
                if utx["txid"] in mined:
                    self.confutxo.append(utx)
                else:
                    newmem.append(utx)
            self.memutxo = newmem

    def initial_split(self, node):
        """Split two coinbase UTxOs into many small coins"""
        self.txouts = []
        self.txouts2 = []
        # Split a coinbase into two transaction puzzle outputs
        split_inputs(node, node.listunspent(0), self.txouts, True)

        # Mine
        while len(node.getrawmempool()) > 0:
            self.generate(node, 1, sync_fun=self.no_op)

        # Repeatedly split those 2 outputs, doubling twice for each rep
        # Use txouts to monitor the available utxo, since these won't be tracked in wallet
        reps = 0
        while reps < 5:
            # Double txouts to txouts2
            while len(self.txouts) > 0:
                split_inputs(node, self.txouts, self.txouts2)
            while len(node.getrawmempool()) > 0:
                self.generate(node, 1, sync_fun=self.no_op)
            # Double txouts2 to txouts
            while len(self.txouts2) > 0:
                split_inputs(node, self.txouts2, self.txouts)
            while len(node.getrawmempool()) > 0:
                self.generate(node, 1, sync_fun=self.no_op)
            reps += 1

    def sanity_check_estimates_range(self):
        """Populate estimation buckets, assert estimates are in a sane range and
        are strictly increasing as the target decreases."""
        self.fees_per_kb = []
        self.memutxo = []
        self.confutxo = self.txouts  # Start with the set of confirmed txouts after splitting
        self.log.info("Will output estimates for 1/2/3/6/15/25 blocks")

        for _ in range(2):
            self.log.info("Creating transactions and mining them with a block size that can't keep up")
            # Create transactions and mine 10 small blocks with node 2, but create txs faster than we can mine
            self.transact_and_mine(10, self.nodes[2])
            check_estimates(self.nodes[1], self.fees_per_kb)

            self.log.info("Creating transactions and mining them at a block size that is just big enough")
            # Generate transactions while mining 10 more blocks, this time with node1
            # which mines blocks with capacity just above the rate that transactions are being created
            self.transact_and_mine(10, self.nodes[1])
            check_estimates(self.nodes[1], self.fees_per_kb)

        # Finish by mining a normal-sized block:
        while len(self.nodes[1].getrawmempool()) > 0:
            self.generate(self.nodes[1], 1)
        self.log.info("Final estimates after emptying mempools")
        check_estimates(self.nodes[1], self.fees_per_kb)

    def test_feerate_mempoolminfee(self):
        high_val = 3*self.nodes[1].estimatesmartfee(1)['feerate']
        self.restart_node(1, extra_args=[f'-minrelaytxfee={high_val}'])
        check_estimates(self.nodes[1], self.fees_per_kb)
        self.restart_node(1)

    def sanity_check_rbf_estimates(self, utxos):
        """During 5 blocks, broadcast low fee transactions. Only 10% of them get
        confirmed and the remaining ones get RBF'd with a high fee transaction at
        the next block.
        The block policy estimator should return the high feerate.
        """
        # The broadcaster and block producer
        node = self.nodes[0]
        miner = self.nodes[1]
        # In sat/vb
        low_feerate = 1
        high_feerate = 10
        # Cache the utxos of which to replace the spender after it failed to get
        # confirmed
        utxos_to_respend = []
        txids_to_replace = []

        assert len(utxos) >= 250
        for _ in range(5):
            # Broadcast 45 low fee transactions that will need to be RBF'd
            for _ in range(45):
                u = utxos.pop(0)
                txid = send_tx(node, u, low_feerate)
                utxos_to_respend.append(u)
                txids_to_replace.append(txid)
            # Broadcast 5 low fee transaction which don't need to
            for _ in range(5):
                send_tx(node, utxos.pop(0), low_feerate)
            # Mine the transactions on another node
            self.sync_mempools(wait=.1, nodes=[node, miner])
            for txid in txids_to_replace:
                miner.prioritisetransaction(txid=txid, fee_delta=-COIN)
            self.generate(miner, 1)
            # RBF the low-fee transactions
            while True:
                try:
                    u = utxos_to_respend.pop(0)
                    send_tx(node, u, high_feerate)
                except IndexError:
                    break

        # Mine the last replacement txs
        self.sync_mempools(wait=.1, nodes=[node, miner])
        self.generate(miner, 1)

        # Only 10% of the transactions were really confirmed with a low feerate,
        # the rest needed to be RBF'd. We must return the 90% conf rate feerate.
        high_feerate_kvb = Decimal(high_feerate) / COIN * 10**3
        est_feerate = node.estimatesmartfee(2)["feerate"]
        assert est_feerate == high_feerate_kvb

    def run_test(self):
        self.log.info("This test is time consuming, please be patient")
        self.log.info("Splitting inputs so we can generate tx's")

        # Split two coinbases into many small utxos
        self.start_node(0)
        self.initial_split(self.nodes[0])
        self.log.info("Finished splitting")

        # Now we can connect the other nodes, didn't want to connect them earlier
        # so the estimates would not be affected by the splitting transactions
        self.start_node(1)
        self.start_node(2)
        self.connect_nodes(1, 0)
        self.connect_nodes(0, 2)
        self.connect_nodes(2, 1)
        self.sync_all()

        self.log.info("Testing estimates with single transactions.")
        self.sanity_check_estimates_range()

        # check that the effective feerate is greater than or equal to the mempoolminfee even for high mempoolminfee
        self.log.info("Test fee rate estimation after restarting node with high MempoolMinFee")
        self.test_feerate_mempoolminfee()

        self.log.info("Restarting node with fresh estimation")
        self.stop_node(0)
        fee_dat = os.path.join(self.nodes[0].datadir, self.chain, "fee_estimates.dat")
        os.remove(fee_dat)
        self.start_node(0)
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)

        self.log.info("Testing estimates with RBF.")
        self.sanity_check_rbf_estimates(self.confutxo + self.memutxo)

        self.log.info("Testing that fee estimation is disabled in blocksonly.")
        self.restart_node(0, ["-blocksonly"])
        assert_raises_rpc_error(-32603, "Fee estimation disabled",
                                self.nodes[0].estimatesmartfee, 2)


if __name__ == '__main__':
    EstimateFeeTest().main()
