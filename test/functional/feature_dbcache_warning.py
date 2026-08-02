#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the warning for an undersized coins cache.

When the in-memory UTXO cache reaches its size limit outside of initial
block download, it is emptied and a one-time warning suggesting to raise
-dbcache is logged.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet

# Transactions per block batch and outputs per transaction. Sized so that
# the cumulative number of new coins comfortably exceeds the ~4 MiB coins
# cache (plus unused mempool space) configured below.
NUM_BATCHES = 3
TXS_PER_BATCH = 30
OUTPUTS_PER_TX = 1500


class DbcacheWarningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-dbcache=4", "-maxmempool=5"]]

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        # Mature enough coinbase outputs to fund all spends. Generating
        # recent blocks also takes the node out of IBD, which the warning
        # requires.
        self.generate(self.wallet, COINBASE_MATURITY + NUM_BATCHES * TXS_PER_BATCH)

        self.log.info("Fill the coins cache beyond its limit and check for the warning")
        with node.assert_debug_log(["consider raising -dbcache"]):
            for _ in range(NUM_BATCHES):
                for _ in range(TXS_PER_BATCH):
                    self.wallet.send_self_transfer_multi(
                        from_node=node,
                        num_outputs=OUTPUTS_PER_TX,
                    )
                self.generate(node, 1)


if __name__ == "__main__":
    DbcacheWarningTest(__file__).main()
