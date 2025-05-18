#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test invalid p2p messages for nodes with bloom filters disabled.

Test that, when bloom filters are not enabled, peers are disconnected if:
1. They send a p2p mempool message
2. They send a p2p filterload message
3. They send a p2p filteradd message
4. They send a p2p filterclear message
"""

from test_framework.messages import msg_mempool, msg_filteradd, msg_filterload, msg_filterclear
from test_framework.p2p import P2PInterface
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import assert_equal


class P2PNoBloomFilterMessages(TortoisecoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-peerbloomfilters=0"]]

    def test_message_causes_disconnect(self, message):
        """Add a p2p connection that sends a message and check that it disconnects."""
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.send_message(message)
        peer.wait_for_disconnect()
        assert_equal(self.nodes[0].getconnectioncount(), 0)

    def run_test(self):
        self.log.info("Test that peer is disconnected if it sends mempool message")
        self.test_message_causes_disconnect(msg_mempool())

        self.log.info("Test that peer is disconnected if it sends filterload message")
        self.test_message_causes_disconnect(msg_filterload())

        self.log.info("Test that peer is disconnected if it sends filteradd message")
        self.test_message_causes_disconnect(msg_filteradd(data=b'\xcc'))

        self.log.info("Test that peer is disconnected if it sends a filterclear message")
        self.test_message_causes_disconnect(msg_filterclear())


if __name__ == '__main__':
    P2PNoBloomFilterMessages(__file__).main()
