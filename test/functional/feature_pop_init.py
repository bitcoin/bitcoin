#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Start 3 nodes. Node0 has no -txindex, Nodes[1,2] have -txindex
Create a chain of 20 blocks, where every next block contains 1 pop tx that endorses previous block.
Restart nodes[0,1] without -reindex.
Node[2] is a control node.

Expect that BTC/VBK tree state on nodes[0,1] is same as before shutdown (test against control node).
"""

from test_framework.pop import endorse_block, create_endorsed_chain, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    disconnect_nodes, assert_equal,
)


class PopInit(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [[], ["-txindex"], ["-reindex"]]
        self.extra_args = [x + ["-checklevel=4"] for x in self.extra_args]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])
        # nodes[0,1] will be restarted
        # node[2] is a control node

        # all nodes connected and synced
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)

        self.sync_all(self.nodes, timeout=60)

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def _restart_init_test(self):
        self.SIZE = 20
        self.addr0 = self.nodes[0].getnewaddress()
        # 100 blocks without endorsements
        self.nodes[0].generate(nblocks=100)
        self.log.info("node0 started mining of {} endorsed blocks".format(self.SIZE))
        create_endorsed_chain(self.nodes[0], self.apm, self.SIZE, self.addr0)
        self.log.info("node0 finished creation of {} endorsed blocks".format(self.SIZE))

        self.sync_blocks(self.nodes)
        self.log.info("nodes are in sync")

        # stop node0
        self.restart_node(0)
        self.restart_node(1)
        self.log.info("nodes[0,1] restarted")
        self.sync_all(self.nodes, timeout=30)
        self.log.info("nodes are in sync")

        bestblocks = [self.get_best_block(x) for x in self.nodes]
        popdata = [x.getpopdatabyheight(bestblocks[0]['height']) for x in self.nodes]

        # when node0 stops, its VBK/BTC trees get cleared. When we start it again, it MUST load payloads into trees.
        # if this assert fails, it means that node restarted, but NOT loaded its VBK/BTC state into memory.
        # node[2] is a control node that has never been shut down.
        assert_equal(popdata[0], popdata[2])
        assert_equal(popdata[1], popdata[2])

        self.log.warning("success! _restart_init_test()")

    def run_test(self):
        """Main test logic"""

        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._restart_init_test()

if __name__ == '__main__':
    PopInit().main()