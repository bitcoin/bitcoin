#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Test with multiple nodes, and multiple PoP endorsements, checking to make sure nodes stay in sync.
"""

from test_framework.pop import KEYSTONE_INTERVAL, endorse_block
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    sync_mempools, disconnect_nodes, assert_equal,
)


class PopFr(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [["-txindex"], ["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypopminer()

    def setup_network(self):
        self.setup_nodes()

        # all nodes connected and synced
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)
            self.sync_all()

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def _shorter_endorsed_chain_wins(self):
        self.log.warning("starting _shorter_endorsed_chain_wins()")

        # all nodes start with 103 blocks
        self.nodes[0].generate(nblocks=103)
        self.sync_all(self.nodes)
        self.log.info("all 3 nodes are at block 103")

        # node2 is disconnected from others
        disconnect_nodes(self.nodes[2], 0)
        disconnect_nodes(self.nodes[2], 1)
        self.log.info("node2 is disconnected")

        # node2 mines another 97 blocks, so total height is 200
        self.nodes[2].generate(nblocks=97)

        # fork A is at 103
        # fork B is at 200
        self.nodes[2].waitforblockheight(200)
        self.log.info("node2 mined 97 more blocks, total height is 200")

        bestblocks = [self.get_best_block(x) for x in self.nodes]

        assert bestblocks[0] != bestblocks[2], "node[0,2] have same best hashes"
        assert bestblocks[0] == bestblocks[1], "node[0,1] have different best hashes: {} vs {}".format(bestblocks[0],
                                                                                                       bestblocks[1])

        # mine 10 more blocks to fork A
        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes[0:1])
        self.log.info("nodes[0,1] are in sync and are at fork A (103...113 blocks)")

        # fork B is at 200
        assert bestblocks[2]['height'] == 200, "unexpected tip: {}".format(bestblocks[2])
        self.log.info("node2 is at fork B (103...200 blocks)")

        # endorse block 113 (fork A tip)
        addr0 = self.nodes[0].getnewaddress()
        txid = endorse_block(self.nodes[0], self.apm, 113, addr0)
        self.log.info("node0 endorsed block 113 (fork A tip)")
        # mine pop tx on node0
        containinghash = self.nodes[0].generate(nblocks=1)
        self.sync_all(self.nodes[0:1])
        containingblock = self.nodes[0].getblock(containinghash[0])
        assert txid in containingblock['tx'], "pop tx is not in containing block"
        self.log.info("nodes[0,1] are in sync, pop tx containing block is 114 on fork A")

        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[1], 2)
        self.log.info("node2 connected to nodes[0,1]")

        self.sync_blocks(self.nodes, timeout=20)
        self.log.info("nodes[0,1,2] are in sync")

        # expected best block hash is fork A (has higher pop score)
        bestblocks = [self.get_best_block(x) for x in self.nodes]
        assert_equal(bestblocks[0], bestblocks[1])
        assert_equal(bestblocks[0], bestblocks[2])
        self.log.info("all nodes switched to common block")

        assert bestblocks[0]['height'] == 113, \
            "nodes[0,1] expected to select shorter chain (113) with higher pop score\n" \
            "but selected longer chain (200)"

        self.log.info("all nodes selected fork A as best chain")

        self.log.warning("_shorter_endorsed_chain_wins() succeeded!")

    def run_test(self):
        """Main test logic"""

        self.sync_all(self.nodes)

        from pypopminer import MockMiner
        self.apm = MockMiner()

        self._shorter_endorsed_chain_wins()


if __name__ == '__main__':
    PopFr().main()
