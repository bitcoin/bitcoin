#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests NODE_NETWORK_LIMITED.

Tests that a node configured with -prune=550 signals NODE_NETWORK_LIMITED correctly
and that it responds to getdata requests for blocks correctly:
    - send a block within 288 + 2 of the tip
    - disconnect peers who request blocks older than that."""
from test_framework.messages import CInv, msg_getdata
from test_framework.mininode import NODE_BLOOM, NODE_NETWORK_LIMITED, NODE_WITNESS, NetworkThread, P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class P2PIgnoreInv(P2PInterface):
    def on_inv(self, message):
        # The node will send us invs for other blocks. Ignore them.
        pass

    def send_getdata_for_block(self, blockhash):
        getdata_request = msg_getdata()
        getdata_request.inv.append(CInv(2, int(blockhash, 16)))
        self.send_message(getdata_request)

class NodeNetworkLimitedTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-prune=550']]

    def run_test(self):
        node = self.nodes[0].add_p2p_connection(P2PIgnoreInv())
        NetworkThread().start()
        node.wait_for_verack()

        expected_services = NODE_BLOOM | NODE_WITNESS | NODE_NETWORK_LIMITED

        self.log.info("Check that node has signalled expected services.")
        assert_equal(node.nServices, expected_services)

        self.log.info("Check that the localservices is as expected.")
        assert_equal(int(self.nodes[0].getnetworkinfo()['localservices'], 16), expected_services)

        self.log.info("Mine enough blocks to reach the NODE_NETWORK_LIMITED range.")
        blocks = self.nodes[0].generate(292)

        self.log.info("Make sure we can max retrive block at tip-288.")
        node.send_getdata_for_block(blocks[1])  # last block in valid range
        node.wait_for_block(int(blocks[1], 16), timeout=3)

        self.log.info("Requesting block at height 2 (tip-289) must fail (ignored).")
        node.send_getdata_for_block(blocks[0])  # first block outside of the 288+2 limit
        node.wait_for_disconnect(5)

if __name__ == '__main__':
    NodeNetworkLimitedTest().main()
