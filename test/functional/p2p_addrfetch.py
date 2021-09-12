#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test p2p addr-fetch connections
"""

import time

from test_framework.messages import msg_addr, CAddress, NODE_NETWORK, NODE_WITNESS
from test_framework.p2p import P2PInterface, p2p_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

ADDR = CAddress()
ADDR.time = int(time.time())
ADDR.nServices = NODE_NETWORK | NODE_WITNESS
ADDR.ip = "192.0.0.8"
ADDR.port = 18444


class P2PAddrFetch(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def assert_getpeerinfo(self, *, peer_ids):
        num_peers = len(peer_ids)
        info = self.nodes[0].getpeerinfo()
        assert_equal(len(info), num_peers)
        for n in range(0, num_peers):
            assert_equal(info[n]['id'], peer_ids[n])
            assert_equal(info[n]['connection_type'], 'addr-fetch')

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Connect to an addr-fetch peer")
        peer_id = 0
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=peer_id, connection_type="addr-fetch")
        self.assert_getpeerinfo(peer_ids=[peer_id])

        self.log.info("Check that we send getaddr but don't try to sync headers with the addr-fetch peer")
        peer.sync_send_with_ping()
        with p2p_lock:
            assert peer.message_count['getaddr'] == 1
            assert peer.message_count['getheaders'] == 0

        self.log.info("Check that answering the getaddr with a single address does not lead to disconnect")
        # This prevents disconnecting on self-announcements
        msg = msg_addr()
        msg.addrs = [ADDR]
        peer.send_and_ping(msg)
        self.assert_getpeerinfo(peer_ids=[peer_id])

        self.log.info("Check that answering with larger addr messages leads to disconnect")
        msg.addrs = [ADDR] * 2
        peer.send_message(msg)
        peer.wait_for_disconnect(timeout=5)

        self.log.info("Check timeout for addr-fetch peer that does not send addrs")
        peer_id = 1
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=peer_id, connection_type="addr-fetch")

        time_now = int(time.time())
        self.assert_getpeerinfo(peer_ids=[peer_id])

        # Expect addr-fetch peer connection to be maintained up to 5 minutes.
        node.setmocktime(time_now + 295)
        self.assert_getpeerinfo(peer_ids=[peer_id])

        # Expect addr-fetch peer connection to be disconnected after 5 minutes.
        node.setmocktime(time_now + 301)
        peer.wait_for_disconnect(timeout=5)
        self.assert_getpeerinfo(peer_ids=[])


if __name__ == '__main__':
    P2PAddrFetch().main()
