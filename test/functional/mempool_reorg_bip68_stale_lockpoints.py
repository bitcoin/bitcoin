#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that removeForReorg correctly handles BIP68 transactions."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than
from test_framework.wallet import MiniWallet

SEQ_BIP68_DISABLE = 0xFFFFFFFE
SEQ_BIP68_ZERO = 0x00000000
SEQ_BIP68_ZERO_TIME = 0x00400000  # time-based, zero relative lock


class MempoolReorgBip68StaleLocksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 200)

        # Confirm a utxo one block BELOW the block we will invalidate, so that
        # the mixed spend below keeps a confirmed input through the reorg.
        mixed_funding = self.wallet.send_self_transfer(
            from_node=node, confirmed_only=True,
        )
        self.generate(node, 1)

        funding = self.wallet.send_self_transfer_multi(
            from_node=node, num_outputs=2, confirmed_only=True,
        )
        block_setup = self.generate(node, 1)[0]
        self.wallet.rescan_utxos(include_mempool=True)

        H = node.getblockcount()
        self.log.info("Setup complete at height H=%d", H)

        parent_A = self.wallet.send_self_transfer(
            from_node=node, utxo_to_spend=funding["new_utxos"][0],
        )
        parent_B = self.wallet.send_self_transfer(
            from_node=node, utxo_to_spend=funding["new_utxos"][1],
        )
        child_A = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=parent_A["new_utxo"],
            sequence=SEQ_BIP68_DISABLE,
        )
        child_B = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=parent_B["new_utxo"],
            sequence=SEQ_BIP68_ZERO,
        )

        # child_mixed spends one confirmed input (mixed_funding, confirmed
        # below block_setup) and one unconfirmed input (child_B's output).
        # Its cached maxInputBlock points to the confirmed input's block,
        # not the genesis sentinel, so a genesis-only check would miss it.
        child_mixed = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[mixed_funding["new_utxo"], child_B["new_utxo"]],
            sequence=SEQ_BIP68_ZERO,
        )
        node.sendrawtransaction(child_mixed["hex"])

        self.log.info("child_A seq=0xFFFFFFFE: %s", child_A["txid"][:16])
        self.log.info("child_B seq=0x00000000: %s", child_B["txid"][:16])
        self.log.info("child_mixed confirmed+unconfirmed inputs, seq=0x00000000: %s", child_mixed["txid"][:16])

        node.invalidateblock(block_setup)
        assert_equal(node.getblockcount(), H - 1)

        mp = node.getrawmempool()

        assert child_A["txid"] in mp, "child_A (BIP68 disabled) must survive"
        assert child_B["txid"] in mp, "child_B (BIP68 enabled) must survive after fix"
        assert child_mixed["txid"] in mp, "child_mixed (confirmed + unconfirmed inputs) must survive after fix"

        self.log.info("child_A (BIP68 disabled): SURVIVED")
        self.log.info("child_B (BIP68 enabled):  SURVIVED (stale lockpoints recalculated)")
        self.log.info("child_mixed (mixed inputs): SURVIVED (stale lockpoints recalculated)")
        self.log.info("PASS: removeForReorg correctly recalculates BIP68 lockpoints")

        # ---- Time-based variant ----
        # The MTP of a block is the median of the previous 11. First mine 6
        # blocks sharing one timestamp so the median is stable and predictable,
        # then 6 blocks with a much higher timestamp so the median moves up.
        # A tx with a zero-value time-based lock entering the mempool now caches
        # lockpoints against the raised MTP. Invalidating the high-timestamp
        # blocks drops the MTP back down, making the cached lockpoints stale
        # while the transaction itself stays valid (its relative lock is zero).
        H2 = node.getblockcount()
        current_time = node.getblockheader(node.getbestblockhash())["time"]

        for _ in range(6):
            node.setmocktime(current_time + 1000)
            self.generate(node, 1)
        median_old = node.getblockheader(node.getbestblockhash())["mediantime"]
        self.log.info("median before raising timestamps: %d", median_old)

        node.setmocktime(current_time + 20000)
        block_time_setup = self.generate(node, 6)[0]
        median_new = node.getblockheader(node.getbestblockhash())["mediantime"]
        self.log.info("median after raising timestamps: %d", median_new)

        assert_greater_than(median_new, median_old)
        assert_equal(node.getblockcount(), H2 + 12)

        self.wallet.rescan_utxos(include_mempool=True)
        parent_time = self.wallet.send_self_transfer(
            from_node=node, confirmed_only=True,
        )
        child_time = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=parent_time["new_utxo"],
            sequence=SEQ_BIP68_ZERO_TIME,
        )
        self.log.info("child_time seq=0x00400000: %s", child_time["txid"][:16])

        node.invalidateblock(block_time_setup)
        assert_equal(node.getblockcount(), H2 + 6)

        mp = node.getrawmempool()
        assert child_time["txid"] in mp, "child_time (BIP68 time-based) must survive after fix"
        self.log.info("child_time (time-based): SURVIVED (stale lockpoints recalculated)")

        node.setmocktime(0)


if __name__ == "__main__":
    MempoolReorgBip68StaleLocksTest(__file__).main()
