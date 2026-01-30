#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test initial headers download and timeout behavior

Test that we only try to initially sync headers from one peer (until our chain
is close to caught up), and that each block announcement results in only one
additional peer receiving a getheaders message.

Also test peer timeout during initial headers sync, including normal peer
disconnection vs noban peer behavior.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    CInv,
    MSG_BLOCK,
    msg_headers,
    msg_inv,
)
from test_framework.p2p import (
    p2p_lock,
    P2PInterface,
)
from test_framework.util import (
    assert_equal,
)
import math
import random
import time

# Constants from net_processing
HEADERS_DOWNLOAD_TIMEOUT_BASE_SEC = 15 * 60
HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER_MS = 1
POW_TARGET_SPACING_SEC = 10 * 60


def calculate_headers_timeout(best_header_time, current_time):
    seconds_since_best_header = current_time - best_header_time
    # Using ceil ensures the calculated timeout is >= actual timeout and not lower
    # because of precision loss from conversion to seconds
    variable_timeout_sec = math.ceil(HEADERS_DOWNLOAD_TIMEOUT_PER_HEADER_MS / 1_000 *
                                     seconds_since_best_header / POW_TARGET_SPACING_SEC)
    return int(current_time + HEADERS_DOWNLOAD_TIMEOUT_BASE_SEC + variable_timeout_sec)


class HeadersSyncTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def announce_random_block(self, peers):
        new_block_announcement = msg_inv(inv=[CInv(MSG_BLOCK, random.randrange(1<<256))])
        for p in peers:
            p.send_and_ping(new_block_announcement)

    def assert_single_getheaders_recipient(self, peers):
        count = 0
        receiving_peer = None
        for p in peers:
            with p2p_lock:
                if "getheaders" in p.last_message:
                    count += 1
                    receiving_peer = p
        assert_equal(count, 1)
        return receiving_peer

    def test_initial_headers_sync(self):
        self.log.info("Test initial headers sync")

        self.log.info("Adding a peer to node0")
        peer1 = self.nodes[0].add_p2p_connection(P2PInterface())
        best_block_hash = int(self.nodes[0].getbestblockhash(), 16)

        # Wait for peer1 to receive a getheaders
        peer1.wait_for_getheaders(block_hash=best_block_hash)
        # An empty reply will clear the outstanding getheaders request,
        # allowing additional getheaders requests to be sent to this peer in
        # the future.
        peer1.send_without_ping(msg_headers())

        self.log.info("Connecting two more peers to node0")
        # Connect 2 more peers; they should not receive a getheaders yet
        peer2 = self.nodes[0].add_p2p_connection(P2PInterface())
        peer3 = self.nodes[0].add_p2p_connection(P2PInterface())

        all_peers = [peer1, peer2, peer3]

        self.log.info("Verify that peer2 and peer3 don't receive a getheaders after connecting")
        for p in all_peers:
            p.sync_with_ping()
        with p2p_lock:
            assert "getheaders" not in peer2.last_message
            assert "getheaders" not in peer3.last_message

        self.log.info("Have all peers announce a new block")
        self.announce_random_block(all_peers)

        self.log.info("Check that peer1 receives a getheaders in response")
        peer1.wait_for_getheaders(block_hash=best_block_hash)
        peer1.send_without_ping(msg_headers()) # Send empty response, see above

        self.log.info("Check that exactly 1 of {peer2, peer3} received a getheaders in response")
        peer_receiving_getheaders = self.assert_single_getheaders_recipient([peer2, peer3])
        peer_receiving_getheaders.send_without_ping(msg_headers()) # Send empty response, see above

        self.log.info("Announce another new block, from all peers")
        self.announce_random_block(all_peers)

        self.log.info("Check that peer1 receives a getheaders in response")
        peer1.wait_for_getheaders(block_hash=best_block_hash)

        self.log.info("Check that the remaining peer received a getheaders as well")
        expected_peer = peer2
        if peer2 == peer_receiving_getheaders:
            expected_peer = peer3

        expected_peer.wait_for_getheaders(block_hash=best_block_hash)

    def setup_timeout_test_peers(self):
        self.log.info("Add peer1 and check it receives an initial getheaders request")
        node = self.nodes[0]
        with node.assert_debug_log(expected_msgs=["initial getheaders (0) to peer=0"]):
            peer1 = node.add_p2p_connection(P2PInterface())
            peer1.wait_for_getheaders(block_hash=int(node.getbestblockhash(), 16))

        self.log.info("Add outbound peer2")
        # This peer has to be outbound otherwise the stalling peer is
        # protected from disconnection
        peer2 = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=1, connection_type="outbound-full-relay")

        assert_equal(node.num_test_p2p_connections(), 2)
        return peer1, peer2

    def trigger_headers_timeout(self):
        self.log.info("Trigger the headers download timeout by advancing mock time")
        # The node has not received any headers from peers yet
        # So the best header time is the genesis block time
        best_header_time = self.nodes[0].getblockchaininfo()["time"]
        current_time = self.nodes[0].mocktime
        timeout = calculate_headers_timeout(best_header_time, current_time)
        # The calculated timeout above is always >= actual timeout, but we still need
        # +1 to trigger the timeout when both values are equal
        self.nodes[0].setmocktime(timeout + 1)

    def test_normal_peer_timeout(self):
        self.log.info("Test peer disconnection on header timeout")
        self.restart_node(0)
        self.nodes[0].setmocktime(int(time.time()))
        peer1, peer2 = self.setup_timeout_test_peers()

        with self.nodes[0].assert_debug_log(["Timeout downloading headers, disconnecting peer=0"]):
            self.trigger_headers_timeout()

        self.log.info("Check that stalling peer1 is disconnected")
        peer1.wait_for_disconnect()
        assert_equal(self.nodes[0].num_test_p2p_connections(), 1)

        self.log.info("Check that peer2 receives a getheaders request")
        peer2.wait_for_getheaders(block_hash=int(self.nodes[0].getbestblockhash(), 16))

    def test_noban_peer_timeout(self):
        self.log.info("Test noban peer on header timeout")
        self.restart_node(0, extra_args=['-whitelist=noban@127.0.0.1'])
        self.nodes[0].setmocktime(int(time.time()))
        peer1, peer2 = self.setup_timeout_test_peers()

        with self.nodes[0].assert_debug_log(["Timeout downloading headers from noban peer, not disconnecting peer=0"]):
            self.trigger_headers_timeout()

        self.log.info("Check that noban peer1 is not disconnected")
        peer1.sync_with_ping()
        assert_equal(self.nodes[0].num_test_p2p_connections(), 2)

        self.log.info("Check that exactly 1 of {peer1, peer2} receives a getheaders")
        self.assert_single_getheaders_recipient([peer1, peer2])

    def run_test(self):
        self.test_initial_headers_sync()
        self.test_normal_peer_timeout()
        self.test_noban_peer_timeout()


if __name__ == '__main__':
    HeadersSyncTest(__file__).main()
