#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    msg_version,
    msg_filterload,
    msg_mempool,
)
from test_framework.p2p import (
    P2PInterface,
    P2PTxInvStore,
    P2P_SERVICES,
    P2P_SUBVERSION,
    P2P_VERSION,
)
from test_framework.wallet import MiniWallet


class P2PConnectionLimits(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # scenario : we have 2 inbound slots and allow a maximum of 1 tx-relaying inbound peer
        self.extra_args = [['-maxconnections=13']]  # 11 slots are reserved for outbounds, leaving 2 inbound slots

    def run_test(self):
        self.test_inbound_limits()

    def add_relay_disabled_peers(self, p2ps, *, services=P2P_SERVICES):
        version_msg = msg_version()
        version_msg.nVersion = P2P_VERSION
        version_msg.strSubVer = P2P_SUBVERSION
        version_msg.nServices = services
        version_msg.relay = 0
        peers = [self.nodes[0].add_p2p_connection(p2p, send_version=False, wait_for_verack=False) for p2p in p2ps]
        for peer in peers:
            peer.send_without_ping(version_msg)
        for peer in peers:
            peer.wait_for_verack()
        return peers

    def add_relay_disabled_peer(self, p2p, *, services=P2P_SERVICES):
        return self.add_relay_disabled_peers([p2p], services=services)[0]

    def test_inbound_limits(self):
        node = self.nodes[0]

        self.log.info('Test with 2 inbound slots, one of which allows tx-relay')
        node.add_p2p_connection(P2PInterface())

        self.log.info('Connect a full-relay inbound peer - test that eviction is triggered')
        # Since there is no unprotected peer to evict here, the new peer is dropped instead.
        with node.assert_debug_log(['failed to find a tx-relaying eviction candidate - connection dropped'], timeout=2):
            self.nodes[0].add_p2p_connection(P2PInterface(), expect_success=False, wait_for_verack=False)
        self.wait_until(lambda: len(node.getpeerinfo()) == 1)
        node.disconnect_p2ps()

        self.log.info('Connect a block-relay inbound peer - test that second full relay peer is accepted')
        self.add_relay_disabled_peer(P2PInterface())

        node.add_p2p_connection(P2PInterface())
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)

        self.log.info('Connecting another full-relay peer triggers non-specific eviction')
        with node.assert_debug_log(['failed to find an eviction candidate - connection dropped (full)'], timeout=2):
            self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False, expect_success=False)
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)

        self.log.info('Run with bloom filter support and check that a switch to tx relay during runtime can trigger eviction')
        self.restart_node(0, ['-maxconnections=13', '-peerbloomfilters'])
        peer1 = self.add_relay_disabled_peer(P2PInterface())

        node.add_p2p_connection(P2PInterface())
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)
        with node.assert_debug_log(['connection dropped after filterload message'], timeout=2):
            peer1.send_without_ping(msg_filterload(data=b'\xbb'*(100)))
        self.wait_until(lambda: len(node.getpeerinfo()) == 1)

        self.log.info('Check BIP35 requests do not enable ongoing transaction relay')
        self.restart_node(0, ['-maxconnections=13', '-peerbloomfilters', '-inboundrelaypercent=100'])
        node.setmocktime(int(time.time()))
        wallet = MiniWallet(node)
        # Complete the requested snapshot before creating the transaction tested for ongoing relay.
        snapshot_tx = wallet.send_self_transfer(from_node=node)
        bip35_peer = self.add_relay_disabled_peer(P2PTxInvStore())
        bip35_peer.send_and_ping(msg_mempool())
        node.bumpmocktime(60)
        bip35_peer.wait_for_broadcast([snapshot_tx['wtxid']])

        relay_peer = node.add_p2p_connection(P2PTxInvStore())
        tx = wallet.send_self_transfer(from_node=node)
        node.bumpmocktime(60)
        relay_peer.wait_for_broadcast([tx['wtxid']])
        bip35_peer.sync_with_ping()
        assert int(tx['wtxid'], 16) not in bip35_peer.get_invs()

        self.log.info('Check BIP35 requests against inbound transaction-relay capacity')
        # NODE_BLOOM permits BIP35, while zero capacity makes any counted requester exceed the limit.
        self.restart_node(0, ['-maxconnections=13', '-peerbloomfilters', '-inboundrelaypercent=0'])
        peer1 = self.add_relay_disabled_peer(P2PInterface())
        with node.assert_debug_log(['connection dropped after mempool message'], timeout=2):
            peer1.send_without_ping(msg_mempool())
            peer1.wait_for_disconnect()

        self.log.info('Test different values of inboundrelaypercent')
        self.restart_node(0, ['-maxconnections=13', '-inboundrelaypercent=0'])
        with node.assert_debug_log(['failed to find a tx-relaying eviction candidate - connection dropped'], timeout=2):
            self.nodes[0].add_p2p_connection(P2PInterface(), expect_success=False, wait_for_verack=False)

        self.restart_node(0, ['-maxconnections=13', '-inboundrelaypercent=100'])
        node.add_p2p_connection(P2PInterface())
        node.add_p2p_connection(P2PInterface())
        self.wait_until(lambda: len(node.getpeerinfo()) == 2)

        self.log.info('Test that EvictTxPeerIfFull only evicts tx-relaying peers')
        NUM_BLOCK_RELAY_PEERS = 21
        self.restart_node(0, ['-maxconnections=33', '-inboundrelaypercent=0'])
        # Avoid block-relay eviction protection.
        self.add_relay_disabled_peers([P2PInterface() for _ in range(NUM_BLOCK_RELAY_PEERS)], services=0)
        self.wait_until(lambda: len(node.getpeerinfo()) == NUM_BLOCK_RELAY_PEERS)

        with node.assert_debug_log(['failed to find a tx-relaying eviction candidate - connection dropped'], timeout=5):
            self.nodes[0].add_p2p_connection(P2PInterface(), expect_success=False, wait_for_verack=False)
        self.wait_until(lambda: len(node.getpeerinfo()) == NUM_BLOCK_RELAY_PEERS)

if __name__ == '__main__':
    P2PConnectionLimits(__file__).main()
