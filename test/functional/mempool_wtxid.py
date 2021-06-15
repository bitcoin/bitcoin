#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test mempool acceptance in case of an already known transaction
with identical non-witness data different witness.
"""
from decimal import Decimal

from test_framework.script import (
    CScript,
    OP_0,
    OP_TRUE,
    OP_IF,
    OP_HASH160,
    OP_EQUAL,
    OP_ELSE,
    OP_ENDIF,
    hash160,
)
from test_framework.messages import (
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    COutPoint,
    sha256,
    COIN,
    ToHex,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

class MempoolWtxidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-incrementalrelayfee=0"]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info('Start with empty mempool, and 200 blocks')
        self.privkeys = [node.get_deterministic_priv_key().key]
        self.address = node.get_deterministic_priv_key().address
        self.coins = []
        # The last 100 coinbase transactions are premature
        for b in node.generatetoaddress(110, self.address)[:-100]:
            coinbase = node.getblock(blockhash=b, verbosity=2)["tx"][0]
            self.coins.append({
                "txid": coinbase["txid"],
                "amount": coinbase["vout"][0]["value"],
                "scriptPubKey": coinbase["vout"][0]["scriptPubKey"],
            })
        assert_equal(node.getmempoolinfo()['size'], 0)

        txid = self.coins.pop()["txid"]

        self.log.info("Submit parent with multiple script branches to mempool")
        hashlock = hash160(b'Preimage')
        witness_script = CScript([OP_IF, OP_HASH160, hashlock, OP_EQUAL, OP_ELSE, OP_TRUE, OP_ENDIF])
        witness_program = sha256(witness_script)
        script_pubkey = CScript([OP_0, witness_program])

        parent = CTransaction()
        parent.vin.append(CTxIn(COutPoint(int(txid, 16), 0), b""))
        parent.vout.append(CTxOut(int(9.99998 * COIN), script_pubkey))
        parent.rehash()

        raw_parent = node.signrawtransactionwithkey(hexstring=parent.serialize().hex(), privkeys=self.privkeys)['hex']
        parent_txid = node.sendrawtransaction(hexstring=raw_parent, maxfeerate=0)
        node.generate(1)

        # Create a new segwit transaction with witness solving first branch
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

        # Create another identical segwit transaction with witness solving second branch
        child_two = CTransaction()
        child_two.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 0), b""))
        child_two.vout.append(CTxOut(int(9.99996 * COIN), child_script_pubkey))
        child_two.wit.vtxinwit.append(CTxInWitness())
        child_two.wit.vtxinwit[0].scriptWitness.stack = [b'', witness_script]
        child_two_wtxid = child_two.getwtxid()
        child_two_txid = child_two.rehash()

        assert_equal(child_one_txid, child_two_txid)
        assert child_one_wtxid != child_two_wtxid

        self.log.info("Submit one child to the mempool")
        testres_child_one = node.testmempoolaccept([ToHex(child_one)])[0]
        txid_submitted = node.sendrawtransaction(ToHex(child_one))
        assert_equal(node.getrawmempool(True)[txid_submitted]['wtxid'], child_one_wtxid)

        # testmempoolaccept reports the "already in mempool" error
        assert_equal(node.testmempoolaccept([ToHex(child_one)]),
                     [{"txid": child_one_txid, "wtxid": child_one_wtxid, "allowed": False, "reject-reason": "txn-already-in-mempool"}])
        testres_child_two = node.testmempoolaccept([ToHex(child_two)])[0]
        assert_equal(testres_child_two, {
            "txid": child_two_txid,
            "wtxid": child_two_wtxid,
            "allowed": False,
            "reject-reason": "txn-same-nonwitness-data-in-mempool"
        })

        # sendrawtransaction will not throw but quits early when the exact same transaction is already in mempool
        node.sendrawtransaction(ToHex(child_one))
        # sendrawtransaction will not throw but quits early when a transaction with the same nonwitness data is already in mempool
        node.sendrawtransaction(ToHex(child_two))

        self.log.info("Simulate an attack attempting to prevent package from being accepted")
        # This grandchild is valid with either child_one or child_two; it shouldn't matter which one.
        grandchild = CTransaction()
        grandchild.vin.append(CTxIn(COutPoint(int(child_two_txid, 16), 0), b""))
        # Deduct 0.00001 BTC as fee, and use the same scriptPubKey (OP_TRUE script)
        grandchild.vout.append(CTxOut(int(9.99995 * COIN), child_script_pubkey))
        grandchild.wit.vtxinwit.append(CTxInWitness())
        grandchild.wit.vtxinwit[0].scriptWitness.stack = [child_witness_script]
        grandchild_txid = grandchild.rehash()
        grandchild_wtxid = grandchild.getwtxid()

        # Submit the package { child_two, grandchild }. MemPoolAccept should detect that child_two
        # "matches" (by txid) child_one which is already in the mempool, and trim child_two from the
        # package. The RPC result should include this information: the full result for child_one
        # from its mempool entry, along with a "txn-same-nonwitness-data-in-mempool" for child_two.
        testres_full = node.testmempoolaccept([ToHex(child_two), ToHex(grandchild)])
        assert_equal(testres_full[0], testres_child_one)
        assert_equal(testres_full[1], testres_child_two)
        assert_equal(testres_full[2], {
                "txid": grandchild_txid,
                "wtxid": grandchild_wtxid,
                "allowed": True,
                "vsize": grandchild.get_vsize(),
                "fees": { "base": Decimal("0.00001")}
        })


if __name__ == '__main__':
    MempoolWtxidTest().main()
