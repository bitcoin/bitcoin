#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various net timeouts.

- Create three bitcoind nodes:

    no_verack_node - we never send a verack in response to their version
    no_version_node - we never send a version (only a ping)
    no_send_node - we never send any P2P message.

- Start all three nodes
- Wait 1 second
- Assert that we're connected
- Send a ping to no_verack_node and no_version_node
- Wait 30 seconds
- Assert that we're still connected
- Send a ping to no_verack_node and no_version_node
- Wait 31 seconds
- Assert that we're no longer connected (timeout to receive version/verack is 60 seconds)
"""

from time import sleep

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class TestNode(P2PInterface):
    def on_version(self, message):
        # Don't send a verack in response
        pass

class TimeoutsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        no_verack_node = self.nodes[0].add_p2p_connection(TestNode())
        no_version_node = self.nodes[0].add_p2p_connection(TestNode(), send_version=False)
        no_send_node = self.nodes[0].add_p2p_connection(TestNode(), send_version=False)

        network_thread_start()

        sleep(1)

        assert no_verack_node.connected
        assert no_version_node.connected
        assert no_send_node.connected

        no_verack_node.send_message(msg_ping())
        no_version_node.send_message(msg_ping())

        sleep(30)

        assert "version" in no_verack_node.last_message

        assert no_verack_node.connected
        assert no_version_node.connected
        assert no_send_node.connected

        no_verack_node.send_message(msg_ping())
        no_version_node.send_message(msg_ping())

        sleep(31)

        assert not no_verack_node.connected
        assert not no_version_node.connected
        assert not no_send_node.connected

if __name__ == '__main__':
    TimeoutsTest().main()
