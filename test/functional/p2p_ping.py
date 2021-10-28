#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test ping message
"""

import time

from test_framework.messages import msg_pong
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

PING_INTERVAL = 2 * 60


class msg_pong_corrupt(msg_pong):
    def serialize(self):
        return b""


class NodePongAdd1(P2PInterface):
    def on_ping(self, message):
        self.send_message(msg_pong(message.nonce + 1))


class NodeNoPong(P2PInterface):
    def on_ping(self, message):
        pass


TIMEOUT_INTERVAL = 20 * 60


class PingPongTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        # Set the peer connection timeout low. It does not matter for this
        # test, as long as it is less than TIMEOUT_INTERVAL.
        self.extra_args = [['-peertimeout=1']]

    def check_peer_info(self, *, pingtime, minping, pingwait):
        stats = self.nodes[0].getpeerinfo()[0]
        assert_equal(stats.pop('pingtime', None), pingtime)
        assert_equal(stats.pop('minping', None), minping)
        assert_equal(stats.pop('pingwait', None), pingwait)

    def mock_forward(self, delta):
        self.mock_time += delta
        self.nodes[0].setmocktime(self.mock_time)

    def run_test(self):
        self.mock_time = int(time.time())
        self.mock_forward(0)

        self.log.info('Check that ping is sent after connection is established')
        no_pong_node = self.nodes[0].add_p2p_connection(NodeNoPong())
        self.mock_forward(3)
        assert no_pong_node.last_message.pop('ping').nonce != 0
        self.check_peer_info(pingtime=None, minping=None, pingwait=3)

        self.log.info('Reply without nonce cancels ping')
        with self.nodes[0].assert_debug_log(['pong peer=0: Short payload']):
            no_pong_node.send_and_ping(msg_pong_corrupt())
        self.check_peer_info(pingtime=None, minping=None, pingwait=None)

        self.log.info('Reply without ping')
        with self.nodes[0].assert_debug_log([
                'pong peer=0: Unsolicited pong without ping, 0 expected, 0 received, 8 bytes',
        ]):
            no_pong_node.send_and_ping(msg_pong())
        self.check_peer_info(pingtime=None, minping=None, pingwait=None)

        self.log.info('Reply with wrong nonce does not cancel ping')
        assert 'ping' not in no_pong_node.last_message
        with self.nodes[0].assert_debug_log(['pong peer=0: Nonce mismatch']):
            # mock time PING_INTERVAL ahead to trigger node into sending a ping
            self.mock_forward(PING_INTERVAL + 1)
            no_pong_node.wait_until(lambda: 'ping' in no_pong_node.last_message)
            self.mock_forward(9)
            # Send the wrong pong
            no_pong_node.send_and_ping(msg_pong(no_pong_node.last_message.pop('ping').nonce - 1))
        self.check_peer_info(pingtime=None, minping=None, pingwait=9)

        self.log.info('Reply with zero nonce does cancel ping')
        with self.nodes[0].assert_debug_log(['pong peer=0: Nonce zero']):
            no_pong_node.send_and_ping(msg_pong(0))
        self.check_peer_info(pingtime=None, minping=None, pingwait=None)

        self.log.info('Check that ping is properly reported on RPC')
        assert 'ping' not in no_pong_node.last_message
        # mock time PING_INTERVAL ahead to trigger node into sending a ping
        self.mock_forward(PING_INTERVAL + 1)
        no_pong_node.wait_until(lambda: 'ping' in no_pong_node.last_message)
        ping_delay = 29
        self.mock_forward(ping_delay)
        no_pong_node.wait_until(lambda: 'ping' in no_pong_node.last_message)
        no_pong_node.send_and_ping(msg_pong(no_pong_node.last_message.pop('ping').nonce))
        self.check_peer_info(pingtime=ping_delay, minping=ping_delay, pingwait=None)

        self.log.info('Check that minping is decreased after a fast roundtrip')
        # mock time PING_INTERVAL ahead to trigger node into sending a ping
        self.mock_forward(PING_INTERVAL + 1)
        no_pong_node.wait_until(lambda: 'ping' in no_pong_node.last_message)
        ping_delay = 9
        self.mock_forward(ping_delay)
        no_pong_node.wait_until(lambda: 'ping' in no_pong_node.last_message)
        no_pong_node.send_and_ping(msg_pong(no_pong_node.last_message.pop('ping').nonce))
        self.check_peer_info(pingtime=ping_delay, minping=ping_delay, pingwait=None)

        self.log.info('Check that peer is disconnected after ping timeout')
        assert 'ping' not in no_pong_node.last_message
        self.nodes[0].ping()
        no_pong_node.wait_until(lambda: 'ping' in no_pong_node.last_message)
        with self.nodes[0].assert_debug_log(['ping timeout: 1201.000000s']):
            self.mock_forward(TIMEOUT_INTERVAL // 2)
            # Check that sending a ping does not prevent the disconnect
            no_pong_node.sync_with_ping()
            self.mock_forward(TIMEOUT_INTERVAL // 2 + 1)
            no_pong_node.wait_for_disconnect()


if __name__ == '__main__':
    PingPongTest().main()
