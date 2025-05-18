#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid locators.
"""

from test_framework.messages import msg_getheaders, msg_getblocks, MAX_LOCATOR_SZ
from test_framework.p2p import P2PInterface
from test_framework.test_framework import TortoisecoinTestFramework


class InvalidLocatorTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]  # convenience reference to the node
        self.generatetoaddress(node, 1, node.get_deterministic_priv_key().address)  # Get node out of IBD

        self.log.info('Test max locator size')
        block_count = node.getblockcount()
        for msg in [msg_getheaders(), msg_getblocks()]:
            self.log.info('Wait for disconnect when sending {} hashes in locator'.format(MAX_LOCATOR_SZ + 1))
            exceed_max_peer = node.add_p2p_connection(P2PInterface())
            msg.locator.vHave = [int(node.getblockhash(i - 1), 16) for i in range(block_count, block_count - (MAX_LOCATOR_SZ + 1), -1)]
            exceed_max_peer.send_message(msg)
            exceed_max_peer.wait_for_disconnect()
            node.disconnect_p2ps()

            self.log.info('Wait for response when sending {} hashes in locator'.format(MAX_LOCATOR_SZ))
            within_max_peer = node.add_p2p_connection(P2PInterface())
            msg.locator.vHave = [int(node.getblockhash(i - 1), 16) for i in range(block_count, block_count - (MAX_LOCATOR_SZ), -1)]
            within_max_peer.send_message(msg)
            if type(msg) is msg_getheaders:
                within_max_peer.wait_for_header(node.getbestblockhash())
            else:
                within_max_peer.wait_for_block(int(node.getbestblockhash(), 16))


if __name__ == '__main__':
    InvalidLocatorTest(__file__).main()
