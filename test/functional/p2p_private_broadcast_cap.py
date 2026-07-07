#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the private-broadcast queue size cap: submissions beyond the cap are
rejected (the queue is not modified), rather than evicting existing entries.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.wallet import MiniWallet


# Must match PrivateBroadcast::MAX_TRANSACTIONS
MAX_TRANSACTIONS = 10_000
OVER_CAP = 5


class PrivateBroadcastCapTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # -privatebroadcast is incompatible with the framework's default
        # -connect=0; allow autoconnect (no actual peers will succeed though).
        self.disable_autoconnect = False
        self.extra_args = [[
            "-privatebroadcast",
            # Fake I2P reachability so the privatebroadcast startup precondition passes.
            "-i2psam=127.0.0.1:1",
            "-proxy=127.0.0.1:1",
        ]]

    def setup_network(self):
        # Skip the framework's default connect_nodes loop. We have a single
        # node and don't need any peer connections.
        self.setup_nodes()

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        # Mature one coinbase to spend.
        self.generate(wallet, 101)

        # Build a parent that fans out to MAX_TRANSACTIONS + OVER_CAP outputs.
        # Inject it directly via generateblock since -privatebroadcast bypasses
        # the mempool
        utxo = wallet.get_utxo()
        parent = wallet.create_self_transfer_multi(
            utxos_to_spend=[utxo],
            num_outputs=MAX_TRANSACTIONS + OVER_CAP,
            fee_per_output=500,
        )
        self.generateblock(node, wallet.get_address(), [parent["hex"]])

        children = [wallet.create_self_transfer(utxo_to_spend=u)
                    for u in parent["new_utxos"]]
        assert_equal(len(children), MAX_TRANSACTIONS + OVER_CAP)

        # Fill the queue exactly to the cap; every distinct submission succeeds.
        self.log.info(f"Filling private broadcast queue to cap ({MAX_TRANSACTIONS} txns)")
        for child in children[:MAX_TRANSACTIONS]:
            node.sendrawtransaction(child["hex"])

        pbinfo = node.getprivatebroadcastinfo()
        assert_equal(len(pbinfo["transactions"]), MAX_TRANSACTIONS)
        present_wtxids = {t["wtxid"] for t in pbinfo["transactions"]}
        for i, child in enumerate(children[:MAX_TRANSACTIONS]):
            assert child["wtxid"] in present_wtxids, \
                f"tx index {i} (wtxid={child['wtxid']}) should be in the queue"

        # Further distinct submissions are rejected with an RPC error, and the
        # queue is left unchanged (nothing evicted to make room).
        self.log.info(f"Submitting {OVER_CAP} more; each should be rejected (queue full)")
        for child in children[MAX_TRANSACTIONS:]:
            assert_raises_rpc_error(-37, "Private broadcast queue is full",
                                    node.sendrawtransaction, child["hex"])

        assert_equal(pbinfo["transactions"], node.getprivatebroadcastinfo()["transactions"])

        self.log.info("Checking abortprivatebroadcast frees a slot for a new submission")
        abort_res = node.abortprivatebroadcast(children[1]["txid"])
        assert_equal([t["wtxid"] for t in abort_res["removed_transactions"]],
                     [children[1]["wtxid"]])
        wtxids = {t["wtxid"] for t in node.getprivatebroadcastinfo()["transactions"]}
        assert_equal(len(wtxids), MAX_TRANSACTIONS - 1)
        assert children[1]["wtxid"] not in wtxids, "aborted tx should be gone from the queue"

        new_child = children[MAX_TRANSACTIONS]  # first previously-rejected tx
        node.sendrawtransaction(new_child["hex"])
        wtxids = {t["wtxid"] for t in node.getprivatebroadcastinfo()["transactions"]}
        assert_equal(len(wtxids), MAX_TRANSACTIONS)
        assert new_child["wtxid"] in wtxids, "freed slot should now hold the new tx"
        assert children[1]["wtxid"] not in wtxids, "aborted tx should not reappear"

        # Re-submitting an already-queued transaction is a no-op, not an error,
        # even when the queue is full.
        self.log.info("Re-submitting an already-queued tx should not error")
        node.sendrawtransaction(children[0]["hex"])
        wtxids = {t["wtxid"] for t in node.getprivatebroadcastinfo()["transactions"]}
        assert_equal(len(wtxids), MAX_TRANSACTIONS)
        assert children[0]["wtxid"] in wtxids, "re-submitted tx should remain queued"

if __name__ == "__main__":
    PrivateBroadcastCapTest(__file__).main()
