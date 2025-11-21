#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test management of automatic connections:
- Extra outbound connections when the tip is stale
- Regularly scheduled extra block-relay-only connections
- Management of full outbound connection slots with respect to network-specific peers
"""

import random
import time

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    CBlockHeader,
    from_hex,
    msg_block,
    msg_headers,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class P2POutboundManagement(BitcoinTestFramework):

    # EXTRA_PEER_CHECK_INTERVAL (45s) + 1, makes sure stale tip check / extra peer eviction is run
    SCHEDULE_EXTRA_PEER_CHECK = 46
    # chosen large enough that EXTRA_NETWORK_PEER_INTERVAL and EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL
    # which are on exponential 5min timers, will hit with very high probability
    MOCKTIME_BUMP = 5000
    # The node only starts making extra block-relay-only connections once its tip is recent
    # (less than 20 * nPowTargetSpacing = 200 minutes old). An older tip keeps them disabled.
    OLD_TIP_AGE = 4 * 60 * 60
    # extra outbound peers are not evicted before they have been connected for this long (seconds)
    MINIMUM_CONNECT_TIME = 30

    def set_test_params(self):
        self.auto_outbound_mode = True
        self.num_nodes = 1
        self.extra_args = [["-test=addrman", "-cjdnsreachable"]]

    def sync_peers(self):
        """Send a ping to all connected peers and wait for pongs."""
        for p in self.auto_outbound_destinations:
            node = p["node"]
            if node.is_connected:
                node.sync_with_ping()

    def last_destination_of_type(self, conn_type):
        """Return the python peer of the most recent outbound connection of the given type."""
        with self.auto_outbound_destinations_lock:
            peers = [d['node'] for d in self.auto_outbound_destinations if d['conn_type'] == conn_type]
        assert peers
        return peers[-1]

    def count_peers(self, conn_type):
        """Return the number of peers of the given connection type that completed the version handshake.

        Peers show up in getpeerinfo as soon as the connection is established, but the node only
        counts them as (extra) outbound peers once the handshake is complete."""
        peer_info = self.nodes[0].getpeerinfo()
        return len([p for p in peer_info if p['connection_type'] == conn_type and 'verack' in p['bytesrecv_per_msg']])

    def has_regular_outbounds(self):
        """Check if we have 8 full outbound and 2 block-relay-only connections."""
        return self.count_peers('outbound-full-relay') == 8 and self.count_peers('block-relay-only') == 2

    def advance_time(self, seconds):
        self.cur_time += seconds
        self.nodes[0].setmocktime(self.cur_time)

    def create_old_block(self):
        """Create a block with a timestamp just after the one of the tip.

        Connecting it counts as a tip update (so that the tip is not considered stale),
        but it does not make the tip recent."""
        node = self.nodes[0]
        tip = node.getblockheader(node.getbestblockhash())
        block = create_block(int(tip['hash'], 16), create_coinbase(tip['height'] + 1), ntime=tip['time'] + 1)
        block.solve()
        return block

    def restart_and_setup_peers(self, *, recent_tip):
        """Restart the node and wait for it to make its regular outbound connections.

        All kinds of extra outbound connections share a single spare connection slot. The age of
        the tip decides whether extra block-relay-only connections are made, so that each subtest
        can deal with one kind of extra connection only.

        Starting with -mocktime ensures that the timers for extra connections, which are set
        during startup, only expire once the test advances the time."""
        node = self.nodes[0]
        if not recent_tip:
            tip_time = node.getblockheader(node.getbestblockhash())['time']
            self.cur_time = max(self.cur_time, tip_time + self.OLD_TIP_AGE)
        self.restart_node(0, extra_args=node.extra_args + [f"-mocktime={self.cur_time}"])
        if recent_tip:
            self.generate(node, 1)

        self.log.info("Wait for the node to make 10 automatic outbound connections")
        self.wait_until(self.has_regular_outbounds)

        # Send current tip header from all peers to avoid being disconnected later on
        tip_header = from_hex(CBlockHeader(), node.getblockheader(node.getbestblockhash(), False))
        headers_msg = msg_headers()
        headers_msg.headers = [tip_header]
        for peer in self.auto_outbound_destinations:
            if peer["node"].is_connected:
                peer["node"].send_and_ping(headers_msg)

        # Run the first stale tip check. If the tip is recent, this also starts the
        # scheduling of extra block-relay-only connections.
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)

    def test_stale_tip(self):
        self.log.info("Test extra full outbound connection when the tip is stale")
        node = self.nodes[0]
        self.restart_and_setup_peers(recent_tip=False)

        self.log.info("Advance time to make the tip stale")
        self.advance_time(self.MOCKTIME_BUMP)
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)  # triggers stale tip check

        self.log.info("Wait for extra full outbound peer")
        self.wait_until(lambda: self.count_peers('outbound-full-relay') == 9)
        self.sync_peers()
        extra_peer = self.last_destination_of_type('outbound-full-relay')

        self.log.info("Announce a new block from the extra peer")
        block = self.create_old_block()
        headers_msg = msg_headers()
        headers_msg.headers = [CBlockHeader(block)]
        extra_peer.send_and_ping(headers_msg)

        self.log.info("Trigger eviction and check that another peer is evicted")
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)
        self.wait_until(self.has_regular_outbounds)
        assert extra_peer.is_connected

        self.log.info("Send the block from the extra peer")
        extra_peer.send_and_ping(msg_block(block))
        assert_equal(node.getbestblockhash(), block.hash_hex)

    def test_extra_block_relay_only(self):
        self.log.info("Test regularly scheduled extra block-relay-only connections")
        node = self.nodes[0]
        self.restart_and_setup_peers(recent_tip=True)

        self.log.info("Advance time to trigger an extra block-relay-only connection")
        self.advance_time(self.MOCKTIME_BUMP)
        self.generate(node, 1)  # prevents extra full outbound connections due to a stale tip

        self.wait_until(lambda: self.count_peers('block-relay-only') == 3)
        self.sync_peers()
        assert_equal(self.count_peers('outbound-full-relay'), 8)
        extra_peer = self.last_destination_of_type('block-relay-only')

        self.log.info("Check that the extra block-relay-only peer is evicted")
        self.advance_time(self.MINIMUM_CONNECT_TIME)
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)
        extra_peer.wait_for_disconnect()

    def test_network_specific_peers(self):
        self.log.info("Test management of full outbound connection slots with respect to networks")
        node = self.nodes[0]
        self.restart_and_setup_peers(recent_tip=False)

        self.log.info("Add two onion and two cjdns addrs to addrman")
        addresses = [
            "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion",
            "nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion",
            "[fc00:1234:5678:9abc:def0:1234:5678:9abc]",
            "[fc11:abcd:ef01:2345:6789:abcd:ef01:2345]",
        ]
        for addr in addresses:
            node.addpeeraddress(address=addr, port=8333, tried=False)

        for expected_networks in [2, 3]:
            self.log.info("Advance time to trigger an extra network-specific connection")
            self.advance_time(self.MOCKTIME_BUMP)
            # prevents extra full outbound connections due to a stale tip
            assert_equal(node.submitblock(self.create_old_block().serialize().hex()), None)

            self.wait_until(lambda: self.count_peers('outbound-full-relay') == 9)
            self.sync_peers()

            self.log.info("Trigger eviction, the only peer of a network is protected")
            node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)
            self.wait_until(self.has_regular_outbounds)

            networks = set(p['network'] for p in node.getpeerinfo())
            assert 'ipv4' in networks
            assert_equal(len(networks), expected_networks)

    def run_test(self):
        node = self.nodes[0]
        self.cur_time = int(time.time())

        self.log.info("Add 100 random IPv4 addresses to addrman")
        for _ in range(100):
            ip = f"{random.randint(1, 223)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(1, 254)}"
            node.addpeeraddress(address=ip, port=8333, tried=False)

        self.test_stale_tip()
        self.test_extra_block_relay_only()
        self.test_network_specific_peers()


if __name__ == '__main__':
    P2POutboundManagement(__file__).main()
