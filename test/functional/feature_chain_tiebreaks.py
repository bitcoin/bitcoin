#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the correct active block is chosen in complex reorgs."""

from test_framework.blocktools import create_block
from test_framework.messages import CBlockHeader
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class ChainTiebreaksTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    @staticmethod
    def send_headers(node, blocks):
        """Submit headers for blocks to node."""
        for block in blocks:
            # Use RPC rather than P2P, to prevent the message from being interpreted as a block
            # announcement.
            node.submitheader(hexdata=CBlockHeader(block).serialize().hex())

    def test_chain_split_in_memory(self):
        node = self.nodes[0]
        # Add P2P connection to bitcoind
        peer = node.add_p2p_connection(P2PDataStore())

        self.log.info('Precomputing blocks')
        #
        #          /- B3 -- B7
        #        B1      \- B8
        #       /  \
        #      /    \ B4 -- B9
        #   B0           \- B10
        #      \
        #       \  /- B5
        #        B2
        #          \- B6
        #
        blocks = []

        # Construct B0, building off genesis.
        start_height = node.getblockcount()
        blocks.append(create_block(
            hashprev=int(node.getbestblockhash(), 16),
            tmpl={"height": start_height + 1}
        ))
        blocks[-1].solve()

        # Construct B1-B10.
        for i in range(1, 11):
            blocks.append(create_block(
                hashprev=int(blocks[(i - 1) >> 1].hash, 16),
                tmpl={
                    "height": start_height + (i + 1).bit_length(),
                    # Make sure each block has a different hash.
                    "curtime": blocks[-1].nTime + 1,
                }
            ))
            blocks[-1].solve()

        self.log.info('Make sure B0 is accepted normally')
        peer.send_blocks_and_test([blocks[0]], node, success=True)
        # B0 must be active chain now.
        assert_equal(node.getbestblockhash(), blocks[0].hash)

        self.log.info('Send B1 and B2 headers, and then blocks in opposite order')
        self.send_headers(node, blocks[1:3])
        peer.send_blocks_and_test([blocks[2]], node, success=True)
        peer.send_blocks_and_test([blocks[1]], node, success=False)
        # B2 must be active chain now, as full data for B2 was received first.
        assert_equal(node.getbestblockhash(), blocks[2].hash)

        self.log.info('Send all further headers in order')
        self.send_headers(node, blocks[3:])
        # B2 is still the active chain, headers don't change this.
        assert_equal(node.getbestblockhash(), blocks[2].hash)

        self.log.info('Send blocks B7-B10')
        peer.send_blocks_and_test([blocks[7]], node, success=False)
        peer.send_blocks_and_test([blocks[8]], node, success=False)
        peer.send_blocks_and_test([blocks[9]], node, success=False)
        peer.send_blocks_and_test([blocks[10]], node, success=False)
        # B2 is still the active chain, as B7-B10 have missing parents.
        assert_equal(node.getbestblockhash(), blocks[2].hash)

        self.log.info('Send parents B3-B4 of B8-B10 in reverse order')
        peer.send_blocks_and_test([blocks[4]], node, success=False, force_send=True)
        peer.send_blocks_and_test([blocks[3]], node, success=False, force_send=True)
        # B9 is now active. Despite B7 being received earlier, the missing parent.
        assert_equal(node.getbestblockhash(), blocks[9].hash)

        self.log.info('Invalidate B9-B10')
        node.invalidateblock(blocks[9].hash)
        node.invalidateblock(blocks[10].hash)
        # B7 is now active.
        assert_equal(node.getbestblockhash(), blocks[7].hash)

        # Invalidate blocks to start fresh on the next test
        node.invalidateblock(blocks[0].hash)

    def test_chain_split_from_disk(self):
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PDataStore())

        self.log.info('Precomputing blocks')
        #
        #      A1
        #     /
        #   G
        #     \
        #      A2
        #
        blocks = []

        # Construct two blocks building from genesis.
        start_height = node.getblockcount()
        genesis_block = node.getblock(node.getblockhash(start_height))
        prev_time = genesis_block["time"]

        for i in range(0, 2):
            blocks.append(create_block(
                hashprev=int(genesis_block["hash"], 16),
                tmpl={"height": start_height + 1,
                # Make sure each block has a different hash.
                "curtime": prev_time + i + 1,
                }
            ))
            blocks[-1].solve()

        # Send blocks and test the last one is not connected
        self.log.info('Send A1 and A2. Make sure than only the former connects')
        peer.send_blocks_and_test([blocks[0]], node, success=True)
        peer.send_blocks_and_test([blocks[1]], node, success=False)

        self.log.info('Restart the node and check that the best tip before restarting matched the ones afterwards')
        # Restart and check enough times to this to eventually fail if the logic is broken
        for _ in range(10):
            self.restart_node(0)
            assert_equal(blocks[0].hash, node.getbestblockhash())

    def run_test(self):
        self.test_chain_split_in_memory()
        self.test_chain_split_from_disk()


if __name__ == '__main__':
    ChainTiebreaksTest().main()
