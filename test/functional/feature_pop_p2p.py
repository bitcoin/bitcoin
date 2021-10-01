#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2021 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.mininode import (
    P2PInterface,
)
from test_framework.pop import endorse_block, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes, assert_equal,
)


class BaseNode(P2PInterface):
    def __init__(self, log=None):
        super().__init__()
        self.log = log
        # Test variables for the offer PopData messages
        self.executed_msg_offer_atv = 0
        self.executed_msg_offer_vtb = 0
        self.executed_msg_offer_vbk = 0
        # Test variables for the send PopData messages
        self.executed_msg_atv = 0
        self.executed_msg_vtb = 0
        self.executed_msg_vbk = 0
        # Test variables for the get PopData messages
        self.executed_msg_get_atv = 0
        self.executed_msg_get_vtb = 0
        self.executed_msg_get_vbk = 0

    def on_inv(self, message):
        for inv in message.inv:
            if inv.type == 8:
                self.log.info("receive message offer ATV")
                self.executed_msg_offer_atv = self.executed_msg_offer_atv + 1
            if inv.type == 9:
                self.log.info("receive message offer VTB")
                self.executed_msg_offer_vtb = self.executed_msg_offer_vtb + 1
            if inv.type == 10:
                self.log.info("receive message offer VBK")
                self.executed_msg_offer_vbk = self.executed_msg_offer_vbk + 1
        


class PopP2P(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypoptools()

    def setup_network(self):
        self.setup_nodes()

        mine_until_pop_active(self.nodes[0])
        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)

    def _run_sync_case(self):
        self.log.info("running _run_sync_case")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)

        bn = BaseNode(self.log)

        self.nodes[0].add_p2p_connection(bn)

        time.sleep(20)

        assert_equal(bn.executed_msg_atv, 0)
        assert_equal(bn.executed_msg_offer_atv, 1)
        assert_equal(bn.executed_msg_offer_vbk, 1)

        self.log.info("_run_sync_case successful")

    def _run_sync_after_generating(self):
        self.log.info("running _run_sync_after_generating")

        bn = BaseNode(self.log)
        self.nodes[0].add_p2p_connection(bn)

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']

        endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)

        time.sleep(20)

        assert_equal(bn.executed_msg_offer_atv, 1)
        assert_equal(bn.executed_msg_offer_vbk, 2)

        self.log.info("_run_sync_after_generating successful")

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self._run_sync_case()

        self.restart_node(0)
        self._run_sync_after_generating()

# TODO: rewrite
if __name__ == '__main__':
    PopP2P().main()
