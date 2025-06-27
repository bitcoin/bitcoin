#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test mempool acceptance in case of an already known transaction
with identical non-witness data but different witness.
"""

from test_framework.messages import (
    COIN,
)
from test_framework.p2p import P2PTxInvStore
from test_framework.script_util import ValidWitnessMalleatedTx
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
        txgen = ValidWitnessMalleatedTx()
        parent = txgen.build_parent_tx(txid, 9.99998 * COIN)

        privkeys = [node.get_deterministic_priv_key().key]
        raw_parent = node.signrawtransactionwithkey(hexstring=parent.serialize().hex(), privkeys=privkeys)['hex']
        signed_parent_txid = node.sendrawtransaction(hexstring=raw_parent, maxfeerate=0)
        self.generate(node, 1)

        peer_wtxid_relay = node.add_p2p_connection(P2PTxInvStore())

        child_one, child_two = txgen.build_malleated_children(signed_parent_txid, 9.99996 * COIN)
        child_one_wtxid = child_one.wtxid_hex
        child_one_txid = child_one.txid_hex
        child_two_wtxid = child_two.wtxid_hex
        child_two_txid = child_two.txid_hex

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
            "reject-reason": "txn-already-in-mempool",
            "reject-details": "txn-already-in-mempool"
        }])
        assert_equal(node.testmempoolaccept([child_two.serialize().hex()])[0], {
            "txid": child_two_txid,
            "wtxid": child_two_wtxid,
            "allowed": False,
            "reject-reason": "txn-same-nonwitness-data-in-mempool",
            "reject-details": "txn-same-nonwitness-data-in-mempool"
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
    MempoolWtxidTest(__file__).main()
