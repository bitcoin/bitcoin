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
)
from test_framework.p2p import P2PTxInvStore
from test_framework.script import (
    CScript,
    OP_ELSE,
    OP_ENDIF,
    OP_EQUAL,
    OP_HASH160,
    OP_IF,
    OP_TRUE,
    hash160,
)
from test_framework.script_util import (
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
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
        # txid refers to a mature coinbase tx
        txid = node.getblock(blockhash=blockhash, verbosity=2)['tx'][0]['txid']
        assert_equal(node.getmempoolinfo()['size'], 0)

        # This allows the child (children) to spend the parent's output
        large_witness = b'PreimageWhichMakesTheWitnessSignificantlyLargerThanTheOther'
        hashlock = hash160(large_witness)
        witness_script = CScript([OP_IF, OP_HASH160, hashlock, OP_EQUAL, OP_ELSE, OP_TRUE, OP_ENDIF])

        parent = CTransaction()
        # Spend the coinbase output
        parent.vin.append(CTxIn(COutPoint(int(txid, 16), 0), b""))

        # Create a P2WSH output that the child that will be replaced will spend
        script_pubkey = script_to_p2wsh_script(witness_script)
        parent.vout.append(CTxOut(int(9.99998 * COIN), script_pubkey))
        parent.rehash()

        self.log.info("Mine the parent")
        privkeys = [node.get_deterministic_priv_key().key]
        raw_parent = node.signrawtransactionwithkey(hexstring=parent.serialize().hex(), privkeys=privkeys)['hex']
        parent_txid = node.sendrawtransaction(hexstring=raw_parent, maxfeerate=0)
        self.generate(node, 1)

        # Besides mempool content, also verify tx broadcast
        peer_wtxid_relay = node.add_p2p_connection(P2PTxInvStore())

        # This simple P2WSH pubkey is trivial to spend, will conveniently allow
        # the grandchild to spend the child's output without needing a signature
        simple_witness_script = CScript([OP_TRUE])
        simple_script_pubkey = script_to_p2wsh_script(simple_witness_script)

        # Create a new transaction with witness solving first (complicated) branch;
        # in this case the witness contains the large_witness
        child_original = CTransaction()
        child_original.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 0), b""))
        child_original.vout.append(CTxOut(int(9.99996 * COIN), simple_script_pubkey))
        child_original.wit.vtxinwit.append(CTxInWitness())
        # The b'\x01' will cause OP_IF to take the "true" branch
        child_original.wit.vtxinwit[0].scriptWitness.stack = [large_witness, b'\x01', witness_script]
        child_original_wtxid = child_original.getwtxid()
        child_original_txid = child_original.rehash()

        # Create an identical transaction but with a smaller witness solving second branch
        child_replacement = deepcopy(child_original)
        # The b'' will cause OP_IF to take the "false" branch
        child_replacement.wit.vtxinwit[0].scriptWitness.stack = [b'', witness_script]
        child_replacement_wtxid = child_replacement.getwtxid()
        child_replacement_txid = child_replacement.rehash()

        assert_equal(child_original_txid, child_replacement_txid)
        assert child_original_wtxid != child_replacement_wtxid
        # Convenient name since txid is the same for both
        child_txid = child_original_txid
        # Check the test itself; replacement transaction must be at least 5%
        # smaller than the transaction being replaced.
        assert_greater_than(child_original.get_vsize() * 95, child_replacement.get_vsize() * 100)

        self.log.info("Submit child to the mempool")
        txid_submitted = node.sendrawtransaction(child_original.serialize().hex())
        assert_equal(txid_submitted, child_txid)
        assert_equal(node.getmempoolentry(child_txid)['wtxid'], child_original_wtxid)

        self.log.info("waiting child to broadcast")
        peer_wtxid_relay.wait_for_broadcast([child_original_wtxid])
        assert_equal(node.getmempoolinfo()['unbroadcastcount'], 0)

        self.log.info("testmempoolaccept child, expect already-in-mempool error")
        assert_equal(node.testmempoolaccept([child_original.serialize().hex()]), [{
            'allowed': False,
            'txid': child_txid,
            'wtxid': child_original_wtxid,
            'reject-reason': "txn-already-in-mempool"
        }])

        # This child of child_original should not be removed from the mempool
        # when child_replacement replaces child_original, since the grandchild's
        # input will also refers to child_replacement's output.
        grandchild = CTransaction()
        grandchild.vin.append(CTxIn(COutPoint(int(child_txid, 16), 0), b""))
        grandchild.vout.append(CTxOut(int(9.99992 * COIN), simple_script_pubkey))
        grandchild.wit.vtxinwit.append(CTxInWitness())
        grandchild.wit.vtxinwit[0].scriptWitness.stack = [simple_witness_script]
        grandchild_wtxid = grandchild.getwtxid()
        grandchild_txid = grandchild.rehash()

        self.log.info("Submit grandchild to the mempool")
        txid_submitted = node.sendrawtransaction(grandchild.serialize().hex())
        assert_equal(txid_submitted, grandchild_txid)
        assert_equal(node.getmempoolentry(grandchild_txid)['wtxid'], grandchild_wtxid)
        self.log.info("waiting grandchild to broadcast")
        peer_wtxid_relay.wait_for_broadcast([child_original_wtxid, grandchild_wtxid])
        assert_equal(node.getmempoolinfo()['size'], 2)

        self.log.info("testmempoolaccept replacement child, should allow replacement")
        mempoolaccept = node.testmempoolaccept([child_replacement.serialize().hex()])[0]
        assert_equal(mempoolaccept['txid'], child_txid)
        assert_equal(mempoolaccept['wtxid'], child_replacement_wtxid)
        assert_equal(mempoolaccept['allowed'], True)

        self.log.info("Test that original child remains in the mempool")
        mempoolentry = node.getmempoolentry(child_txid)
        assert_equal(mempoolentry['wtxid'], child_original_wtxid)

        self.log.info("Submit replacement child to the mempool")
        node.sendrawtransaction(child_replacement.serialize().hex())

        self.log.info("Verify replacement child is in the mempool")
        mempoolentry = node.getmempoolentry(child_txid)
        assert_equal(mempoolentry['wtxid'], child_replacement_wtxid)

        self.log.info("waiting replacement child to broadcast")
        wait_list = [child_original_wtxid, grandchild_wtxid, child_replacement_wtxid]
        peer_wtxid_relay.wait_for_broadcast(wait_list)
        assert_equal(node.getmempoolinfo()['unbroadcastcount'], 0)

        self.log.info("Verify grandchild is still in mempool")
        assert_equal(node.getmempoolinfo()['size'], 2)
        mempoolentry = node.getmempoolentry(grandchild_txid)
        assert_equal(mempoolentry['wtxid'], grandchild_wtxid)

        self.log.info("Mine this mempool")
        self.generate(node, 1)
        assert_equal(node.getmempoolinfo()['size'], 0)


if __name__ == '__main__':
    MempoolWtxidTest().main()
