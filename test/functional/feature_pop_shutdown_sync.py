#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""

"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.pop import mine_until_pop_active
from test_framework.util import assert_raises_rpc_error, connect_nodes, disconnect_nodes
import time

class PopShutdownSync(BitcoinTestFramework):
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

        # all nodes connected and synced
        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)
        self.sync_all()

    def get_best_block(self, node):
        hash = node.getbestblockhash()
        return node.getblock(hash)

    def run_test(self):
        self.sync_all(self.nodes[0:2])
        lastblock = self.nodes[0].getblockcount()
        self.log.info("nodes synced with block height %d", lastblock)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        disconnect_nodes(self.nodes[0], 1)
        lastblock = self.nodes[1].getblockcount()
        self.nodes[1].generate(nblocks=1000)
        self.log.info("node1 disconnected and generating more blocks")
        self.nodes[1].waitforblockheight(lastblock + 1000)
        lastblock = self.nodes[1].getblockcount()
        self.log.info("node1 reached block height %d", lastblock)
        connect_nodes(self.nodes[0], 1)
        self.log.info("node1 reconnected")
        time.sleep(1) # Sleep for 1 second to let headers sync

        lastblock = self.get_best_block(self.nodes[0])
        self.stop_node(0)
        self.log.info("node0 stopped with block {}".format(lastblock))
        self.start_node(0)
        connect_nodes(self.nodes[0], 1)
        self.log.info("node0 restarted")
        self.sync_all(self.nodes[0:2])


if __name__ == '__main__':
    PopShutdownSync().main()
