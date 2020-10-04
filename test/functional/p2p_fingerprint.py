#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test various fingerprinting protections.

If a stale block more than a month old or its header are requested by a peer,
the node should pretend that it does not have it to avoid fingerprinting.
"""

import time

from test_framework.blocktools import (create_block, create_coinbase)
from test_framework.messages import CInv, MSG_BLOCK
from test_framework.p2p import (
    P2PInterface,
    msg_headers,
    msg_block,
    msg_getdata,
    msg_getheaders,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class P2PFingerprintTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

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

    # Checks that stale blocks timestamped more than a month ago are not served
    # by the node while recent stale blocks and old active chain blocks are.
    # This does not currently test that stale blocks timestamped within the
    # last month but that have over a month's worth of work are also withheld.
    def run_test(self):
        node0 = self.nodes[0].add_p2p_connection(P2PInterface())

        # Set node time to 60 days ago
        self.nodes[0].setmocktime(int(time.time()) - 60 * 24 * 60 * 60)

        # Generating a chain of 10 blocks
        block_hashes = self.nodes[0].generatetoaddress(10, self.nodes[0].get_deterministic_priv_key().address)

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
        self.nodes[0].generatetoaddress(1, self.nodes[0].get_deterministic_priv_key().address)
        assert_equal(self.nodes[0].getblockcount(), 14)
        node0.sync_with_ping()

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

if __name__ == '__main__':
    P2PFingerprintTest().main()
