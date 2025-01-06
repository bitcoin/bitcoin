#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running bitcoind with -reindex and -reindex-chainstate options.

- Start a single node and generate 3 blocks.
- Stop the node and restart it with -reindex. Verify that the node has reindexed up to block 3.
- Stop the node and restart it with -reindex-chainstate. Verify that the node has reindexed up to block 3.
- Verify that out-of-order blocks are correctly processed, see LoadExternalBlockFile()
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
    MAGIC_BYTES,
    CBlockHeader,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    msg_block,
    msg_headers,
)
from test_framework.p2p import P2PInterface
from test_framework.script import (
    CScript,
    OP_TRUE,
)
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
            'LoadExternalBlockFile: Processing out of order child',
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

    # Check that reindex uses -assumevalid
    def assume_valid(self):
        self.log.info("Testing reindex with -assumevalid")
        self.start_node(0)
        node = self.nodes[0]
        self.generate(node, 1)
        coinbase_to_spend = node.getblock(node.getbestblockhash())['tx'][0]
        # Generate COINBASE_MATURITY blocks to make the coinbase spendable
        self.generate(node, COINBASE_MATURITY)
        best_blockhash = node.getbestblockhash()
        tip = int(best_blockhash, 16)
        block_info = node.getblock(best_blockhash)
        block_time = block_info['time'] + 1
        height = block_info['height'] + 1
        blocks = []

        # Create a block with an invalid signature
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(coinbase_to_spend, 16), 0), scriptSig=b""))
        tx.vout.append(CTxOut(49 * 10000, CScript([OP_TRUE])))

        invalid_block = create_block(tip, create_coinbase(height), block_time, txlist=[tx])
        invalid_block.solve()
        tip = invalid_block.hash_int
        blocks.append(invalid_block)

        # Bury that block with roughly two weeks' worth of blocks (2100 blocks) and an extra 100 blocks
        for _ in range(2200):
            block_time += 1
            height += 1
            block = create_block(tip, create_coinbase(height), block_time)
            block.solve()
            blocks.append(block)
            tip = block.hash_int

        # Start node0 with -assumevalid pointing to the last block
        node.stop_node()
        self.start_node(0, extra_args=["-assumevalid=" + blocks[-1].hash_hex])

        # Send blocks to node0
        p2p0 = node.add_p2p_connection(BaseNode())
        p2p0.send_header_for_blocks(blocks[0:2000])
        p2p0.send_header_for_blocks(blocks[2000:])
        # Only send the first 2100 blocks so the assumevalid block is not saved to disk before we reindex
        # This allows us to test that assumevalid is used in reindex even if the assumevalid block was not loaded during reindex
        for block in blocks[:2101]:
            p2p0.send_without_ping(msg_block(block))
        p2p0.sync_with_ping(timeout=960)

        expected_height = height - 100
        # node0 should sync to tip of the chain, ignoring the invalid signature
        assert_equal(node.getblock(node.getbestblockhash())['height'], expected_height)

        self.log.info("Testing reindex with assumevalid block in block index and minchainwork > best_header chainwork")
        # Restart node0 but raise minimumchainwork to be 100 blocks greater than the chainwork of the best header
        best_header_target = int(node.getblock(node.getbestblockhash())['target'], 16)
        chainwork = (100 + expected_height) * (2**256) // (best_header_target + 1)
        chainwork = format(chainwork, '064x')
        node.stop_node()
        self.start_node(0, extra_args=["-reindex", "-assumevalid=" + blocks[2100].hash_hex, "-minimumchainwork=" + chainwork])

        # node0 should sync to tip of the chain, ignoring the invalid signature
        best_blockhash = node.getbestblockhash()
        assert_equal(node.getblock(best_blockhash)['height'], expected_height)
        assert_equal(int(best_blockhash, 16), blocks[2100].hash_int)

        self.log.info("Testing reindex with assumevalid block not in block index")
        node.stop_node()
        self.start_node(0, extra_args=["-reindex", "-assumevalid=" + blocks[-1].hash_hex])

        # node0 should still sync to tip of the chain, ignoring the invalid signature
        best_blockhash = node.getbestblockhash()
        assert_equal(node.getblock(best_blockhash)['height'], expected_height)
        assert_equal(int(best_blockhash, 16), blocks[2100].hash_int)

        self.log.info("Test reindex without -assumevalid does not skip script validation")
        node.stop_node()
        self.start_node(0, extra_args=["-reindex"])
        best_blockhash = node.getbestblockhash()
        assert_equal(node.getblock(best_blockhash)['height'], block_info['height'])
        assert_equal(best_blockhash, block_info['hash'])

        self.log.info("Success")

    def run_test(self):
        self.reindex(False)
        self.reindex(True)
        self.reindex(False)
        self.reindex(True)

        self.out_of_order()
        self.continue_reindex_after_shutdown()
        self.assume_valid()


if __name__ == '__main__':
    ReindexTest(__file__).main()
