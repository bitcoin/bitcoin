#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test orphaned block rewards in the wallet."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class OrphanedBlockRewardTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Generate some blocks and obtain some coins on node 0.  We send
        # some balance to node 1, which will hold it as a single coin.
        self.nodes[0].generate(150)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 10)
        self.nodes[0].generate(1)

        # Get a block reward with node 1 and remember the block so we can orphan
        # it later.
        self.sync_blocks()
        blk = self.nodes[1].generate(1)[0]
        self.sync_blocks()

        # Let the block reward mature and send coins including both
        # the existing balance and the block reward.
        self.nodes[0].generate(150)
        assert_equal(self.nodes[1].getbalance(), 10 + 25)
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 30)

        # Orphan the block reward and make sure that the original coins
        # from the wallet can still be spent.
        self.nodes[0].invalidateblock(blk)
        self.nodes[0].generate(152)
        self.sync_blocks()
        # Without the following abandontransaction call, the coins are
        # not considered available yet.
        assert_equal(self.nodes[1].getbalances()["mine"], {
          "trusted": 0,
          "untrusted_pending": 0,
          "immature": 0,
        })
        # The following abandontransaction is necessary to make the later
        # lines succeed, and probably should not be needed; see
        # https://github.com/bitcoin/bitcoin/issues/14148.
        self.nodes[1].abandontransaction(txid)
        assert_equal(self.nodes[1].getbalances()["mine"], {
          "trusted": 10,
          "untrusted_pending": 0,
          "immature": 0,
        })
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 9)

if __name__ == '__main__':
    OrphanedBlockRewardTest().main()
