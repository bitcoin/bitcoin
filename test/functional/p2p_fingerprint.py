#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various fingerprinting protections.

If a stale block more than a month old or its header are requested by a peer,
the node should pretend that it does not have it to avoid fingerprinting.
"""

import time

from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.messages import (
    CInv,
    MSG_BLOCK,
    CBlockHeader,
    CBlock,
    HeaderAndShortIDs,
    from_hex,
)
from test_framework.p2p import (
    P2PInterface,
    msg_headers,
    msg_block,
    msg_cmpctblock,
    msg_getdata,
    msg_getheaders,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
)


class P2PFingerprintTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.onion_port = p2p_port(1)
        self.extra_args = [[f"-bind=127.0.0.1:{self.onion_port}=onion"]]

    # Build a chain of blocks on top of given one
    def build_chain(self, nblocks, prev_hash, prev_height, prev_median_time):
        blocks = []
        for _ in range(nblocks):
            coinbase = create_coinbase(prev_height + 1)
            block_time = prev_median_time + 1
            block = create_block(int(prev_hash, 16), coinbase, block_time)
            block.solve()

            blocks.append(block)
            prev_hash = block.hash
            prev_height += 1
            prev_median_time = block_time
        return blocks

    # Send a getdata request for a given block hash
    def send_block_request(self, block_hash, node):
        msg = msg_getdata()
        msg.inv.append(CInv(MSG_BLOCK, block_hash))
        node.send_message(msg)

    # Send a getheaders request for a given single block hash
    def send_header_request(self, block_hash, node):
        msg = msg_getheaders()
        msg.hashstop = block_hash
        node.send_message(msg)

    def test_header_leak_via_headers(self, peer_id, node, stale_hash, allowed_to_leak=False):
        if allowed_to_leak:
            self.log.info(f"check that existence of stale header {hex(stale_hash)[2:]} leaks. peer={peer_id}")
        else:
            self.log.info(f"check that existence of stale header {hex(stale_hash)[2:]} does not leak. peer={peer_id}")

        # Build headers for the fingerprinting attack
        fake_stale_headers = []
        prev_hash = stale_hash
        for i in range(16):
            fake_stale_header = CBlock()
            fake_stale_header.hashPrevBlock = prev_hash
            fake_stale_header.nBits = (32 << 24) + 0x7f0000
            fake_stale_header.solve()
            fake_stale_headers.append(fake_stale_header)
            prev_hash = fake_stale_header.rehash()

        self.log.info(f"send fake header with stale block as previous block. peer={peer_id}")
        with p2p_lock:
            node.last_message.pop("getheaders", None)
        node.send_message(msg_headers(fake_stale_headers[:1]))
        if allowed_to_leak:
            node.wait_for_disconnect()
            return
        else:
            node.wait_for_getheaders()

        self.log.info(f"send multiple fake headers with stale block as previous block. peer={peer_id}")
        with p2p_lock:
            node.last_message.pop("getheaders", None)
        with self.nodes[0].assert_debug_log(expected_msgs=[f"Misbehaving: peer={peer_id}"]):
            node.send_message(msg_headers(fake_stale_headers))

        self.log.info(f"send fake header using a compact block. peer={peer_id}")
        header_and_shortids = HeaderAndShortIDs()
        header_and_shortids.header = fake_stale_headers[0]
        with p2p_lock:
            node.last_message.pop("getheaders", None)
        node.send_message(msg_cmpctblock(header_and_shortids.to_p2p()))
        node.wait_for_getheaders()

    def get_header(self, header_hash):
        header = from_hex(CBlockHeader(), self.nodes[0].getblockheader(blockhash=header_hash, verbose=False))
        header.calc_sha256()
        assert_equal(header.hash, header_hash)
        return header

    # Checks that stale blocks timestamped more than a month ago are not served
    # by the node while recent stale blocks and old active chain blocks are.
    # This does not currently test that stale blocks timestamped within the
    # last month but that have over a month's worth of work are also withheld.
    def run_test(self):
        node0 = self.nodes[0].add_p2p_connection(P2PInterface())

        # Set node time to 60 days ago
        self.nodes[0].setmocktime(int(time.time()) - 60 * 24 * 60 * 60)

        # Generating a chain of 10 blocks
        block_hashes = self.generatetoaddress(self.nodes[0], 10, self.nodes[0].get_deterministic_priv_key().address)

        # Create longer chain starting 2 blocks before current tip
        height = len(block_hashes) - 2
        block_hash = block_hashes[height - 1]
        block_time = self.nodes[0].getblockheader(block_hash)["mediantime"] + 1
        new_blocks = self.build_chain(5, block_hash, height, block_time)

        # Force reorg to a longer chain
        node0.send_message(msg_headers(new_blocks))
        node0.wait_for_getdata([x.sha256 for x in new_blocks])
        for block in new_blocks:
            node0.send_and_ping(msg_block(block))

        # Check that reorg succeeded
        assert_equal(self.nodes[0].getblockcount(), 13)

        stale_hash = int(block_hashes[-1], 16)

        # Check that getdata request for stale block succeeds
        self.send_block_request(stale_hash, node0)
        node0.wait_for_block(stale_hash, timeout=3)

        # Check that getheader request for stale block header succeeds
        self.send_header_request(stale_hash, node0)
        node0.wait_for_header(hex(stale_hash), timeout=3)

        # Longest chain is extended so stale is much older than chain tip
        self.nodes[0].setmocktime(0)
        block_hash = int(self.generatetoaddress(self.nodes[0], 1, self.nodes[0].get_deterministic_priv_key().address)[-1], 16)
        assert_equal(self.nodes[0].getblockcount(), 14)
        node0.wait_for_block(block_hash, timeout=3)

        # Request for very old stale block should now fail
        with p2p_lock:
            node0.last_message.pop("block", None)
        self.send_block_request(stale_hash, node0)
        node0.sync_with_ping()
        assert "block" not in node0.last_message

        # Request for very old stale block header should now fail
        with p2p_lock:
            node0.last_message.pop("headers", None)
        self.send_header_request(stale_hash, node0)
        node0.sync_with_ping()
        assert "headers" not in node0.last_message

        # Verify we can fetch very old blocks and headers on the active chain
        block_hash = int(block_hashes[2], 16)
        self.send_block_request(block_hash, node0)
        self.send_header_request(block_hash, node0)
        node0.sync_with_ping()

        self.send_block_request(block_hash, node0)
        node0.wait_for_block(block_hash, timeout=3)

        self.send_header_request(block_hash, node0)
        node0.wait_for_header(hex(block_hash), timeout=3)

        clearnet_node1 = node0
        clearnet_node2 = self.nodes[0].add_p2p_connection(P2PInterface())
        onion_node = self.nodes[0].add_p2p_connection(P2PInterface(), dstport=self.onion_port)

        # Since the node mined the stale block itself, it should not leak it's
        # existence to any network.
        self.test_header_leak_via_headers(peer_id=0, node=clearnet_node1, stale_hash=stale_hash, allowed_to_leak=False)
        self.test_header_leak_via_headers(peer_id=1, node=clearnet_node2, stale_hash=stale_hash, allowed_to_leak=False)
        self.test_header_leak_via_headers(peer_id=2, node=onion_node, stale_hash=stale_hash, allowed_to_leak=False)

        # Send the stale headers through clearnet_node1's network.
        self.log.info("sending stale headers on clear net connection")
        clearnet_node1.send_and_ping(msg_headers([
            self.get_header(block_hashes[-2]),
            self.get_header(block_hashes[-1])
        ]))

        # The header should now leak on any clearnet connection.
        self.test_header_leak_via_headers(peer_id=0, node=clearnet_node1, stale_hash=stale_hash, allowed_to_leak=True)
        self.test_header_leak_via_headers(peer_id=1, node=clearnet_node2, stale_hash=stale_hash, allowed_to_leak=True)
        # Both clearnet connections should be disconnected because they sent
        # invalid headers that build on the stale header.
        assert(not clearnet_node1.is_connected)
        assert(not clearnet_node2.is_connected)

        # The header should still not be observable from another network.
        self.test_header_leak_via_headers(peer_id=2, node=onion_node, stale_hash=stale_hash, allowed_to_leak=False)

        # Send the stale headers through onion_node's network.
        self.log.info("sending stale headers on onion connection")
        onion_node.send_and_ping(msg_headers([
            self.get_header(block_hashes[-2]),
            self.get_header(block_hashes[-1])
        ]))

        self.test_header_leak_via_headers(peer_id=2, node=onion_node, stale_hash=stale_hash, allowed_to_leak=True)
        assert(not onion_node.is_connected)

if __name__ == '__main__':
    P2PFingerprintTest().main()
