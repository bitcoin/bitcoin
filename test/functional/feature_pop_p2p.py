#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.mininode import (
    P2PInterface,
    msg_get_atv,
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

    def on_ofATV(self, message):
        self.log.info("receive message offer ATV")
        self.executed_msg_offer_atv = self.executed_msg_offer_atv + 1

    def on_ofVTB(self, message):
        self.log.info("receive message offer VTB")
        self.executed_msg_offer_vtb = self.executed_msg_offer_vtb + 1

    def on_ofVBK(self, message):
        self.log.info("receive message offer VBK")
        self.executed_msg_offer_vbk = self.executed_msg_offer_vbk + 1

    def on_ATV(self, message):
        self.log.info("receive message ATV")
        self.executed_msg_atv = self.executed_msg_atv + 1

    def on_VTB(self, message):
        self.log.info("receive message VTB")
        self.executed_msg_vtb = self.executed_msg_vtb + 1

    def on_VBK(self, message):
        self.log.info("receive message VBK")
        self.executed_msg_vbk = self.executed_msg_vbk + 1

    def on_gATV(self, message):
        self.log.info("receive message get ATV")
        self.executed_msg_get_atv = self.executed_msg_get_atv + 1

    def on_gVTB(self, message):
        self.log.info("receive message get VTB")
        self.executed_msg_get_vtb = self.executed_msg_get_vtb + 1

    def on_gVBK(self, message):
        self.log.info("receive message get VBK")
        self.executed_msg_get_vbk = self.executed_msg_get_vbk + 1


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
        atv_id = endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)

        bn = BaseNode(self.log)

        self.nodes[0].add_p2p_connection(bn)
        time.sleep(2)

        assert bn.executed_msg_atv == 0
        assert bn.executed_msg_offer_atv == 1
        assert bn.executed_msg_offer_vbk == 1

        msg = msg_get_atv([atv_id])
        self.nodes[0].p2p.send_message(msg)
        self.nodes[0].p2p.send_message(msg)
        self.nodes[0].p2p.send_message(msg)

        time.sleep(2)

        assert bn.executed_msg_atv == 3

        self.log.info("_run_sync_case successful")

    def _run_sync_after_generating(self):
        self.log.info("running _run_sync_after_generating")

        bn = BaseNode(self.log)
        self.nodes[0].add_p2p_connection(bn)

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']

        atv_id = endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)

        msg = msg_get_atv([atv_id])
        self.nodes[0].p2p.send_message(msg)

        time.sleep(5)

        assert_equal(bn.executed_msg_atv, 1)
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


if __name__ == '__main__':
    PopP2P().main()
