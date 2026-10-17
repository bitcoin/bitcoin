#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that recently reconnected addresses don't evict existing inbound peers.

When all inbound slots (or all tx-relay inbound slots) are taken, a new inbound
connection from an address that already connected twice within the last one to
two INBOUND_RECONNECT_WINDOWs (10 minutes) is dropped instead of evicting an
existing peer. With a free slot it is accepted as usual.

All test peers connect from 127.0.0.1, which is exempt from the check unless
-test=inbound_reconnect_check_local is passed. After two connections the
address counts as recent for the rest of the node's lifetime (or until the
window expires), so only the first two attempts after a (re)start are
unflagged.

With 2 inbound slots nothing is ever evictable, since 4 peers are protected by
netgroup before anything else. The old behaviour therefore shows up as the
"failed to find ... connection dropped" log lines rather than an eviction.
p2p_eviction.py covers the case where a peer would actually have been evicted.
"""
import time

from test_framework.messages import (
    msg_filterload,
    msg_version,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SUBVERSION,
    P2P_VERSION,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

WINDOW = 10 * 60  # INBOUND_RECONNECT_WINDOW in seconds

# 2 inbound slots (13 - 11 reserved for outbound), 1 of which may relay txs.
# The large -peertimeout keeps peers connected across the mocktime jump.
BASE_ARGS = ['-maxconnections=13', '-test=inbound_reconnect_check_local', '-peertimeout=999999999']

DROPPED_RECONNECT = 'dropped (reconnected too soon, inbound slots full)'
DROPPED_FULL = 'failed to find an eviction candidate - connection dropped (full)'
DROPPED_TXRELAY_RECONNECT = 'tx-relay inbound slots full and peer reconnected too soon - connection dropped after {} message'
DROPPED_TXRELAY_FULL = 'failed to find a tx-relaying eviction candidate - connection dropped after {} message'
EVICTED = 'selected inbound connection for eviction'


class P2PInboundReconnect(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [BASE_ARGS]

    def blocks_only_version(self):
        """VERSION with relay=0 (takes no tx-relay slot) and nServices=0 (no block-relay eviction protection)."""
        version = msg_version()
        version.nVersion = P2P_VERSION
        version.strSubVer = P2P_SUBVERSION
        version.nServices = 0
        version.relay = 0
        return version

    def add_blocks_only_peer(self):
        peer = self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        peer.send_without_ping(self.blocks_only_version())
        peer.wait_for_verack()
        return peer

    def make_attempt(self):
        """Connect and disconnect a peer, so that 127.0.0.1 has made one more attempt."""
        node = self.nodes[0]
        peers_before = len(node.getpeerinfo())
        peer = self.add_blocks_only_peer()
        peer.peer_disconnect()
        peer.wait_for_disconnect()
        self.wait_until(lambda: len(node.getpeerinfo()) == peers_before)

    def fill_inbound_slots(self):
        peers = [self.add_blocks_only_peer() for _ in range(2)]
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 2)
        return peers

    def connect_dropped_at_accept(self, expected, unexpected):
        """Connect while the inbound slots are full. The connection is dropped before any message."""
        node = self.nodes[0]
        with node.assert_debug_log([expected], unexpected_msgs=unexpected, timeout=5):
            node.add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False, expect_success=False)
        assert_equal(len(node.getpeerinfo()), 2)

    def connect_dropped_after_version(self, expected, unexpected):
        """Connect a tx-relaying peer while the tx-relay slot is taken. The connection is dropped after version."""
        node = self.nodes[0]
        with node.assert_debug_log([expected], unexpected_msgs=unexpected, timeout=5):
            node.add_p2p_connection(P2PInterface(), wait_for_verack=False, expect_success=False)
        self.wait_until(lambda: len(node.getpeerinfo()) == 1)

    def run_test(self):
        node = self.nodes[0]

        self.log.info('With a free slot, an address that reconnects is accepted every time')
        for _ in range(5):
            node.add_p2p_connection(P2PInterface())
            assert_equal(len(node.getpeerinfo()), 1)
            node.disconnect_p2ps()
            self.wait_until(lambda: len(node.getpeerinfo()) == 0)

        self.log.info('With the inbound slots full, a reconnecting address is dropped instead of evicting a peer')
        peers = self.fill_inbound_slots()
        self.connect_dropped_at_accept(DROPPED_RECONNECT, [DROPPED_FULL, EVICTED])
        assert all(peer.is_connected for peer in peers)

        self.log.info('After two windows without an attempt the address is forgotten')
        node.setmocktime(int(time.time()) + 2 * WINDOW + 60)
        self.connect_dropped_at_accept(DROPPED_FULL, [DROPPED_RECONNECT])
        self.connect_dropped_at_accept(DROPPED_FULL, [DROPPED_RECONNECT])
        self.log.info('...and the third attempt makes it recent again')
        self.connect_dropped_at_accept(DROPPED_RECONNECT, [DROPPED_FULL])
        assert all(peer.is_connected for peer in peers)

        self.log.info('With the tx-relay slot taken, a reconnecting tx-relaying peer is dropped after version')
        self.restart_node(0, BASE_ARGS)
        self.make_attempt()  # first attempt
        first = node.add_p2p_connection(P2PInterface())  # second attempt, unflagged, takes the tx-relay slot
        self.connect_dropped_after_version(DROPPED_TXRELAY_RECONNECT.format('version'),
                                           [DROPPED_TXRELAY_FULL.format('version'), EVICTED])
        assert first.is_connected

        self.log.info('A reconnecting peer that switches to tx relay via filterload is dropped, not another peer')
        self.restart_node(0, BASE_ARGS + ['-peerbloomfilters'])
        self.make_attempt()  # first attempt
        first = node.add_p2p_connection(P2PInterface())  # second attempt, unflagged, takes the tx-relay slot
        second = self.add_blocks_only_peer()  # third attempt, flagged, takes the other inbound slot
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)
        with node.assert_debug_log([DROPPED_TXRELAY_RECONNECT.format('filterload')],
                                   unexpected_msgs=[DROPPED_TXRELAY_FULL.format('filterload'), EVICTED], timeout=5):
            second.send_without_ping(msg_filterload(data=b'\xbb' * 100))
        second.wait_for_disconnect()
        self.wait_until(lambda: len(node.getpeerinfo()) == 1)
        assert first.is_connected

        self.log.info('Local addresses are exempt unless -test=inbound_reconnect_check_local is passed')
        self.restart_node(0, ['-maxconnections=13'])
        self.fill_inbound_slots()
        self.connect_dropped_at_accept(DROPPED_FULL, [DROPPED_RECONNECT])


if __name__ == '__main__':
    P2PInboundReconnect(__file__).main()
