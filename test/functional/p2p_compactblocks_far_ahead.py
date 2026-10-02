#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test handling of compact blocks more than two blocks ahead of the tip.

The node does not try to reconstruct such a block from its mempool:
- if it already requested the block from the peer, it asks for the full block again;
- otherwise, from a high-bandwidth peer, it handles the header as a headers message.
"""

from test_framework.blocktools import create_empty_fork
from test_framework.messages import (
    HeaderAndShortIDs,
    msg_block,
    msg_cmpctblock,
    msg_headers,
    msg_sendcmpct,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


def cmpctblock(block):
    compact = HeaderAndShortIDs()
    compact.initialize_from_block(block)
    return msg_cmpctblock(compact.to_p2p())


class CompactBlocksFarAheadTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def test_requested(self):
        self.log.info("A requested compact block far ahead is requested again as a full block")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_sendcmpct(announce=False, version=2))

        blocks = create_empty_fork(node, fork_length=3)
        peer.send_without_ping(msg_headers(blocks))
        peer.wait_for_getdata([b.hash_int for b in blocks])

        # The last block is still in flight, so only this response asks for it again.
        peer.send_without_ping(cmpctblock(blocks[-1]))
        peer.wait_for_getdata([blocks[-1].hash_int])

        for block in blocks:
            peer.send_and_ping(msg_block(block))
        assert_equal(node.getbestblockhash(), blocks[-1].hash_hex)

    def test_unrequested_from_hb_peer(self):
        self.log.info("An unrequested compact block far ahead from an outbound HB peer protects it from eviction")
        node = self.nodes[0]
        outbound = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=0, connection_type="outbound-full-relay")
        outbound.send_and_ping(msg_sendcmpct(announce=False, version=2))

        def outbound_info():
            return next(p for p in node.getpeerinfo() if p["connection_type"] == "outbound-full-relay")

        # Delivering a new tip makes the node select the peer as high-bandwidth.
        outbound.send_and_ping(msg_block(create_empty_fork(node, fork_length=1)[0]))
        assert outbound_info()["bip152_hb_to"]

        # Another peer announces the first two headers, so that the compact
        # block for the third is more than two blocks ahead of the tip.
        inbound = node.add_p2p_connection(P2PInterface())
        blocks = create_empty_fork(node, fork_length=3)
        inbound.send_and_ping(msg_headers(blocks[:2]))

        with node.assert_debug_log([f"Protecting outbound peer={outbound_info()['id']} from eviction"]):
            outbound.send_and_ping(cmpctblock(blocks[-1]))

    def run_test(self):
        # Leave IBD.
        self.generate(self.nodes[0], 1)
        self.test_requested()
        self.test_unrequested_from_hb_peer()


if __name__ == '__main__':
    CompactBlocksFarAheadTest(__file__).main()
