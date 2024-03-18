#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
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
        self.generate(self.nodes[0], 150)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 10)
        self.generate(self.nodes[0], 1)

        # Get a block reward with node 1 and remember the block so we can orphan
        # it later.
        self.sync_blocks()
        blk = self.generate(self.nodes[1], 1)[0]

        # Let the block reward mature and send coins including both
        # the existing balance and the block reward.
        self.generate(self.nodes[0], 150)
        assert_equal(self.nodes[1].getbalance(), 10 + 25)
        pre_reorg_conf_bals = self.nodes[1].getbalances()
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 30)
        orig_chain_tip = self.nodes[0].getbestblockhash()
        self.sync_mempools()

        # Orphan the block reward and make sure that the original coins
        # from the wallet can still be spent.
        self.nodes[0].invalidateblock(blk)
        blocks = self.generate(self.nodes[0], 152)
        conflict_block = blocks[0]
        # We expect the descendants of orphaned rewards to no longer be considered
        assert_equal(self.nodes[1].getbalances()["mine"], {
          "trusted": 10,
          "untrusted_pending": 0,
          "immature": 0,
        })
        # And the unconfirmed tx to be abandoned
        assert_equal(self.nodes[1].gettransaction(txid)["details"][0]["abandoned"], True)

        # The abandoning should persist through reloading
        self.nodes[1].unloadwallet(self.default_wallet_name)
        self.nodes[1].loadwallet(self.default_wallet_name)
        assert_equal(self.nodes[1].gettransaction(txid)["details"][0]["abandoned"], True)

        # If the orphaned reward is reorged back into the main chain, any unconfirmed
        # descendant txs at the time of the original reorg remain abandoned.
        self.nodes[0].invalidateblock(conflict_block)
        self.nodes[0].reconsiderblock(blk)
        assert_equal(self.nodes[0].getbestblockhash(), orig_chain_tip)
        self.generate(self.nodes[0], 3)

        balances = self.nodes[1].getbalances()
        del balances["lastprocessedblock"]
        del pre_reorg_conf_bals["lastprocessedblock"]
        assert_equal(balances, pre_reorg_conf_bals)
        assert_equal(self.nodes[1].gettransaction(txid)["details"][0]["abandoned"], True)


if __name__ == '__main__':
    OrphanedBlockRewardTest().main()
