#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that removeForReorg correctly handles BIP68 transactions."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

SEQ_BIP68_DISABLE = 0xFFFFFFFE
SEQ_BIP68_ZERO = 0x00000000


class MempoolReorgBip68StaleLocksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]]

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.generate(self.wallet, 200)

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

        self.log.info("child_A seq=0xFFFFFFFE: %s", child_A["txid"][:16])
        self.log.info("child_B seq=0x00000000: %s", child_B["txid"][:16])

        block_Y = self.generateblock(
            node, output=self.wallet.get_address(), transactions=[],
        )["hash"]

        node.invalidateblock(block_Y)
        node.invalidateblock(block_setup)
        assert node.getblockcount() == H - 1

        mp = node.getrawmempool()

        assert child_A["txid"] in mp, "child_A (BIP68 disabled) must survive"
        assert child_B["txid"] in mp, "child_B (BIP68 enabled) must survive after fix"

        self.log.info("child_A (BIP68 disabled): SURVIVED")
        self.log.info("child_B (BIP68 enabled):  SURVIVED (stale lockpoints recalculated)")
        self.log.info("PASS: removeForReorg correctly recalculates BIP68 lockpoints")


if __name__ == "__main__":
    MempoolReorgBip68StaleLocksTest(__file__).main()
