#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import *

class BaseNode(P2PInterface):
    nServices = 0
    firstAddrnServices = 0
    def on_version(self, message):
        self.nServices = message.nServices

class NodeNetworkLimitedTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-prune=550']]

    def getSignaledServiceFlags(self):
        node = self.nodes[0].add_p2p_connection(BaseNode())
        NetworkThread().start()
        node.wait_for_verack()
        services = node.nServices
        self.nodes[0].disconnect_p2ps()
        node.wait_for_disconnect()
        return services

    def tryGetBlockViaGetData(self, blockhash, must_disconnect):
        node = self.nodes[0].add_p2p_connection(BaseNode())
        NetworkThread().start()
        node.wait_for_verack()
        node.send_message(msg_verack())
        getdata_request = msg_getdata()
        getdata_request.inv.append(CInv(2, int(blockhash, 16)))
        node.send_message(getdata_request)

        if (must_disconnect):
            #ensure we get disconnected
            node.wait_for_disconnect(5)
        else:
            # check if the peer sends us the requested block
            node.wait_for_block(int(blockhash, 16), 3)
            self.nodes[0].disconnect_p2ps()
            node.wait_for_disconnect()

    def run_test(self):
        #NODE_BLOOM & NODE_WITNESS & NODE_NETWORK_LIMITED must now be signaled
        assert_equal(self.getSignaledServiceFlags(), 1036) #1036 == 0x40C == 0100 0000 1100
#                                                                              |        ||
#                                                                              |        |^--- NODE_BLOOM
#                                                                              |        ^---- NODE_WITNESS
#                                                                              ^-- NODE_NETWORK_LIMITED

        #now mine some blocks over the NODE_NETWORK_LIMITED + 2(racy buffer ext.) target
        firstblock = self.nodes[0].generate(1)[0]
        blocks = self.nodes[0].generate(292)
        blockWithinLimitedRange = blocks[-1]

        #make sure we can max retrive block at tip-288
        #requesting block at height 2 (tip-289) must fail (ignored)
        self.tryGetBlockViaGetData(firstblock, True) #first block must lead to disconnect
        self.tryGetBlockViaGetData(blocks[1], False) #last block in valid range
        self.tryGetBlockViaGetData(blocks[0], True) #first block outside of the 288+2 limit

        #NODE_NETWORK_LIMITED must still be signaled after restart
        self.restart_node(0)
        assert_equal(self.getSignaledServiceFlags(), 1036)

        #test the RPC service flags
        assert_equal(self.nodes[0].getnetworkinfo()['localservices'], "000000000000040c")

        # getdata a block above the NODE_NETWORK_LIMITED threshold must be possible
        self.tryGetBlockViaGetData(blockWithinLimitedRange, False)

        # getdata a block below the NODE_NETWORK_LIMITED threshold must be ignored
        self.tryGetBlockViaGetData(firstblock, True)

if __name__ == '__main__':
    NodeNetworkLimitedTest().main()
