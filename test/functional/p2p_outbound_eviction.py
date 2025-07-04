#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Test node outbound peer eviction logic

A subset of our outbound peers are subject to eviction logic if they cannot keep up
with our vision of the best chain. This criteria applies only to non-protected peers,
and can be triggered by either not learning about any blocks from an outbound peer after
a certain deadline, or by them not being able to catch up fast enough (under the same deadline).

This tests the different eviction paths based on the peer's behavior and on whether they are protected
or not.
"""
import time

from test_framework.messages import (
    from_hex,
    msg_headers,
    CBlockHeader,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework

# Timeouts (in seconds)
CHAIN_SYNC_TIMEOUT = 20 * 60
HEADERS_RESPONSE_TIME = 2 * 60


class P2POutEvict(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def test_outbound_eviction_unprotected(self):
        # This tests the eviction logic for **unprotected** outbound peers (that is, PeerManagerImpl::ConsiderEviction)
        node = self.nodes[0]
        cur_mock_time = node.mocktime

        # Get our tip header and its parent
        tip_header = from_hex(CBlockHeader(), node.getblockheader(node.getbestblockhash(), False))
        prev_header = from_hex(CBlockHeader(), node.getblockheader(f"{tip_header.hashPrevBlock:064x}", False))

        self.log.info("Create an outbound connection and don't send any headers")
        # Test disconnect due to no block being announced in 22+ minutes (headers are not even exchanged)
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="outbound-full-relay")
        # Wait for over 20 min to trigger the first eviction timeout. This sets the last call past 2 min in the future.
        cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
        node.setmocktime(cur_mock_time)
        peer.sync_with_ping()
        # Wait for over 2 more min to trigger the disconnection
        peer.wait_for_getheaders(block_hash=tip_header.hashPrevBlock)
        cur_mock_time += (HEADERS_RESPONSE_TIME + 1)
        node.setmocktime(cur_mock_time)
        self.log.info("Test that the peer gets evicted")
        peer.wait_for_disconnect()

        self.log.info("Create an outbound connection and send header but the peer never catches up")
        # Mimic a node that just falls behind for long enough
        # This should also apply for a node doing IBD that does not catch up in time
        # Connect a peer and make it send us headers ending in our tip's parent
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="outbound-full-relay")
        peer.send_and_ping(msg_headers([prev_header]))

        # Trigger the timeouts
        cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
        node.setmocktime(cur_mock_time)
        peer.sync_with_ping()
        peer.wait_for_getheaders(block_hash=tip_header.hashPrevBlock)
        cur_mock_time += (HEADERS_RESPONSE_TIME + 1)
        node.setmocktime(cur_mock_time)
        self.log.info("Test that the peer gets evicted")
        peer.wait_for_disconnect()

        self.log.info("Create an outbound connection and keep lagging behind, but not too much")
        # Test that if the peer never catches up with our current tip, but it does with the
        # expected work that we set when setting the timer (that is, our tip at the time)
        # the node does not disconnect the peer
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="outbound-full-relay")

        self.log.info("Mine a block so our peer starts lagging")
        prev_prev_hash = tip_header.hashPrevBlock
        best_block_hash = self.generateblock(node, output="raw(42)", transactions=[])["hash"]
        peer.sync_with_ping()

        self.log.info("The peer keeps catching up with the old tip; check that the node does not evict the peer")
        for i in range(10):
            # Generate an additional block so the peers is 2 blocks behind
            prev_header = from_hex(CBlockHeader(), node.getblockheader(best_block_hash, False))
            best_block_hash = self.generateblock(node, output="raw(42)", transactions=[])["hash"]
            tip_header = from_hex(CBlockHeader(), node.getblockheader(best_block_hash, False))
            peer.sync_with_ping()

            # Advance time but not enough to evict the peer
            cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
            node.setmocktime(cur_mock_time)
            peer.sync_with_ping()

            # Make the peer wait until it gets node's last call (by receiving a getheaders)
            peer.wait_for_getheaders(block_hash=prev_prev_hash)

            # The peer sends a header with the previous tip (so the peer goes back to 1 block behind)
            peer.send_and_ping(msg_headers([prev_header]))
            prev_prev_hash = tip_header.hashPrevBlock

        self.log.info("Create an outbound connection and take some time to catch up, but do it in time")
        # Check that if the peer manages to catch up within time, the timeouts are removed (and the peer is not disconnected)
        # We are reusing the peer from the previous case which already sent the node a valid (but old) block and whose timer is ticking

        # Make the peer send an updated headers message matching our tip
        peer.send_and_ping(msg_headers([from_hex(CBlockHeader(), node.getblockheader(best_block_hash, False))]))

        # Wait for long enough for the timeouts to have triggered and check that the peer is still connected
        cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
        node.setmocktime(cur_mock_time)
        peer.sync_with_ping()
        cur_mock_time += (HEADERS_RESPONSE_TIME + 1)
        node.setmocktime(cur_mock_time)
        self.log.info("Test that the peer does not get evicted")
        peer.sync_with_ping()

        node.disconnect_p2ps()

    def test_outbound_eviction_protected(self):
        # This tests the eviction logic for **protected** outbound peers (that is, PeerManagerImpl::ConsiderEviction)
        # Outbound connections are flagged as protected if:
        #   - The peer sends a connecting block with at least as much work as our current tip.
        #   - There are still available slots in the node's protected_peers list.
        # This test ensures that such protected outbound peers are not disconnected even after chain sync and headers timeouts.
        node = self.nodes[0]
        cur_mock_time = node.mocktime
        tip_header = from_hex(CBlockHeader(), node.getblockheader(node.getbestblockhash(), False))

        self.log.info("Create an outbound connection to a peer that shares our tip so it gets granted protection")
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="outbound-full-relay")
        peer.send_and_ping(msg_headers([tip_header]))

        self.log.info("Mine a new block and sync with our peer")
        self.generateblock(node, output="raw(42)", transactions=[])
        peer.sync_with_ping()

        self.log.info("Let enough time pass for the timeouts to go off")
        # Trigger the timeouts and check how we are still connected
        cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
        node.setmocktime(cur_mock_time)
        peer.sync_with_ping()
        peer.wait_for_getheaders(block_hash=tip_header.hashPrevBlock)
        cur_mock_time += (HEADERS_RESPONSE_TIME + 1)
        node.setmocktime(cur_mock_time)
        self.log.info("Test that the peer does not get evicted")
        peer.sync_with_ping()

        node.disconnect_p2ps()

    def test_outbound_eviction_mixed(self):
        # This tests the outbound eviction logic for a mix of protected and unprotected peers.
        node = self.nodes[0]
        cur_mock_time = node.mocktime

        self.log.info("Create a mix of protected and unprotected outbound connections to check against eviction")

        # Let's try this logic having multiple peers, some protected and some unprotected
        # We protect up to 4 peers as long as they have provided a block with the same amount of work as our tip
        self.log.info("The first 4 peers are protected by sending us a valid block with enough work")
        tip_header = from_hex(CBlockHeader(), node.getblockheader(node.getbestblockhash(), False))
        headers_message = msg_headers([tip_header])
        protected_peers = []
        for i in range(4):
            peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=i, connection_type="outbound-full-relay")
            peer.send_and_ping(headers_message)
            protected_peers.append(peer)

        # We can create 4 additional outbound connections to peers that are unprotected. 2 of them will be well behaved,
        # whereas the other 2 will misbehave (1 sending no headers, 1 sending old ones)
        self.log.info("The remaining 4 peers will be mixed between honest (2) and misbehaving peers (2)")
        prev_header = from_hex(CBlockHeader(), node.getblockheader(f"{tip_header.hashPrevBlock:064x}", False))
        headers_message = msg_headers([prev_header])
        honest_unprotected_peers = []
        for i in range(2):
            peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=4+i, connection_type="outbound-full-relay")
            peer.send_and_ping(headers_message)
            honest_unprotected_peers.append(peer)

        misbehaving_unprotected_peers = []
        for i in range(2):
            peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=6+i, connection_type="outbound-full-relay")
            if i%2==0:
                peer.send_and_ping(headers_message)
            misbehaving_unprotected_peers.append(peer)

        self.log.info("Mine a new block and keep the unprotected honest peer on sync, all the rest off-sync")
        # Mine a block so all peers become outdated
        target_hash = prev_header.hash_int
        tip_hash = self.generateblock(node, output="raw(42)", transactions=[])["hash"]
        tip_header = from_hex(CBlockHeader(), node.getblockheader(tip_hash, False))
        tip_headers_message = msg_headers([tip_header])

        # Let the timeouts hit and check back
        cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
        node.setmocktime(cur_mock_time)
        for peer in protected_peers + misbehaving_unprotected_peers:
            peer.sync_with_ping()
            peer.wait_for_getheaders(block_hash=target_hash)
        for peer in honest_unprotected_peers:
            peer.send_and_ping(tip_headers_message)
            peer.wait_for_getheaders(block_hash=target_hash)

        cur_mock_time += (HEADERS_RESPONSE_TIME + 1)
        node.setmocktime(cur_mock_time)
        self.log.info("Check that none of the honest or protected peers were evicted, but all misbehaving unprotected peers were")
        for peer in protected_peers + honest_unprotected_peers:
            peer.sync_with_ping()
        for peer in misbehaving_unprotected_peers:
            peer.wait_for_disconnect()

        node.disconnect_p2ps()

    def test_outbound_eviction_blocks_relay_only(self):
        # The logic for outbound eviction protection only applies to outbound-full-relay peers
        # This tests that other types of peers (blocks-relay-only for instance) are not granted protection
        node = self.nodes[0]
        cur_mock_time = node.mocktime
        tip_header = from_hex(CBlockHeader(), node.getblockheader(node.getbestblockhash(), False))

        self.log.info("Create an blocks-only outbound connection to a peer that shares our tip. This would usually grant protection")
        peer = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="block-relay-only")
        peer.send_and_ping(msg_headers([tip_header]))

        self.log.info("Mine a new block and sync with our peer")
        self.generateblock(node, output="raw(42)", transactions=[])
        peer.sync_with_ping()

        self.log.info("Let enough time pass for the timeouts to go off")
        # Trigger the timeouts and check how the peer gets evicted, since protection is only given to outbound-full-relay peers
        cur_mock_time += (CHAIN_SYNC_TIMEOUT + 1)
        node.setmocktime(cur_mock_time)
        peer.sync_with_ping()
        peer.wait_for_getheaders(block_hash=tip_header.hash_int)
        cur_mock_time += (HEADERS_RESPONSE_TIME + 1)
        node.setmocktime(cur_mock_time)
        self.log.info("Test that the peer gets evicted")
        peer.wait_for_disconnect()

        node.disconnect_p2ps()


    def run_test(self):
        self.nodes[0].setmocktime(int(time.time()))
        self.test_outbound_eviction_unprotected()
        self.test_outbound_eviction_protected()
        self.test_outbound_eviction_mixed()
        self.test_outbound_eviction_blocks_relay_only()


if __name__ == '__main__':
    P2POutEvict(__file__).main()
