#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test fee estimation code."""
from decimal import Decimal
import random

from test_framework.mininode import CTransaction, CTxIn, CTxOut, COutPoint, ToHex, COIN
from test_framework.script import CScript, OP_1, OP_DROP, OP_2, OP_HASH160, OP_EQUAL, hash160, OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    connect_nodes,
    satoshi_round,
    sync_blocks,
    sync_mempools,
)

# Construct 2 trivial P2SH's and the ScriptSigs that spend them
# So we can create many transactions without needing to spend
# time signing.
REDEEM_SCRIPT_1 = CScript([OP_1, OP_DROP])
REDEEM_SCRIPT_2 = CScript([OP_2, OP_DROP])
P2SH_1 = CScript([OP_HASH160, hash160(REDEEM_SCRIPT_1), OP_EQUAL])
P2SH_2 = CScript([OP_HASH160, hash160(REDEEM_SCRIPT_2), OP_EQUAL])

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
            raise RuntimeError("Insufficient funds: need %d, have %d" % (amount + fee, total_in))
    tx.vout.append(CTxOut(int((total_in - amount - fee) * COIN), P2SH_1))
    tx.vout.append(CTxOut(int(amount * COIN), P2SH_2))
    # These transactions don't need to be signed, but we still have to insert
    # the ScriptSig that will satisfy the ScriptPubKey.
    for inp in tx.vin:
        inp.scriptSig = SCRIPT_SIG[inp.prevout.n]
    txid = from_node.sendrawtransaction(ToHex(tx), True)
    unconflist.append({"txid": txid, "vout": 0, "amount": total_in - amount - fee})
    unconflist.append({"txid": txid, "vout": 1, "amount": amount})

    return (ToHex(tx), fee)

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
        completetx = from_node.signrawtransaction(ToHex(tx))["hex"]
    else:
        tx.vin[0].scriptSig = SCRIPT_SIG[prevtxout["vout"]]
        completetx = ToHex(tx)
    txid = from_node.sendrawtransaction(completetx, True)
    txouts.append({"txid": txid, "vout": 0, "amount": half_change})
    txouts.append({"txid": txid, "vout": 1, "amount": rem_change})

def check_estimates(node, fees_seen, max_invalid):
    """Call estimatesmartfee and verify that the estimates meet certain invariants."""

    delta = 1.0e-6  # account for rounding error
    last_feerate = float(max(fees_seen))
    all_smart_estimates = [node.estimatesmartfee(i) for i in range(1, 26)]
    for i, e in enumerate(all_smart_estimates):  # estimate is for i+1
        feerate = float(e["feerate"])
        assert_greater_than(feerate, 0)

        if feerate + delta < min(fees_seen) or feerate - delta > max(fees_seen):
            raise AssertionError("Estimated fee (%f) out of range (%f,%f)"
                                 % (feerate, min(fees_seen), max(fees_seen)))
        if feerate - delta > last_feerate:
            raise AssertionError("Estimated fee (%f) larger than last fee (%f) for lower number of confirms"
                                 % (feerate, last_feerate))
        last_feerate = feerate

        if i == 0:
            assert_equal(e["blocks"], 2)
        else:
            assert_greater_than_or_equal(i + 1, e["blocks"])

class EstimateFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3

    def setup_network(self):
        """
        We'll setup the network to have 3 nodes that all mine with different parameters.
        But first we need to use one node to create a lot of outputs
        which we will use to generate our transactions.
        """
        self.add_nodes(3, extra_args=[["-maxorphantx=1000", "-whitelist=127.0.0.1"],
                                      ["-blockmaxsize=17000", "-maxorphantx=1000"],
                                      ["-blockmaxsize=8000", "-maxorphantx=1000"]])
        # Use node0 to mine blocks for input splitting
        # Node1 mines small blocks but that are bigger than the expected transaction rate.
        # NOTE: the CreateNewBlock code starts counting block size at 1,000 bytes,
        # (17k is room enough for 110 or so transactions)
        # Node2 is a stingy miner, that
        # produces too small blocks (room for only 55 or so transactions)

    def transact_and_mine(self, numblocks, mining_node):
        min_fee = Decimal("0.00001")
        # We will now mine numblocks blocks generating on average 100 transactions between each block
        # We shuffle our confirmed txout set before each set of transactions
        # small_txpuzzle_randfee will use the transactions that have inputs already in the chain when possible
        # resorting to tx's that depend on the mempool when those run out
        for i in range(numblocks):
            random.shuffle(self.confutxo)
            for j in range(random.randrange(100 - 50, 100 + 50)):
                from_index = random.randint(1, 2)
                (txhex, fee) = small_txpuzzle_randfee(self.nodes[from_index], self.confutxo,
                                                      self.memutxo, Decimal("0.005"), min_fee, min_fee)
                tx_kbytes = (len(txhex) // 2) / 1000.0
                self.fees_per_kb.append(float(fee) / tx_kbytes)
            sync_mempools(self.nodes[0:3], wait=.1)
            mined = mining_node.getblock(mining_node.generate(1)[0], True)["tx"]
            sync_blocks(self.nodes[0:3], wait=.1)
            # update which txouts are confirmed
            newmem = []
            for utx in self.memutxo:
                if utx["txid"] in mined:
                    self.confutxo.append(utx)
                else:
                    newmem.append(utx)
            self.memutxo = newmem

    def run_test(self):
        self.log.info("This test is time consuming, please be patient")
        self.log.info("Splitting inputs so we can generate tx's")

        # Start node0
        self.start_node(0)
        self.txouts = []
        self.txouts2 = []
        # Split a coinbase into two transaction puzzle outputs
        split_inputs(self.nodes[0], self.nodes[0].listunspent(0), self.txouts, True)

        # Mine
        while (len(self.nodes[0].getrawmempool()) > 0):
            self.nodes[0].generate(1)

        # Repeatedly split those 2 outputs, doubling twice for each rep
        # Use txouts to monitor the available utxo, since these won't be tracked in wallet
        reps = 0
        while (reps < 5):
            # Double txouts to txouts2
            while (len(self.txouts) > 0):
                split_inputs(self.nodes[0], self.txouts, self.txouts2)
            while (len(self.nodes[0].getrawmempool()) > 0):
                self.nodes[0].generate(1)
            # Double txouts2 to txouts
            while (len(self.txouts2) > 0):
                split_inputs(self.nodes[0], self.txouts2, self.txouts)
            while (len(self.nodes[0].getrawmempool()) > 0):
                self.nodes[0].generate(1)
            reps += 1
        self.log.info("Finished splitting")

        # Now we can connect the other nodes, didn't want to connect them earlier
        # so the estimates would not be affected by the splitting transactions
        self.start_node(1)
        self.start_node(2)
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[2], 1)

        self.sync_all()

        self.fees_per_kb = []
        self.memutxo = []
        self.confutxo = self.txouts  # Start with the set of confirmed txouts after splitting
        self.log.info("Will output estimates for 1/2/3/6/15/25 blocks")

        for i in range(2):
            self.log.info("Creating transactions and mining them with a block size that can't keep up")
            # Create transactions and mine 10 small blocks with node 2, but create txs faster than we can mine
            self.transact_and_mine(10, self.nodes[2])
            check_estimates(self.nodes[1], self.fees_per_kb, 14)

            self.log.info("Creating transactions and mining them at a block size that is just big enough")
            # Generate transactions while mining 10 more blocks, this time with node1
            # which mines blocks with capacity just above the rate that transactions are being created
            self.transact_and_mine(10, self.nodes[1])
            check_estimates(self.nodes[1], self.fees_per_kb, 2)

        # Finish by mining a normal-sized block:
        while len(self.nodes[1].getrawmempool()) > 0:
            self.nodes[1].generate(1)

        sync_blocks(self.nodes[0:3], wait=.1)
        self.log.info("Final estimates after emptying mempools")
        check_estimates(self.nodes[1], self.fees_per_kb, 2)

if __name__ == '__main__':
    EstimateFeeTest().main()
