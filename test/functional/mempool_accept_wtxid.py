#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test mempool acceptance in case of an already known transaction
with identical non-witness data but different witness.
"""

from test_framework.p2p import P2PTxInvStore
from test_framework.script_util import build_malleated_tx_package
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
)
from test_framework.wallet import (
    MiniWallet,
)

class MempoolWtxidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        mini_wallet = MiniWallet(node)
        self.log.info('Start with pre-generated blocks')

        assert_equal(node.getmempoolinfo()['size'], 0)

        self.log.info("Submit parent with multiple script branches to mempool")

        parent = mini_wallet.create_self_transfer()["tx"]
        parent_amount = parent.vout[0].nValue - 10000
        child_amount = parent_amount - 10000
        parent, child_one, child_two = build_malleated_tx_package(
            parent=parent,
            rebalance_parent_output_amount=parent_amount,
            child_amount=child_amount
        )

        mini_wallet.sendrawtransaction(from_node=node, tx_hex=parent.serialize().hex())

        self.generate(node, 1)

        peer_wtxid_relay = node.add_p2p_connection(P2PTxInvStore())

        assert_equal(child_one.txid_hex, child_two.txid_hex)
        assert_not_equal(child_one.wtxid_hex, child_two.wtxid_hex)

        self.log.info("Submit child_one to the mempool")
        txid_submitted = node.sendrawtransaction(child_one.serialize().hex())
        assert_equal(node.getmempoolentry(txid_submitted)['wtxid'], child_one.wtxid_hex)
        peer_wtxid_relay.wait_for_broadcast([child_one.wtxid_hex])
        assert_equal(node.getmempoolinfo()["unbroadcastcount"], 0)

        # testmempoolaccept reports the "already in mempool" error
        assert_equal(node.testmempoolaccept([child_one.serialize().hex()]), [{
            "txid": child_one.txid_hex,
            "wtxid": child_one.wtxid_hex,
            "allowed": False,
            "reject-reason": "txn-already-in-mempool",
            "reject-details": "txn-already-in-mempool"
        }])
        assert_equal(node.testmempoolaccept([child_two.serialize().hex()])[0], {
            "txid": child_two.txid_hex,
            "wtxid": child_two.wtxid_hex,
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
        peer_wtxid_relay_2.wait_for_broadcast([child_one.wtxid_hex])
        assert_equal(node.getmempoolinfo()["unbroadcastcount"], 0)

if __name__ == '__main__':
    MempoolWtxidTest(__file__).main()
