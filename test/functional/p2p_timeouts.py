#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various net timeouts.

- Create three peers:

    no_verack_node - we never send a verack in response to their version
    no_version_node - we never send a version (only a ping)
    no_send_node - we never send any P2P message.

- Wait 1 second
- Assert that we're connected
- Send a ping to no_verack_node and no_version_node
- Wait 1 second
- Assert that we're still connected
- Send a ping to no_verack_node and no_version_node
- Wait 2 seconds
- Assert that we're no longer connected (timeout to receive version/verack is 3 seconds)
"""

from test_framework.messages import msg_ping
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework

import time


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

    def mock_forward(self, delta):
        self.mock_time += delta
        self.nodes[0].setmocktime(self.mock_time)

    def run_test(self):
        self.mock_time = int(time.time())
        self.mock_forward(0)

        # Setup the p2p connections, making sure the connections are established before the mocktime is bumped
        with self.nodes[0].assert_debug_log(['Added connection peer=0']):
            no_verack_node = self.nodes[0].add_p2p_connection(TestP2PConn(), wait_for_verack=False)
        with self.nodes[0].assert_debug_log(['Added connection peer=1']):
            no_version_node = self.nodes[0].add_p2p_connection(TestP2PConn(), send_version=False, wait_for_verack=False)
        with self.nodes[0].assert_debug_log(['Added connection peer=2']):
            no_send_node = self.nodes[0].add_p2p_connection(TestP2PConn(), send_version=False, wait_for_verack=False)

        # Wait until we got the verack in response to the version. Though, don't wait for the other node to receive the
        # verack, since we never sent one
        no_verack_node.wait_for_verack()

        self.mock_forward(1)

        assert no_verack_node.is_connected
        assert no_version_node.is_connected
        assert no_send_node.is_connected

        with self.nodes[0].assert_debug_log(['Unsupported message "ping" prior to verack from peer=0']):
            no_verack_node.send_message(msg_ping())

        with self.nodes[0].assert_debug_log(['non-version message before version handshake. Message "ping" from peer=1']):
            no_version_node.send_message(msg_ping())

        self.mock_forward(1)
        assert "version" in no_verack_node.last_message

        assert no_verack_node.is_connected
        assert no_version_node.is_connected
        assert no_send_node.is_connected

        no_verack_node.send_message(msg_ping())
        no_version_node.send_message(msg_ping())

        if self.options.v2transport:
            expected_timeout_logs = [
                "version handshake timeout, disconnecting peer=0",
                "version handshake timeout, disconnecting peer=1",
                "version handshake timeout, disconnecting peer=2",
            ]
        else:
            expected_timeout_logs = [
                "version handshake timeout, disconnecting peer=0",
                "socket no message in first 3 seconds, 1 0 disconnecting peer=1",
                "socket no message in first 3 seconds, 0 0 disconnecting peer=2",
            ]

        with self.nodes[0].assert_debug_log(expected_msgs=expected_timeout_logs):
            self.mock_forward(2)
            no_verack_node.wait_for_disconnect(timeout=1)
            no_version_node.wait_for_disconnect(timeout=1)
            no_send_node.wait_for_disconnect(timeout=1)

        self.stop_nodes(0)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg='Error: peertimeout must be a positive integer.',
            extra_args=['-peertimeout=0'],
        )


if __name__ == '__main__':
    TimeoutsTest().main()
