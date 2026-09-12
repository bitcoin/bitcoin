#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the interaction between ping timeouts and block download."""

import time

from test_framework.blocktools import create_empty_fork
from test_framework.messages import (
    CBlockHeader,
    CInv,
    MSG_BLOCK,
    MSG_TYPE_MASK,
    MSG_WITNESS_FLAG,
    msg_block,
    msg_getdata,
    msg_headers,
    msg_ping,
)
from test_framework.p2p import (
    NetworkThread,
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    mine_large_block,
)

from test_framework.wallet import MiniWallet

TIMEOUT_INTERVAL = 20 * 60
# Time the peer is given to answer a ping after it delivered its last block.
POST_BLOCK_PONG_GRACE = 60
MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16
PING_NONCE = 1
# Number of blocks the peer asks for in a single getdata message, the maximum a peer
# would request at the same time. The total amount of data requested (~16 MB) needs to
# be larger than what the kernel is willing to buffer for a socket that nobody reads
# from - otherwise the node would get through the entire request and answer the ping in
# spite of the peer not reading.
NUM_GETDATA = MAX_BLOCKS_IN_TRANSIT_PER_PEER
# Number of blocks the node downloads from the peer in the second subtest. Needs to
# be more than MAX_BLOCKS_IN_TRANSIT_PER_PEER, so that the peer keeps blocks in
# flight while it slowly serves them one by one.
NUM_DOWNLOAD_BLOCKS = 25


class SlowPeer(P2PInterface):
    """A peer that counts the blocks it receives."""
    def __init__(self):
        super().__init__()
        self.blocks_received = 0

    def on_block(self, message):
        self.blocks_received += 1


class BusyPeer(P2PInterface):
    """A peer that only serves a block when the test tells it to.

    It never answers a ping, mimicking a peer that works off its getdata queue
    before it gets around to processing the ping.
    """
    def __init__(self, blocks):
        super().__init__()
        self.blocks = {block.hash_int: block for block in blocks}
        self.getdata_requests = []
        self.blocks_served = 0

    def on_getdata(self, message):
        for inv in message.inv:
            if (inv.type & MSG_TYPE_MASK) == MSG_BLOCK:
                self.getdata_requests.append(inv.hash)

    def on_ping(self, message):
        pass

    def serve_next_block(self):
        """Serve the block that has been requested first and is still outstanding."""
        with p2p_lock:
            block = self.blocks[self.getdata_requests.pop(0)]
        self.send_without_ping(msg_block(block))
        self.blocks_served += 1


class PingIBDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Make the node stop serving blocks as soon as the peer stops reading from
        # the socket, instead of buffering up to 1 MB of them.
        self.extra_args = [["-maxsendbuffer=1"]]

    def set_reading(self, peer, enabled):
        """Pause or resume reading from the peer's socket."""
        pause_or_resume = peer._transport.resume_reading if enabled else peer._transport.pause_reading
        NetworkThread.network_event_loop.call_soon_threadsafe(pause_or_resume)

    def bytes_sent(self, node, msg_type):
        return node.getpeerinfo()[0]["bytessent_per_msg"].get(msg_type, 0)

    def wait_for_send_stall(self, node):
        """Wait until the node can't make any progress sending blocks to its peer anymore."""
        previous_bytes = 0

        def stalled():
            nonlocal previous_bytes
            block_bytes, previous_bytes = previous_bytes, self.bytes_sent(node, "block")
            return block_bytes > 0 and block_bytes == previous_bytes
        self.wait_until(stalled, check_interval=1)

    def test_pong_delay_ibd(self):
        # Tests that we only serve pings after serving all outstanding block requests, which could make a peer
        # timeout our node for not answering with a pong, even though the reason is their own slowness in downloading blocks.
        self.log.info("Check that during IBD, a node answers a ping only after serving the blocks asked for before it.")
        node = self.nodes[0]
        self.wallet = MiniWallet(node)

        self.log.info("Mine a large block that can be served to a peer over and over again")
        mine_large_block(self, self.wallet, node)
        block_hash = int(node.getbestblockhash(), 16)

        node.setmocktime(int(time.time()))
        # Don't use v2transport in this test, the unoptimized python ChaCha20
        # implementation would take a long time to decrypt the blocks that are served.
        peer = node.add_p2p_connection(SlowPeer(), supports_v2_p2p=False)
        # Answer the ping the node sends right after the handshake, so that it has no
        # ping of its own pending while the peer isn't reading from the socket.
        peer.wait_until(lambda: "ping" in peer.last_message)
        peer.sync_with_ping()
        pong_bytes = self.bytes_sent(node, "pong")

        self.log.info("Ask for a lot of blocks, send a ping and stop reading from the socket")
        self.set_reading(peer, False)
        peer.send_without_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, block_hash)] * NUM_GETDATA))
        peer.send_without_ping(msg_ping(nonce=PING_NONCE))
        self.wait_for_send_stall(node)
        assert_equal(self.bytes_sent(node, "pong"), pong_bytes)

        self.log.info(f"Check that the ping is still unanswered {TIMEOUT_INTERVAL + 60}s later")
        # Since we don't answer the ping within the ping timeout interval, the peer would disconnect
        # us if they don't relax the ping timeout rules while requesting blocks.
        # Note that the reason for the stall is not us, but the slowness of the peer itself, so
        # they would disconnect a good and fast peer that is not at fault.
        node.bumpmocktime(TIMEOUT_INTERVAL + 60)
        assert_equal(self.bytes_sent(node, "pong"), pong_bytes)

        self.log.info("Check that the pong only arrives after all requested blocks were sent")
        peer.last_message.pop("pong")
        self.set_reading(peer, True)
        peer.wait_until(lambda: "pong" in peer.last_message, timeout=120)
        assert_equal(peer.last_message["pong"].nonce, PING_NONCE)
        assert_equal(peer.blocks_received, NUM_GETDATA)
        node.disconnect_p2ps()

    def test_ping_timeout_ibd(self):
        self.log.info("Check that a peer isn't disconnected over a ping while it is serving blocks.")
        # The test framework uses a huge -peertimeout by default, which would disable the
        # ping timeout.
        self.restart_node(0, extra_args=["-peertimeout=1"])
        node = self.nodes[0]
        node.setmocktime(int(time.time()))

        self.log.info("Prepare the blocks that the node will download from the peer")
        blocks = create_empty_fork(node, NUM_DOWNLOAD_BLOCKS)

        start_height = node.getblockcount()
        peer = node.add_outbound_p2p_connection(BusyPeer(blocks), p2p_idx=0)
        # The node sends an initial ping as soon as it is connected. Since the peer doesn't answer it,
        # it would time out after TIMEOUT_INTERVAL.
        peer.wait_until(lambda: "ping" in peer.last_message)
        ping_start = node.mocktime

        self.log.info("Announce the blocks and wait for the node to request them")
        peer.send_and_ping(msg_headers([CBlockHeader(block) for block in blocks]))
        self.wait_until(lambda: len(node.getpeerinfo()[0]["inflight"]) == MAX_BLOCKS_IN_TRANSIT_PER_PEER)

        self.log.info(f"Send the blocks slowly, until the timeout of {TIMEOUT_INTERVAL}s is exceeded.")
        with node.assert_debug_log([], unexpected_msgs=["ping timeout"]):
            while node.mocktime <= ping_start + TIMEOUT_INTERVAL:
                node.bumpmocktime(TIMEOUT_INTERVAL // 5)
                peer.serve_next_block()
                self.wait_until(lambda: node.getblockcount() == start_height + peer.blocks_served)
                assert peer.is_connected
        assert_greater_than(node.getpeerinfo()[0]["pingwait"], TIMEOUT_INTERVAL)

        self.log.info("Serve the remaining blocks, so that nothing is in flight anymore")
        while peer.blocks_served < NUM_DOWNLOAD_BLOCKS:
            # The node only requests the next block once it processed an earlier one.
            peer.wait_until(lambda: len(peer.getdata_requests) > 0)
            peer.serve_next_block()
        self.wait_until(lambda: node.getblockcount() == start_height + NUM_DOWNLOAD_BLOCKS)
        assert_equal(node.getpeerinfo()[0]["inflight"], [])

        self.log.info("Check that the peer still gets a grace period to answer the ping")
        node.bumpmocktime(POST_BLOCK_PONG_GRACE // 2)
        peer.sync_with_ping()
        assert peer.is_connected

        self.log.info("Check that the ping timeout applies once the grace period is over")
        with node.assert_debug_log(expected_msgs=["ping timeout"]):
            node.bumpmocktime(POST_BLOCK_PONG_GRACE)
            peer.wait_for_disconnect()

    def run_test(self):
        self.test_pong_delay_ibd()
        self.test_ping_timeout_ibd()


if __name__ == '__main__':
    PingIBDTest(__file__).main()
