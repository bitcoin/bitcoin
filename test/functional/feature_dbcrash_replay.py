#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deterministic recovery from a crash during a chainstate flush.

With -dbcrashratio=1 the node always crashes after writing the first partial
coins batch, leaving the coins database between the last flushed block and
the tip. The last flushed block is on a branch that was reorged out, so on
restart ReplayBlocks must roll back the old branch and roll forward all
blocks up to the new tip.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class ChainstateReplayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def run_test(self):
        node, ref = self.nodes

        self.log.info("Build and flush a short branch on node0 that will be reorged out")
        self.disconnect_nodes(0, 1)
        node_wallet = MiniWallet(node)
        # Spend the newest utxos, which node1's branch never spends, so the rollback must restore them
        newest = sorted(node_wallet.get_utxos(mark_as_spent=False), key=lambda u: u['height'])[-2:]
        for utxo in newest:
            node_wallet.send_self_transfer_multi(from_node=node, utxos_to_spend=[utxo], num_outputs=10)
            self.generate(node, 1, sync_fun=self.no_op)

        self.log.info("Restart node0 so that every partial coins batch write crashes")
        self.restart_node(0, extra_args=["-dbcrashratio=1", "-dbbatchsize=1", "-dbcache=100"])

        self.log.info("Reorg node0 to a longer branch without flushing the coins cache")
        ref_wallet = MiniWallet(ref)
        for _ in range(5):
            ref_wallet.send_self_transfer_multi(from_node=ref, num_outputs=10)
            self.generate(ref, 1, sync_fun=self.no_op)
        self.connect_nodes(0, 1)
        self.sync_blocks()
        tip = ref.getbestblockhash()
        expected_hash = ref.gettxoutsetinfo()["hash_serialized_3"]

        self.log.info("Force a flush on node0 and check that it crashes")
        try:
            node.gettxoutsetinfo()
        except Exception:
            pass
        node.wait_until_stopped()

        self.log.info("Restart node0 and check that ReplayBlocks rolls back and forward to the tip")
        with node.assert_debug_log(["Replaying blocks", "Rolling back", "Rolling forward"]):
            self.start_node(0)
        assert_equal(node.getbestblockhash(), tip)
        assert_equal(node.gettxoutsetinfo()["hash_serialized_3"], expected_hash)


if __name__ == "__main__":
    ChainstateReplayTest(__file__).main()
