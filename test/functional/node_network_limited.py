#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.messages import CInv, msg_getdata
from test_framework.mininode import NODE_BLOOM, NODE_NETWORK_LIMITED, NODE_WITNESS, NetworkThread, P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class P2PIgnoreInv(P2PInterface):
    def on_inv(self, message):
        # The node will send us invs for other blocks. Ignore them.
        pass

class NodeNetworkLimitedTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-prune=550']]

    def get_signalled_service_flags(self):
        node = self.nodes[0].add_p2p_connection(P2PIgnoreInv())
        NetworkThread().start()
        node.wait_for_verack()
        services = node.nServices
        self.nodes[0].disconnect_p2ps()
        node.wait_for_disconnect()
        return services

    def try_get_block_via_getdata(self, blockhash, must_disconnect):
        node = self.nodes[0].add_p2p_connection(P2PIgnoreInv())
        NetworkThread().start()
        node.wait_for_verack()
        getdata_request = msg_getdata()
        getdata_request.inv.append(CInv(2, int(blockhash, 16)))
        node.send_message(getdata_request)

        if (must_disconnect):
            # Ensure we get disconnected
            node.wait_for_disconnect(5)
        else:
            # check if the peer sends us the requested block
            node.wait_for_block(int(blockhash, 16), 3)
            self.nodes[0].disconnect_p2ps()
            node.wait_for_disconnect()

    def run_test(self):
        # NODE_BLOOM & NODE_WITNESS & NODE_NETWORK_LIMITED must now be signaled
        assert_equal(self.get_signalled_service_flags(), NODE_BLOOM | NODE_WITNESS | NODE_NETWORK_LIMITED)

        # Test the RPC service flags
        assert_equal(int(self.nodes[0].getnetworkinfo()['localservices'], 16), NODE_BLOOM | NODE_WITNESS | NODE_NETWORK_LIMITED)

        # Now mine some blocks over the NODE_NETWORK_LIMITED + 2(racy buffer ext.) target
        blocks = self.nodes[0].generate(292)

        # Make sure we can max retrive block at tip-288
        # requesting block at height 2 (tip-289) must fail (ignored)
        self.try_get_block_via_getdata(blocks[1], False)  # last block in valid range
        self.try_get_block_via_getdata(blocks[0], True)  # first block outside of the 288+2 limit

if __name__ == '__main__':
    NodeNetworkLimitedTest().main()
