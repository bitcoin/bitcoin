#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various net timeouts.

- Create three dashd nodes:

    no_verack_node - we never send a verack in response to their version
    no_version_node - we never send a version (only a ping)
    no_send_node - we never send any P2P message.

- Start all three nodes
- Wait 1 second
- Assert that we're connected
- Send a ping to no_verack_node and no_version_node
- Wait 1 second
- Assert that we're still connected
- Send a ping to no_verack_node and no_version_node
- Wait 2 seconds
- Assert that we're no longer connected (timeout to receive version/verack is 3 seconds)
"""

from time import sleep

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class TestP2PConn(P2PInterface):
    def on_version(self, message):
        # Don't send a verack in response
        pass

class TimeoutsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        # set timeout to receive version/verack to 3 seconds
        self.extra_args = [["-peertimeout=3"]]

    def run_test(self):
        # Setup the p2p connections
        no_verack_node = self.nodes[0].add_p2p_connection(TestP2PConn())
        no_version_node = self.nodes[0].add_p2p_connection(TestP2PConn(), send_version=False, wait_for_verack=False)
        no_send_node = self.nodes[0].add_p2p_connection(TestP2PConn(), send_version=False, wait_for_verack=False)

        sleep(1)

        assert no_verack_node.is_connected
        assert no_version_node.is_connected
        assert no_send_node.is_connected

        no_verack_node.send_message(msg_ping())
        no_version_node.send_message(msg_ping())

        sleep(1)

        assert "version" in no_verack_node.last_message

        assert no_verack_node.is_connected
        assert no_version_node.is_connected
        assert no_send_node.is_connected

        no_verack_node.send_message(msg_ping())
        no_version_node.send_message(msg_ping())

        expected_timeout_logs = [
            "version handshake timeout from 0",
            "socket no message in first 3 seconds, 1 0 from 1",
            "socket no message in first 3 seconds, 0 0 from 2",
        ]

        with self.nodes[0].assert_debug_log(expected_msgs=expected_timeout_logs):
            sleep(3 + 1) # Sleep one second more than peertimeout
            assert not no_verack_node.is_connected
            assert not no_version_node.is_connected
            assert not no_send_node.is_connected

if __name__ == '__main__':
    TimeoutsTest().main()
