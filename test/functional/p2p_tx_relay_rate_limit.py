#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction relay rate limiting via token buckets.

With -txsendrate=R, the inbound count bucket has capacity R*30. A broadcast
transaction is relayed immediately while the bucket has tokens; once it is
exhausted the excess transactions queue in a global backlog and drain as the
bucket refills (R tokens/second as mocktime advances).
"""

from decimal import Decimal
import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

SEND_RATE = 2                 # -txsendrate value
BUCKET_CAP = SEND_RATE * 30   # count bucket capacity (60)
NUM_TXS = 80                  # total transactions to submit


class TxRelayRateLimitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-txsendrate={SEND_RATE}']]

    def inbound_backlog(self, node):
        return node.getnetworkinfo()['inv_buckets']['inbound']['backlog']

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        node.setmocktime(int(time.time()))

        # Mine enough blocks for mature coinbase UTXOs.
        self.generate(wallet, COINBASE_MATURITY + NUM_TXS + 50)

        # Connect an inbound peer (negotiates wtxid relay by default)
        peer = node.add_p2p_connection(P2PTxInvStore())

        # Advance time so the peer's trickle timer initializes
        node.bumpmocktime(10)
        peer.sync_with_ping()
        assert_equal(len(peer.get_invs()), 0)

        # Verify the configured send rate
        assert_equal(node.getnetworkinfo()['tx_send_rate'], SEND_RATE)

        self.test_rate_limit_and_rbf(node, wallet, peer)

    def test_rate_limit_and_rbf(self, node, wallet, peer):
        self.log.info(f"Submitting {NUM_TXS} transactions at frozen time (bucket capacity {BUCKET_CAP})")

        # Prepare an RBF pair: original and replacement spending the same UTXO.
        # Both are created upfront so we can submit the replacement later.
        rbf_utxo = wallet.get_utxo()
        tx_rbf_orig = wallet.create_self_transfer(utxo_to_spend=rbf_utxo)
        tx_rbf_repl = wallet.create_self_transfer(utxo_to_spend=rbf_utxo, fee_rate=Decimal("0.009"))

        # Submit NUM_TXS transactions at frozen time. Each broadcast is relayed
        # immediately while the count bucket has tokens, so the first BUCKET_CAP
        # are handed straight to the peer and the rest queue in the backlog. The
        # RBF original is placed in the backlogged tail.
        RBF_INDEX = NUM_TXS - 5
        for i in range(NUM_TXS):
            if i == RBF_INDEX:
                node.sendrawtransaction(tx_rbf_orig['hex'])
            else:
                wallet.send_self_transfer(from_node=node)

        # The excess beyond the bucket capacity is backlogged. Nothing is
        # announced yet -- the per-peer trickle timer hasn't fired (frozen time).
        self.log.info(f"Backlog after burst: {self.inbound_backlog(node)}")
        assert_equal(self.inbound_backlog(node), NUM_TXS - BUCKET_CAP)
        assert_equal(len(peer.get_invs()), 0)

        # RBF the backlogged original while time is still frozen, so the
        # replacement also queues in the backlog (the bucket is exhausted). The
        # original's wtxid stays in the backlog vector for now but is dropped
        # when the backlog is processed, since it is no longer in the mempool.
        self.log.info("RBF'ing a backlogged transaction")
        node.sendrawtransaction(tx_rbf_repl['hex'])
        assert_equal(self.inbound_backlog(node), NUM_TXS - BUCKET_CAP + 1)

        # Advance time so the bucket refills and the backlog drains. Loop until
        # the backlog is empty and every surviving tx has trickled out.
        self.log.info("Advancing time to drain the backlog")
        for _ in range(30):
            if self.inbound_backlog(node) == 0 and len(peer.get_invs()) == NUM_TXS:
                break
            node.bumpmocktime(4)
            peer.sync_with_ping()

        announced = set(peer.get_invs())
        self.log.info(f"Total announced: {len(announced)}")

        # Every surviving transaction is announced: the burst minus the dropped
        # RBF original, plus the replacement.
        assert_equal(self.inbound_backlog(node), 0)
        assert_equal(len(announced), NUM_TXS)
        assert int(tx_rbf_orig['wtxid'], 16) not in announced
        assert int(tx_rbf_repl['wtxid'], 16) in announced

        self.log.info("Rate limiting and RBF backlog cleanup test passed")


if __name__ == '__main__':
    TxRelayRateLimitTest(__file__).main()
