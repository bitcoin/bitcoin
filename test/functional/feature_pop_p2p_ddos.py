#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.
from test_framework.pop import endorse_block, mine_until_pop_active
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    sync_mempools,
    p2p_port, assert_equal,
)

from test_framework.mininode import (
    P2PInterface,
    msg_offer_atv,
    msg_get_atv,
    msg_atv,
)

import time


class BaseNode(P2PInterface):
    def __init__(self):
        super().__init__()


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

    def _run_case(self, msg_func, i):
        bn = BaseNode()
        self.nodes[0].add_p2p_connection(bn)
        self.log.warning("running case {}".format(i))

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        atv_id = endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)

        assert len(self.nodes[0].getpeerinfo()) == 1
        peerinfo = self.nodes[0].getpeerinfo()[0]
        assert (not 'noban' in peerinfo['permissions'])

        # spaming node1
        self.log.info("spaming node0 ...")
        disconnected = False
        for i in range(10000):
            try:
                msg = msg_func([atv_id])
                # Send message is used to send a P2P message to the node over our P2PInterface
                self.nodes[0].p2p.send_message(msg)
            except IOError:
                disconnected = True

        self.log.info("node0 banned our peer")
        assert disconnected

        assert_equal(len(self.nodes[0].getpeerinfo()), 0)

    def _run_case2(self):
        bn = BaseNode()
        self.nodes[0].add_p2p_connection(bn)
        self.log.warning("running case for pop data sending")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']
        atv_id = endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)
        raw_atv = self.nodes[0].getrawatv(atv_id)

        assert len(self.nodes[0].getpeerinfo()) == 1
        peerinfo = self.nodes[0].getpeerinfo()[0]
        assert (not 'noban' in peerinfo['permissions'])

        # spaming node1
        self.log.info("spaming node0 ...")
        disconnected = False
        for i in range(10000):
            try:
                msg = msg_atv(raw_atv)
                # Send message is used to send a P2P message to the node over our P2PInterface
                self.nodes[0].p2p.send_message(msg)
            except IOError:
                disconnected = True

        self.log.info("node0 banned our peer")
        assert disconnected

        assert_equal(len(self.nodes[0].getpeerinfo()), 0)

    def _run_case3(self):
        bn = BaseNode()
        self.nodes[0].add_p2p_connection(bn)
        self.log.warning("running case3 for pop data sending")

        # endorse block 5
        addr = self.nodes[0].getnewaddress()
        self.log.info("endorsing block 5 on node0 by miner {}".format(addr))
        tipheight = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height']
        atv_id = endorse_block(self.nodes[0], self.apm, tipheight - 5, addr)
        raw_atv = self.nodes[0].getrawatv(atv_id)

        assert len(self.nodes[0].getpeerinfo()) == 1
        peerinfo = self.nodes[0].getpeerinfo()[0]
        assert (not 'noban' in peerinfo['permissions'])
        assert peerinfo['banscore'] == 0

        # spaming node1
        self.log.info("send to node0 that it has not requested...")
        msg = msg_atv(raw_atv)
        # Send message is used to send a P2P message to the node over our P2PInterface
        self.nodes[0].p2p.send_message(msg)

        time.sleep(1)

        peerinfo = self.nodes[0].getpeerinfo()[0]
        assert peerinfo['banscore'] == 20

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypoptools.pypopminer import MockMiner
        self.apm = MockMiner()

        self.cases = [msg_offer_atv, msg_get_atv]

        for i, case in enumerate(self.cases):
            self._run_case(case, i)
            self.restart_node(0)

        self._run_case2()

        self.restart_node(0)
        self._run_case3()


if __name__ == '__main__':
    PopP2P().main()
