#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Test with multiple nodes, and multiple PoP endorsements, checking to make sure nodes stay in sync.
"""

from test_framework.pop import endorse_block, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
)


class PoPMempoolSync(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()

        mine_until_pop_active(self.nodes[0])

        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)
            self.sync_all()

    def run_test(self):
        """Main test logic"""

        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        addr0 = self.nodes[0].getnewaddress()
        self.log.info("node0 endorses block 5")
        self.nodes[0].generate(nblocks=10)

        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']
        atvid = endorse_block(self.nodes[0], self.apm, tipheight - 5, addr0)

        self.sync_pop_mempools(self.nodes, timeout=20)
        self.log.info("nodes[0,1] have syncd pop mempools")

        rawpopmempool1 = self.nodes[1].getrawpopmempool()
        assert atvid in rawpopmempool1['atvs']
        self.log.info("node1 contains atv1 in its pop mempool")

        self.restart_node(1)
        self.log.info("node1 has been restarted")
        rawpopmempool1 = self.nodes[1].getrawpopmempool()
        assert atvid not in rawpopmempool1['atvs']
        self.log.info("node1 does not contain atv1 in its pop mempool after restart")

        connect_nodes(self.nodes[0], 1)
        self.log.info("node1 connect to node0")

        self.sync_pop_mempools(self.nodes, timeout=20)
        self.log.info("nodes[0,1] have syncd pop mempools")

        rawpopmempool1 = self.nodes[1].getrawpopmempool()
        assert atvid in rawpopmempool1['atvs']


if __name__ == '__main__':
    PoPMempoolSync().main()
