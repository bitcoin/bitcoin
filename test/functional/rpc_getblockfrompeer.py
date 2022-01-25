#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getblockfrompeer RPC."""

from test_framework.authproxy import JSONRPCException
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

        self.log.info("Arguments must be sensible")
        assert_raises_rpc_error(-8, "hash must be of length 64 (not 4, for '1234')", self.nodes[0].getblockfrompeer, "1234", 0)

        self.log.info("We must already have the header")
        assert_raises_rpc_error(-1, "Block header missing", self.nodes[0].getblockfrompeer, "00" * 32, 0)

        self.log.info("Non-existent peer generates error")
        assert_raises_rpc_error(-1, "Peer does not exist", self.nodes[0].getblockfrompeer, short_tip, peer_0_peer_1_id + 1)

        self.log.info("Successful fetch")
        result = self.nodes[0].getblockfrompeer(short_tip, peer_0_peer_1_id)
        self.wait_until(lambda: self.check_for_block(short_tip), timeout=1)
        assert_equal(result, {})

        self.log.info("Don't fetch blocks we already have")
        assert_raises_rpc_error(-1, "Block already downloaded", self.nodes[0].getblockfrompeer, short_tip, peer_0_peer_1_id)

if __name__ == '__main__':
    GetBlockFromPeerTest().main()
