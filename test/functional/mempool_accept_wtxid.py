#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test mempool acceptance in case of an already known transaction
with identical non-witness data but different witness.
"""

from copy import deepcopy
from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    sha256,
)
from test_framework.p2p import P2PTxInvStore
from test_framework.script import (
    CScript,
    OP_0,
    OP_ELSE,
    OP_ENDIF,
    OP_EQUAL,
    OP_HASH160,
    OP_IF,
    OP_TRUE,
    hash160,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
)

class MempoolWtxidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]

        self.log.info('Start with empty mempool and 101 blocks')
        # The last 100 coinbase transactions are premature
        blockhash = self.generate(node, 101)[0]
        txid = node.getblock(blockhash=blockhash, verbosity=2)["tx"][0]["txid"]
        assert_equal(node.getmempoolinfo()['size'], 0)

        self.log.info("Submit parent with multiple script branches to mempool")
        hashlock = hash160(b'Preimage')
        witness_script = CScript([OP_IF, OP_HASH160, hashlock, OP_EQUAL, OP_ELSE, OP_TRUE, OP_ENDIF])
        witness_program = sha256(witness_script)
        script_pubkey = CScript([OP_0, witness_program])

        parent = CTransaction()
        parent.vin.append(CTxIn(COutPoint(int(txid, 16), 0), b""))
        parent.vout.append(CTxOut(int(9.99998 * COIN), script_pubkey))
        parent.rehash()

        privkeys = [node.get_deterministic_priv_key().key]
        raw_parent = node.signrawtransactionwithkey(hexstring=parent.serialize().hex(), privkeys=privkeys)['hex']
        parent_txid = node.sendrawtransaction(hexstring=raw_parent, maxfeerate=0)
        self.generate(node, 1)

        peer_wtxid_relay = node.add_p2p_connection(P2PTxInvStore())

        # Create a new transaction with witness solving first branch
        child_witness_script = CScript([OP_TRUE])
        child_witness_program = sha256(child_witness_script)
        child_script_pubkey = CScript([OP_0, child_witness_program])

        child_one = CTransaction()
        child_one.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 0), b""))
        child_one.vout.append(CTxOut(int(9.99996 * COIN), child_script_pubkey))
        child_one.wit.vtxinwit.append(CTxInWitness())
        child_one.wit.vtxinwit[0].scriptWitness.stack = [b'Preimage', b'\x01', witness_script]
        child_one_wtxid = child_one.getwtxid()
        child_one_txid = child_one.rehash()

        # Create another identical transaction with witness solving second branch
        child_two = deepcopy(child_one)
        child_two.wit.vtxinwit[0].scriptWitness.stack = [b'', witness_script]
        child_two_wtxid = child_two.getwtxid()
        child_two_txid = child_two.rehash()

        assert_equal(child_one_txid, child_two_txid)
        assert_not_equal(child_one_wtxid, child_two_wtxid)

        self.log.info("Submit child_one to the mempool")
        txid_submitted = node.sendrawtransaction(child_one.serialize().hex())
        assert_equal(node.getmempoolentry(txid_submitted)['wtxid'], child_one_wtxid)

        peer_wtxid_relay.wait_for_broadcast([child_one_wtxid])
        assert_equal(node.getmempoolinfo()["unbroadcastcount"], 0)

        # testmempoolaccept reports the "already in mempool" error
        assert_equal(node.testmempoolaccept([child_one.serialize().hex()]), [{
            "txid": child_one_txid,
            "wtxid": child_one_wtxid,
            "allowed": False,
            "reject-reason": "txn-already-in-mempool"
        }])
        assert_equal(node.testmempoolaccept([child_two.serialize().hex()])[0], {
            "txid": child_two_txid,
            "wtxid": child_two_wtxid,
            "allowed": False,
            "reject-reason": "txn-same-nonwitness-data-in-mempool"
        })

        # sendrawtransaction will not throw but quits early when the exact same transaction is already in mempool
        node.sendrawtransaction(child_one.serialize().hex())

        self.log.info("Connect another peer that hasn't seen child_one before")
        peer_wtxid_relay_2 = node.add_p2p_connection(P2PTxInvStore())

        self.log.info("Submit child_two to the mempool")
        # sendrawtransaction will not throw but quits early when a transaction with the same non-witness data is already in mempool
        node.sendrawtransaction(child_two.serialize().hex())

        # The node should rebroadcast the transaction using the wtxid of the correct transaction
        # (child_one, which is in its mempool).
        peer_wtxid_relay_2.wait_for_broadcast([child_one_wtxid])
        assert_equal(node.getmempoolinfo()["unbroadcastcount"], 0)

if __name__ == '__main__':
    MempoolWtxidTest().main()
