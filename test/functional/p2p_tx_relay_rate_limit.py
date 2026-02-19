#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction relay rate limiting via token buckets.

With -txsendrate=R, the inbound count bucket has capacity R*30. Transactions
submitted within that capacity relay immediately; excess transactions queue
in a global backlog and drain as the bucket refills at R tokens/second.
"""

import time
from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than
from test_framework.wallet import MiniWallet

SEND_RATE = 2                  # -txsendrate value
BUCKET_CAP = SEND_RATE * 30   # count bucket capacity (60)
NUM_TXS = 80                  # total transactions to submit


class TxRelayRateLimitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-txsendrate={SEND_RATE}']]

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)

        node.setmocktime(int(time.time()))

        # Mine enough blocks for mature coinbase UTXOs (extra headroom for
        # additional txs that may be needed to maintain backlog while waiting
        # for the trickle timer)
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

        # Submit NUM_TXS transactions at frozen time.
        # First BUCKET_CAP go via immediate relay; the rest queue in the backlog.
        # Place the RBF original past BUCKET_CAP so it lands in the backlog.
        RBF_INDEX = NUM_TXS - 5
        all_wtxids = []
        for i in range(NUM_TXS):
            if i == RBF_INDEX:
                node.sendrawtransaction(tx_rbf_orig['hex'])
                all_wtxids.append(tx_rbf_orig['wtxid'])
            else:
                tx = wallet.send_self_transfer(from_node=node)
                all_wtxids.append(tx['wtxid'])

        # Verify the backlog formed: exactly the excess beyond bucket capacity
        info = node.getnetworkinfo()
        backlog = info['inv_buckets']['inbound']['backlog']
        self.log.info(f"Backlog after burst: {backlog}")
        assert_equal(backlog, NUM_TXS - BUCKET_CAP)

        # RBF the backlogged transaction while the bucket is still empty,
        # so the replacement also goes to the backlog.
        self.log.info("RBF'ing a backlogged transaction")
        node.sendrawtransaction(tx_rbf_repl['hex'])

        # Backlog now has (NUM_TXS - BUCKET_CAP + 1) entries.  The replaced
        # original's wtxid will be silently dropped when the backlog is
        # processed (SortMiningScoreWithTopology won't find it in the mempool).

        # Advance time in increments until the trickle timer fires and
        # announcements appear.  Each 5s bump refills SEND_RATE*5 count
        # tokens which CatchupRelayTransactions drains from the backlog,
        # but nothing is actually announced until the per-peer trickle
        # timer fires.  We add extra txs each round to ensure backlog
        # remains for the partial drain check.
        self.log.info("Advancing time for partial backlog drain")
        total_txs = NUM_TXS
        while True:
            node.bumpmocktime(5)
            peer.sync_with_ping()
            announced = set(peer.get_invs())
            if len(announced) > 0:
                break
            # Bucket refilled during catchup; add more txs to maintain backlog
            for _ in range(SEND_RATE * 5):
                tx = wallet.send_self_transfer(from_node=node)
                all_wtxids.append(tx['wtxid'])
            total_txs += SEND_RATE * 5

        self.log.info(f"Announced after partial drain: {len(announced)} (total txs: {total_txs})")
        assert_greater_than(len(announced), BUCKET_CAP)  # immediate + some catchup
        assert_greater_than(total_txs, len(announced))    # but not all yet

        # The replaced original should not have been announced (it was backlogged
        # and got evicted from the mempool before the backlog was processed)
        assert int(tx_rbf_orig['wtxid'], 16) not in announced

        # Advance time enough for the remaining backlog to fully drain
        self.log.info("Advancing time for full backlog drain")
        node.bumpmocktime(60)
        peer.sync_with_ping()

        announced = set(peer.get_invs())
        self.log.info(f"Total announced: {len(announced)}")

        # All unique transactions should have been announced:
        #   BUCKET_CAP immediate originals
        # + surviving backlogged originals (minus the RBF'd one)
        # + 1 replacement
        # + any extra txs added while waiting for the trickle timer
        assert_equal(len(announced), total_txs)

        # Confirm the replaced original was never announced, replacement was
        assert int(tx_rbf_orig['wtxid'], 16) not in announced
        assert int(tx_rbf_repl['wtxid'], 16) in announced

        # Backlog should be fully drained
        info = node.getnetworkinfo()
        assert_equal(info['inv_buckets']['inbound']['backlog'], 0)

        self.log.info("Rate limiting and RBF backlog cleanup test passed")


if __name__ == '__main__':
    TxRelayRateLimitTest(__file__).main()
