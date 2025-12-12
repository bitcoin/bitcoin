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
    CBlock,
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

    # makes sure stale tip check / extra peer eviction is run
    SCHEDULE_EXTRA_PEER_CHECK = 46
    # chosen large enough that EXTRA_NETWORK_PEER_INTERVAL and EXTRA_BLOCK_RELAY_ONLY_PEER_INTERVAL
    # which are on exponential 5min timers, will hit with very high probability
    MOCKTIME_BUMP = 5000

    def set_test_params(self):
        self.auto_outbound_mode = True
        self.num_nodes = 1
        self.extra_args = [["-test=addrman", "-cjdnsreachable"]]

    def all_sync_send_with_ping(self, peers):
        """Send a ping to all connected peers and wait for pongs."""
        for p in peers:
            node = p["node"]
            if node.is_connected:
                node.sync_with_ping()

    def has_regular_outbounds(self, node):
        """Check if we have 8 full outbound and 2 block-relay-only connections."""
        peer_info = node.getpeerinfo()
        full_outbound = [p for p in peer_info if p['connection_type'] == 'outbound-full-relay']
        block_relay = [p for p in peer_info if p['connection_type'] == 'block-relay-only']
        return len(full_outbound) == 8 and len(block_relay) == 2

    def has_extra_block_relay_peer(self, node):
        """Check if we have more than 2 block-relay-only peers."""
        peer_info = node.getpeerinfo()
        block_relay_peers = [p for p in peer_info if p['connection_type'] == 'block-relay-only']
        return len(block_relay_peers) > 2

    def has_extra_full_outbound_peer(self, node):
        """Check if we have more than 8 full outbound peers."""
        peer_info = node.getpeerinfo()
        full_outbound = [p for p in peer_info if p['connection_type'] == 'outbound-full-relay']
        return len(full_outbound) > 8

    def disconnect_last_block_relay_peer(self):
        """Find and disconnect the most recently connected block-relay-only peer.
           Done manually instead of waiting for the node to disconnect automatically to
           avoid rare interactions with the scheduling of the next extra block-relay-only peer"""
        node = self.nodes[0]
        peer_info = node.getpeerinfo()
        block_relay_peers = [p for p in peer_info if p['connection_type'] == 'block-relay-only']
        assert block_relay_peers
        # Disconnect the peer with the highest id
        last_peer = max(block_relay_peers, key=lambda p: p['id'])
        node.disconnectnode(nodeid=last_peer['id'])

    def wait_for_network_specific_peer(self):
        """Advance time, wait for a network-specific full outbound peer, and trigger eviction.

        Extra block-relay-only connections are prioritized over network-specific ones, but defensively:
        if an already connected tor or cjdns address is picked, there is no retry. This can cause either
        type of peer to arrive first.
        """
        node = self.nodes[0]

        self.log.info("Advance time")
        self.cur_time += self.MOCKTIME_BUMP
        node.setmocktime(self.cur_time)
        self.generate(node, 1)  # Disable stale-tip extra-outbound connections that could interfer

        self.log.info("Wait for either extra block-relay or network-specific peer")
        self.wait_until(lambda: self.has_extra_block_relay_peer(node) or self.has_extra_full_outbound_peer(node))
        self.all_sync_send_with_ping(self.auto_outbound_destinations)

        if self.has_extra_block_relay_peer(node) and not self.has_extra_full_outbound_peer(node):
            self.log.info("Got extra block-relay peer first, disconnecting to allow network-specific peer")
            self.disconnect_last_block_relay_peer()
            self.wait_until(lambda: self.has_regular_outbounds(node))
            self.log.info("Now wait for network-specific peer")
            self.wait_until(lambda: self.has_extra_full_outbound_peer(node))
            self.all_sync_send_with_ping(self.auto_outbound_destinations)
        else:
            self.log.info("Got network-specific peer directly")

        self.log.info("Trigger eviction to get back to regular outbound count")
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)  # triggers eviction
        self.wait_until(lambda: self.has_regular_outbounds(node))

    def setup_peers(self, node):
        self.log.info("Wait for the node to make 10 automatic outbound connections")
        self.wait_until(lambda: self.has_regular_outbounds(node))

        # Send current tip header from all peers to avoid being disconnected later on
        self.log.info("Send current tip header from all peers")
        tip_hash = node.getbestblockhash()
        tip_block = node.getblock(tip_hash, 0)
        block = from_hex(CBlock(), tip_block)
        tip_header = CBlockHeader(block)

        headers_msg = msg_headers()
        headers_msg.headers = [tip_header]

        for peer in self.auto_outbound_destinations:
            peer_node = peer["node"]
            if peer_node.is_connected:
                peer_node.send_and_ping(headers_msg)

        # Initialize regular scheduling of extra block relay connections
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)

    def test_stale_tip_and_extra_block_relay_only(self):
        self.log.info("Part 1: Test extra outbound connections when the tip is stale")
        node = self.nodes[0]

        self.setup_peers(node)

        self.log.info("Advance time to trigger stale tip and extra block relay only peers")
        self.cur_time += self.MOCKTIME_BUMP
        node.setmocktime(self.cur_time)
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)  # triggers stale tip check

        self.log.info("Wait for extra full outbound peer due to stale tip")
        self.wait_until(lambda: self.has_extra_full_outbound_peer(node))
        self.all_sync_send_with_ping(self.auto_outbound_destinations)

        self.log.info("Create new block and have the extra peer send it")
        tip = node.getbestblockhash()
        tip_info = node.getblock(tip)
        height = tip_info['height'] + 1
        block_time = tip_info['time'] + 1

        block = create_block(int(tip, 16), create_coinbase(height), block_time)
        block.solve()

        block_header = CBlockHeader(block)
        headers_msg = msg_headers()
        headers_msg.headers = [block_header]
        extra_peer = self.auto_outbound_destinations[-1]['node']
        extra_peer.send_and_ping(headers_msg)

        self.log.info("Wait for eviction of another peer")
        node.mockscheduler(self.SCHEDULE_EXTRA_PEER_CHECK)

        self.log.info("Send full block from stale tip peer")
        assert extra_peer.is_connected
        extra_peer.send_and_ping(msg_block(block))

        self.log.info("Check for extra block-relay-only peer")
        self.wait_until(lambda: self.has_extra_block_relay_peer(node))
        self.all_sync_send_with_ping(self.auto_outbound_destinations)

        # Verify we now have an extra block-relay-only peer
        peer_info = node.getpeerinfo()
        block_relay_peers = [p for p in peer_info if p['connection_type'] == 'block-relay-only']
        assert_equal(len(block_relay_peers), 3)

        self.log.info("Disconnect extra block-relay-only peer")
        self.disconnect_last_block_relay_peer()

        self.wait_until(lambda: self.has_regular_outbounds(node))

    def test_network_management(self):
        self.log.info("Part 2: Test management of outbound connection slots with respect to networks.")
        self.restart_node(0)
        node = self.nodes[0]
        node.setmocktime(self.cur_time)
        # Generate a block to prevent stale tip detection
        self.generate(node, 1)

        self.setup_peers(node)

        self.log.info("Add two onion and two cjdns addrs to addrman")
        addresses = [
            "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion",
            "nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion",
            "[fc00:1234:5678:9abc:def0:1234:5678:9abc]",
            "[fc11:abcd:ef01:2345:6789:abcd:ef01:2345]",
        ]
        for addr in addresses:
            node.addpeeraddress(address=addr, port=8333, tried=False)

        self.wait_for_network_specific_peer()

        peer_info = node.getpeerinfo()
        networks = set(p['network'] for p in peer_info)
        assert_equal(len(networks), 2)
        assert 'ipv4' in networks, "Expected IPv4 connections"
        assert 'onion' in networks or 'cjdns' in networks, "Expected at least one onion or cjdns connection"
        self.all_sync_send_with_ping(self.auto_outbound_destinations)

        self.wait_for_network_specific_peer()

        peer_info = node.getpeerinfo()
        networks = set(p['network'] for p in peer_info)
        assert_equal(len(networks), 3)
        assert 'ipv4' in networks, "Expected IPv4 connections"
        assert 'onion' in networks, "Expected onion connection"
        assert 'cjdns' in networks, "Expected cjdns connection"
        self.all_sync_send_with_ping(self.auto_outbound_destinations)

    def run_test(self):
        node = self.nodes[0]
        self.cur_time = int(time.time())
        node.setmocktime(self.cur_time)

        self.log.info("Add 100 random IPv4 addresses to addrman")
        for _ in range(100):
            ip = f"{random.randint(1, 223)}.{random.randint(0, 255)}.{random.randint(0, 255)}.{random.randint(1, 254)}"
            node.addpeeraddress(address=ip, port=8333, tried=False)

        self.test_stale_tip_and_extra_block_relay_only()
        self.test_network_management()


if __name__ == '__main__':
    P2POutboundManagement(__file__).main()
