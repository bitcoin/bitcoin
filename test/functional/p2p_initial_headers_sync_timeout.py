#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test peer timeout during initial headers sync.
Tests normal peer disconnection vs noban peer behavior."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.p2p import (
    p2p_lock,
    P2PInterface,
)
from test_framework.util import assert_equal
import time

HEADERS_SYNC_BASE_TIMEOUT_SEC = 900  # 15 minutes
HEADERS_PER_HEADER_TIMEOUT_US = 1000  # 1 millisecond
POW_TARGET_SPACING_SEC = 600  # 10 minutes


class HeadersSyncTimeoutTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.peer1 = None
        self.peer2 = None

    def run_test(self):
        self.node = self.nodes[0]

        self.test_normal_peer_timeout()
        self.test_noban_peer_timeout()

    def test_normal_peer_timeout(self):
        self.log.info("Test peer disconnection on header timeout")
        self.node.setmocktime(int(time.time()))
        self.setup_peers()

        with self.node.assert_debug_log(
                ["Timeout downloading headers, disconnecting peer=0"]):
            self.trigger_timeout()

        self.log.info("Check that stalling peer1 is disconnected")
        self.peer1.wait_for_disconnect()
        assert_equal(self.node.num_test_p2p_connections(), 1)

        self.log.info("Check that peer2 receives a getheader request")
        self.peer2.wait_for_getheaders()

    def test_noban_peer_timeout(self):
        self.log.info("Test noban peer on header timeout")
        self.restart_node(0, extra_args=['-whitelist=noban@127.0.0.1'])
        self.node.setmocktime(int(time.time()))
        self.setup_peers()

        with self.node.assert_debug_log(
                ["Timeout downloading headers from noban peer, "
                 "not disconnecting peer=0"]):
            self.trigger_timeout()

        self.log.info("Check that noban peer1 is not disconnected")
        self.peer1.sync_with_ping()
        assert_equal(self.node.num_test_p2p_connections(), 2)

        self.log.info("Check that exactly 1 of {peer1, peer2} receives a "
                      "getheaders")
        count = 0
        for p in [self.peer1, self.peer2]:
            with p2p_lock:
                if "getheaders" in p.last_message:
                    count += 1
        assert_equal(count, 1)

    def setup_peers(self):
        self.log.info("Add peer1 and check it receives an initial "
                      "getheaders request")
        with self.node.assert_debug_log(
                expected_msgs=["initial getheaders (0) to peer=0"]):
            self.peer1 = self.node.add_p2p_connection(P2PInterface())
        self.peer1.wait_for_getheaders()

        self.log.info("Add outbound peer2")
        with self.node.wait_for_new_peer():
            # This peer has to be outbound otherwise the stalling peer is
            # protected from disconnection
            self.peer2 = self.node.add_outbound_p2p_connection(
                P2PInterface(), p2p_idx=1,
                connection_type="outbound-full-relay")
        assert_equal(self.node.num_test_p2p_connections(), 2)

    def trigger_timeout(self):
        self.log.info("Trigger the headers download timeout by advancing "
                      "mock time")
        best_header_time = self.node.getblockchaininfo()['time']
        current_time = self.node.mocktime
        peer1_timeout = calculate_headers_timeout(best_header_time,
                                                  current_time)
        self.node.setmocktime(peer1_timeout + 1)


def calculate_headers_timeout(best_header_time, current_time):
    time_since_best = current_time - best_header_time
    variable_timeout = ((HEADERS_PER_HEADER_TIMEOUT_US * time_since_best) /
                        POW_TARGET_SPACING_SEC / 1_000_000)
    total_timeout = (current_time + HEADERS_SYNC_BASE_TIMEOUT_SEC +
                     variable_timeout)
    return int(total_timeout)


if __name__ == '__main__':
    HeadersSyncTimeoutTest(__file__).main()
