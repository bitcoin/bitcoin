#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.pop import endorse_block, sync_pop_mempools, create_endorsed_chain, \
    assert_pop_state_equal, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    sync_mempools,
)

'''
Start 2 nodes. 
Node0 creates a chain of 100 blocks where every block is endorsed once.
Wait until both nodes sync.

Restart Node0 without -reindex, and with -checkblocks=[0..100), and -checklevel=[0..4] and compare state of Nodes0,1.
POP state must be equal.
'''


class PoPVerifyDB(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ["-reindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()
        mine_until_pop_active(self.nodes[0])

        for i in range(self.num_nodes - 1):
            connect_nodes(self.nodes[i + 1], i)
        self.sync_all()

    # actual = node which restarted
    # expected = node which not restarted
    def verify_state(self, actual, expected):
        self.sync_all([actual, expected], timeout=40)

    def run_test(self):
        """Main test logic"""

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()
        self.addrs = [x.getnewaddress() for x in self.nodes]
        self.endorsed_length = 100

        assert_pop_state_equal(self.nodes)
        create_endorsed_chain(self.nodes[0], self.apm, self.endorsed_length, self.addrs[0])
        self.sync_all(self.nodes, timeout=20)
        assert_pop_state_equal(self.nodes)

        checkblocks = 0  # all blocks
        # 0, 1, 2, 3, 4
        for checklevel in range(5):
            self.log.info("checkblocks={} checklevel={}".format(checkblocks, checklevel))
            self.restart_node(0, extra_args=[
                "-checkblocks={}".format(checkblocks),
                "-checklevel={}".format(checklevel)
            ])
            time.sleep(10)
            self.sync_all(self.nodes, timeout=60)
            assert_pop_state_equal(self.nodes)
            self.log.info("success")


if __name__ == '__main__':
    PoPVerifyDB().main()
