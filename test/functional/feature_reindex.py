#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running bitcoind with -reindex and -reindex-chainstate options.

- Start a single node and generate 3 blocks.
- Stop the node and restart it with -reindex. Verify that the node has reindexed up to block 3.
- Stop the node and restart it with -reindex-chainstate. Verify that the node has reindexed up to block 3.
- Verify that out-of-order blocks are correctly processed, see LoadExternalBlockFile()
- Verify that -reindex does not attempt to connect blocks when best_header chainwork is below MinimumChainWork
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    CBlockHeader,
    msg_block,
    msg_headers,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import MAGIC_BYTES
from test_framework.p2p import P2PInterface
from test_framework.util import (
    assert_equal,
    util_xor,
)

class BaseNode(P2PInterface):
    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_without_ping(headers_message)


class ReindexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def reindex(self, justchainstate=False):
        self.generatetoaddress(self.nodes[0], 3, self.nodes[0].get_deterministic_priv_key().address)
        blockcount = self.nodes[0].getblockcount()
        self.stop_nodes()
        extra_args = [["-reindex-chainstate" if justchainstate else "-reindex"]]
        self.start_nodes(extra_args)
        assert_equal(self.nodes[0].getblockcount(), blockcount)  # start_node is blocking on reindex
        self.log.info("Success")

    # Check that blocks can be processed out of order
    def out_of_order(self):
        # The previous test created 12 blocks
        assert_equal(self.nodes[0].getblockcount(), 12)
        self.stop_nodes()

        # In this test environment, blocks will always be in order (since
        # we're generating them rather than getting them from peers), so to
        # test out-of-order handling, swap blocks 1 and 2 on disk.
        blk0 = self.nodes[0].blocks_path / "blk00000.dat"
        xor_dat = self.nodes[0].read_xor_key()

        with open(blk0, 'r+b') as bf:
            # Read at least the first few blocks (including genesis)
            b = util_xor(bf.read(2000), xor_dat, offset=0)

            # Find the offsets of blocks 2, 3, and 4 (the first 3 blocks beyond genesis)
            # by searching for the regtest marker bytes (see pchMessageStart).
            def find_block(b, start):
                return b.find(MAGIC_BYTES["regtest"], start)+4

            genesis_start = find_block(b, 0)
            assert_equal(genesis_start, 4)
            b2_start = find_block(b, genesis_start)
            b3_start = find_block(b, b2_start)
            b4_start = find_block(b, b3_start)

            # Blocks 2 and 3 should be the same size.
            assert_equal(b3_start - b2_start, b4_start - b3_start)

            # Swap the second and third blocks (don't disturb the genesis block).
            bf.seek(b2_start)
            bf.write(util_xor(b[b3_start:b4_start], xor_dat, offset=b2_start))
            bf.write(util_xor(b[b2_start:b3_start], xor_dat, offset=b3_start))

        # The reindexing code should detect and accommodate out of order blocks.
        with self.nodes[0].assert_debug_log([
            'LoadExternalBlockFile: Out of order block',
            'LoadOutOfOrderBlocks: Processing out of order child',
        ]):
            extra_args = [["-reindex"]]
            self.start_nodes(extra_args)

        # All blocks should be accepted and processed.
        assert_equal(self.nodes[0].getblockcount(), 12)

    def continue_reindex_after_shutdown(self):
        node = self.nodes[0]
        self.generate(node, 1500)

        # Restart node with reindex and stop reindex as soon as it starts reindexing
        self.log.info("Restarting node while reindexing..")
        node.stop_node()
        with node.busy_wait_for_debug_log([b'initload thread start']):
            node.start(['-blockfilterindex', '-reindex'])
            node.wait_for_rpc_connection(wait_for_import=False)
        node.stop_node()

        # Start node without the reindex flag and verify it does not wipe the indexes data again
        db_path = node.chain_path / 'indexes' / 'blockfilter' / 'basic' / 'db'
        with node.assert_debug_log(expected_msgs=[f'Opening LevelDB in {db_path}'], unexpected_msgs=[f'Wiping LevelDB in {db_path}']):
            node.start(['-blockfilterindex'])
            node.wait_for_rpc_connection(wait_for_import=False)
        node.stop_node()

    def only_connect_minchainwork_chain(self):
        self.start_node(0)
        node = self.nodes[0]
        self.generatetoaddress(self.nodes[0], 3, self.nodes[0].get_deterministic_priv_key().address)
        blockcount = node.getblockcount()
        best_header_target = int(node.getblock(node.getbestblockhash())['target'], 16)
        chainwork = (100 + blockcount) * (2**256) // (best_header_target + 1)
        chainwork = format(chainwork, '064x')

        tip = int(node.getbestblockhash(), 16)
        block_time = node.getblock(node.getbestblockhash())['time'] + 1
        height = node.getblock(node.getbestblockhash())['height'] + 1

        blocks = []
        for _ in range(100):
            block = create_block(tip, create_coinbase(height), block_time)
            block.solve()
            blocks.append(block)
            tip = block.hash_int
            block_time += 1
            height += 1

        self.stop_node(0)
        extra_args = ["-reindex", "-minimumchainwork=" + chainwork]
        with node.assert_debug_log(expected_msgs=["Waiting for header sync to finish before activating chain..."]):
            # No blocks are connected because chainwork of best_header is too low
            self.start_node(0, extra_args)

        p2p = node.add_p2p_connection(BaseNode())
        p2p.send_header_for_blocks(blocks)

        # Reindexed blocks are connected after headers sync
        self.wait_until(lambda: node.getblockcount() == blockcount)

        # Send headers again to ensure that the block is requested
        p2p.send_header_for_blocks(blocks)
        p2p.send_without_ping(msg_block(blocks[0]))
        self.wait_until(lambda: node.getblockcount() == blockcount + 1)

    def run_test(self):
        self.reindex(False)
        self.reindex(True)
        self.reindex(False)
        self.reindex(True)

        self.out_of_order()
        self.continue_reindex_after_shutdown()
        self.only_connect_minchainwork_chain()


if __name__ == '__main__':
    ReindexTest(__file__).main()
