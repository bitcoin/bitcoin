#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test initial headers download

Test that we only try to initially sync headers from one peer (until our chain
is close to caught up), and that each block announcement results in only one
additional peer receiving a getheaders message.
"""

from test_framework.test_framework import SyscoinTestFramework
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
import random

class HeadersSyncTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def announce_random_block(self, peers):
        new_block_announcement = msg_inv(inv=[CInv(MSG_BLOCK, random.randrange(1<<256))])
        for p in peers:
            p.send_and_ping(new_block_announcement)

    def run_test(self):
        self.log.info("Adding a peer to node0")
        peer1 = self.nodes[0].add_p2p_connection(P2PInterface())

        # Wait for peer1 to receive a getheaders
        peer1.wait_for_getheaders()
        # An empty reply will clear the outstanding getheaders request,
        # allowing additional getheaders requests to be sent to this peer in
        # the future.
        peer1.send_message(msg_headers())

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

        with p2p_lock:
            peer1.last_message.pop("getheaders", None)

        self.log.info("Have all peers announce a new block")
        self.announce_random_block(all_peers)

        self.log.info("Check that peer1 receives a getheaders in response")
        peer1.wait_for_getheaders()
        peer1.send_message(msg_headers()) # Send empty response, see above
        with p2p_lock:
            peer1.last_message.pop("getheaders", None)

        self.log.info("Check that exactly 1 of {peer2, peer3} received a getheaders in response")
        count = 0
        peer_receiving_getheaders = None
        for p in [peer2, peer3]:
            with p2p_lock:
                if "getheaders" in p.last_message:
                    count += 1
                    peer_receiving_getheaders = p
                    p.last_message.pop("getheaders", None)
                    p.send_message(msg_headers()) # Send empty response, see above

        assert_equal(count, 1)

        self.log.info("Announce another new block, from all peers")
        self.announce_random_block(all_peers)

        self.log.info("Check that peer1 receives a getheaders in response")
        peer1.wait_for_getheaders()

        self.log.info("Check that the remaining peer received a getheaders as well")
        expected_peer = peer2
        if peer2 == peer_receiving_getheaders:
            expected_peer = peer3

        expected_peer.wait_for_getheaders()

        self.log.info("Success!")

if __name__ == '__main__':
    HeadersSyncTest().main()

