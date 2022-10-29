#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getblockfrompeer RPC."""

from test_framework.authproxy import JSONRPCException
from test_framework.messages import (
    CBlock,
    from_hex,
    msg_headers,
    NODE_WITNESS,
)
from test_framework.p2p import (
    P2P_SERVICES,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class GetBlockFromPeerTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def setup_network(self):
        self.setup_nodes()

    def check_for_block(self, hash):
        try:
            self.nodes[0].getblock(hash)
            return True
        except JSONRPCException:
            return False

    def run_test(self):
        self.log.info("Mine 4 blocks on Node 0")
        self.generate(self.nodes[0], 4, sync_fun=self.no_op)
        assert_equal(self.nodes[0].getblockcount(), 204)

        self.log.info("Mine competing 3 blocks on Node 1")
        self.generate(self.nodes[1], 3, sync_fun=self.no_op)
        assert_equal(self.nodes[1].getblockcount(), 203)
        short_tip = self.nodes[1].getbestblockhash()

        self.log.info("Connect nodes to sync headers")
        self.connect_nodes(0, 1)
        self.sync_blocks()

        self.log.info("Node 0 should only have the header for node 1's block 3")
        x = next(filter(lambda x: x['hash'] == short_tip, self.nodes[0].getchaintips()))
        assert_equal(x['status'], "headers-only")
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[0].getblock, short_tip)

        self.log.info("Fetch block from node 1")
        peers = self.nodes[0].getpeerinfo()
        assert_equal(len(peers), 1)
        peer_0_peer_1_id = peers[0]["id"]

        self.log.info("Arguments must be valid")
        assert_raises_rpc_error(-8, "hash must be of length 64 (not 4, for '1234')", self.nodes[0].getblockfrompeer, "1234", peer_0_peer_1_id)
        assert_raises_rpc_error(-3, "JSON value of type number is not of expected type string", self.nodes[0].getblockfrompeer, 1234, peer_0_peer_1_id)
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].getblockfrompeer, short_tip, "0")

        self.log.info("We must already have the header")
        assert_raises_rpc_error(-1, "Block header missing", self.nodes[0].getblockfrompeer, "00" * 32, 0)

        self.log.info("Non-existent peer generates error")
        for peer_id in [-1, peer_0_peer_1_id + 1]:
            assert_raises_rpc_error(-1, "Peer does not exist", self.nodes[0].getblockfrompeer, short_tip, peer_id)

        self.log.info("Fetching from pre-segwit peer generates error")
        self.nodes[0].add_p2p_connection(P2PInterface(), services=P2P_SERVICES & ~NODE_WITNESS)
        peers = self.nodes[0].getpeerinfo()
        assert_equal(len(peers), 2)
        presegwit_peer_id = peers[1]["id"]
        assert_raises_rpc_error(-1, "Pre-SegWit peer", self.nodes[0].getblockfrompeer, short_tip, presegwit_peer_id)

        self.log.info("Successful fetch")
        result = self.nodes[0].getblockfrompeer(short_tip, peer_0_peer_1_id)
        self.wait_until(lambda: self.check_for_block(short_tip), timeout=1)
        assert_equal(result, {})

        self.log.info("Don't fetch blocks we already have")
        assert_raises_rpc_error(-1, "Block already downloaded", self.nodes[0].getblockfrompeer, short_tip, peer_0_peer_1_id)

        self.log.info("Don't fetch blocks while the node has not synced past it yet")
        # For this test we need node 1 in prune mode and as a side effect this also disconnects
        # the nodes which is also necessary for the rest of the test.
        self.restart_node(1, ["-prune=550"])

        # Generate a block on the disconnected node that the pruning node is not connected to
        blockhash = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[0]
        block_hex = self.nodes[0].getblock(blockhash=blockhash, verbosity=0)
        block = from_hex(CBlock(), block_hex)

        # Connect a P2PInterface to the pruning node and have it submit only the header of the
        # block that the pruning node has not seen
        node1_interface = self.nodes[1].add_p2p_connection(P2PInterface())
        node1_interface.send_and_ping(msg_headers([block]))

        # Get the peer id of the P2PInterface from the pruning node
        node1_peers = self.nodes[1].getpeerinfo()
        assert_equal(len(node1_peers), 1)
        node1_interface_id = node1_peers[0]["id"]

        # Trying to fetch this block from the P2PInterface should not be possible
        error_msg = "In prune mode, only blocks that the node has already synced previously can be fetched from a peer"
        assert_raises_rpc_error(-1, error_msg, self.nodes[1].getblockfrompeer, blockhash, node1_interface_id)


if __name__ == '__main__':
    GetBlockFromPeerTest().main()
