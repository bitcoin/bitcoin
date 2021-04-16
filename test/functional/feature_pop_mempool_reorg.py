#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

from test_framework.pop import mine_vbk_blocks, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    disconnect_nodes,
    assert_equal,
)

class PopPayouts(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])

        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)

    def get_best_block(self, node_id):
        hash = self.nodes[node_id].getbestblockhash()
        return self.nodes[node_id].getblock(hash)

    def _test_mempool_reorg_case(self):
        self.log.info("running _test_mempool_reorg_case()")

        self.log.info("disconnect nodes 0 and 1")
        disconnect_nodes(self.nodes[0], 1)

        vbk_blocks_amount = 10
        self.log.info("generate {} vbk blocks".format(vbk_blocks_amount))
        vbk_blocks = mine_vbk_blocks(self.nodes[0], self.apm, vbk_blocks_amount)

        # mine a block on node[0] with these vbk blocks
        node0_tip_hash = self.nodes[0].generate(nblocks=1)[0]
        node0_tip = self.nodes[0].getblock(node0_tip_hash)

        assert len(node0_tip['pop']['data']['vbkblocks']) == vbk_blocks_amount == len(vbk_blocks)
        assert_equal(node0_tip, self.get_best_block(0))

        node1_tip = self.get_best_block(1)

        assert node1_tip['hash'] != node0_tip['hash']

        self.log.info("node 1 mine 10 blocks")
        node1_tip_hash = self.nodes[1].generate(nblocks=10)[9]
        node1_tip = self.nodes[1].getblock(node1_tip_hash)

        assert_equal(node1_tip, self.get_best_block(1))

        connect_nodes(self.nodes[0], 1)
        self.log.info("connect node 1 and node 0")

        self.sync_all(self.nodes, timeout=30)
        self.log.info("nodes[0,1] are in sync")

        assert_equal(self.get_best_block(1), self.get_best_block(0))

        # mine a block on node[1] with these vbk blocks
        tip_hash = self.nodes[1].generate(nblocks=1)[0]
        tip = self.nodes[1].getblock(tip_hash)

        assert len(tip['pop']['data']['vbkblocks']) == vbk_blocks_amount == len(vbk_blocks)

        self.log.info("success! _test_mempool_reorg_case()")


    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._test_mempool_reorg_case()

if __name__ == '__main__':
    PopPayouts().main()