#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test processing of unrequested blocks.

Setup: two nodes, node0+node1, not connected to each other. Node1 will have
nMinimumChainWork set to 0x10, so it won't process low-work unrequested blocks.

We have one P2PInterface connection to node0 called test_node, and one to node1
called min_work_node.

The test:
1. Generate one block on each node, to leave IBD.

2. Mine a new block on each tip, and deliver to each node from node's peer.
   The tip should advance for node0, but node1 should skip processing due to
   nMinimumChainWork.

Node1 is unused in tests 3-7:

3. Mine a block that forks from the genesis block, and deliver to test_node.
   Node0 should not process this block (just accept the header), because it
   is unrequested and doesn't have more or equal work to the tip.

4a,b. Send another two blocks that build on the forking block.
   Node0 should process the second block but be stuck on the shorter chain,
   because it's missing an intermediate block.

4c.Send 288 more blocks on the longer chain (the number of blocks ahead
   we currently store).
   Node0 should process all but the last block (too far ahead in height).

5. Send a duplicate of the block in #3 to Node0.
   Node0 should not process the block because it is unrequested, and stay on
   the shorter chain.

6. Send Node0 an inv for the height 3 block produced in #4 above.
   Node0 should figure out that Node0 has the missing height 2 block and send a
   getdata.

7. Send Node0 the missing block again.
   Node0 should process and the tip should advance.

8. Create a fork which is invalid at a height longer than the current chain
   (ie to which the node will try to reorg) but which has headers built on top
   of the invalid block. Check that we get disconnected if we send more headers
   on the chain the node now knows to be invalid.

9. Test Node1 is able to sync when connected to node0 (which should have sufficient
   work on its chain).
"""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase, create_tx_with_script


class AcceptBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ["-minimumchainwork=0x10"]]

    def setup_network(self):
        # Node0 will be used to test behavior of processing unrequested blocks
        # from peers which are not whitelisted, while Node1 will be used for
        # the whitelisted case.
        # Node2 will be used for non-whitelisted peers to test the interaction
        # with nMinimumChainWork.
        self.setup_nodes()

    def run_test(self):
        # Setup the p2p connections
        # test_node connects to node0 (not whitelisted)
        test_node = self.nodes[0].add_p2p_connection(P2PInterface())
        # min_work_node connects to node1 (whitelisted)
        min_work_node = self.nodes[1].add_p2p_connection(P2PInterface())

        # 1. Have nodes mine a block (leave IBD)
        [ n.generate(1) for n in self.nodes ]
        tips = [ int("0x" + n.getbestblockhash(), 0) for n in self.nodes ]

        # 2. Send one block that builds on each tip.
        # This should be accepted by node0
        blocks_h2 = []  # the height 2 blocks on each node's chain
        block_time = self.mocktime + 1
        for i in range(2):
            blocks_h2.append(create_block(tips[i], create_coinbase(2), block_time + 1))
            blocks_h2[i].solve()
            block_time += 1
        test_node.send_message(msg_block(blocks_h2[0]))
        min_work_node.send_message(msg_block(blocks_h2[1]))

        for x in [test_node, min_work_node]:
            x.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 2)
        assert_equal(self.nodes[1].getblockcount(), 1)
        self.log.info("First height 2 block accepted by node0; correctly rejected by node1")

        # 3. Send another block that builds on genesis.
        block_h1f = create_block(int("0x" + self.nodes[0].getblockhash(0), 0), create_coinbase(1), block_time)
        block_time += 1
        block_h1f.solve()
        test_node.send_message(msg_block(block_h1f))

        test_node.sync_with_ping()
        tip_entry_found = False
        for x in self.nodes[0].getchaintips():
            if x['hash'] == block_h1f.hash:
                assert_equal(x['status'], "headers-only")
                tip_entry_found = True
        assert(tip_entry_found)
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[0].getblock, block_h1f.hash)

        # 4. Send another two block that build on the fork.
        block_h2f = create_block(block_h1f.sha256, create_coinbase(2), block_time)
        block_time += 1
        block_h2f.solve()
        test_node.send_message(msg_block(block_h2f))

        test_node.sync_with_ping()
        # Since the earlier block was not processed by node, the new block
        # can't be fully validated.
        tip_entry_found = False
        for x in self.nodes[0].getchaintips():
            if x['hash'] == block_h2f.hash:
                assert_equal(x['status'], "headers-only")
                tip_entry_found = True
        assert(tip_entry_found)

        # But this block should be accepted by node since it has equal work.
        self.nodes[0].getblock(block_h2f.hash)
        self.log.info("Second height 2 block accepted, but not reorg'ed to")

        # 4b. Now send another block that builds on the forking chain.
        block_h3 = create_block(block_h2f.sha256, create_coinbase(3), block_h2f.nTime+1)
        block_h3.solve()
        test_node.send_message(msg_block(block_h3))

        test_node.sync_with_ping()
        # Since the earlier block was not processed by node, the new block
        # can't be fully validated.
        tip_entry_found = False
        for x in self.nodes[0].getchaintips():
            if x['hash'] == block_h3.hash:
                assert_equal(x['status'], "headers-only")
                tip_entry_found = True
        assert(tip_entry_found)
        self.nodes[0].getblock(block_h3.hash)

        # But this block should be accepted by node since it has more work.
        self.nodes[0].getblock(block_h3.hash)
        self.log.info("Unrequested more-work block accepted")

        # 4c. Now mine 288 more blocks and deliver; all should be processed but
        # the last (height-too-high) on node (as long as it is not missing any headers)
        tip = block_h3
        all_blocks = []
        for i in range(288):
            next_block = create_block(tip.sha256, create_coinbase(i + 4), tip.nTime+1)
            next_block.solve()
            all_blocks.append(next_block)
            tip = next_block

        # Now send the block at height 5 and check that it wasn't accepted (missing header)
        test_node.send_message(msg_block(all_blocks[1]))
        test_node.sync_with_ping()
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].getblock, all_blocks[1].hash)
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].getblockheader, all_blocks[1].hash)

        # The block at height 5 should be accepted if we provide the missing header, though
        headers_message = msg_headers()
        headers_message.headers.append(CBlockHeader(all_blocks[0]))
        test_node.send_message(headers_message)
        test_node.send_message(msg_block(all_blocks[1]))
        test_node.sync_with_ping()
        self.nodes[0].getblock(all_blocks[1].hash)

        # Now send the blocks in all_blocks
        for i in range(288):
            test_node.send_message(msg_block(all_blocks[i]))
        test_node.sync_with_ping()

        # Blocks 1-287 should be accepted, block 288 should be ignored because it's too far ahead
        for x in all_blocks[:-1]:
            self.nodes[0].getblock(x.hash)
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[0].getblock, all_blocks[-1].hash)

        # 5. Test handling of unrequested block on the node that didn't process
        # Should still not be processed (even though it has a child that has more
        # work).

        # The node should have requested the blocks at some point, so
        # disconnect/reconnect first

        self.nodes[0].disconnect_p2ps()
        self.nodes[1].disconnect_p2ps()

        test_node = self.nodes[0].add_p2p_connection(P2PInterface())

        test_node.send_message(msg_block(block_h1f))

        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 2)
        self.log.info("Unrequested block that would complete more-work chain was ignored")

        # 6. Try to get node to request the missing block.
        # Poke the node with an inv for block at height 3 and see if that
        # triggers a getdata on block 2 (it should if block 2 is missing).
        with mininode_lock:
            # Clear state so we can check the getdata request
            test_node.last_message.pop("getdata", None)
            test_node.send_message(msg_inv([CInv(2, block_h3.sha256)]))

        test_node.sync_with_ping()
        with mininode_lock:
            getdata = test_node.last_message["getdata"]

        # Check that the getdata includes the right block
        assert_equal(getdata.inv[0].hash, block_h1f.sha256)
        self.log.info("Inv at tip triggered getdata for unprocessed block")

        # 7. Send the missing block for the third time (now it is requested)
        test_node.send_message(msg_block(block_h1f))

        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 290)
        self.nodes[0].getblock(all_blocks[286].hash)
        assert_equal(self.nodes[0].getbestblockhash(), all_blocks[286].hash)
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[0].getblock, all_blocks[287].hash)
        self.log.info("Successfully reorged to longer chain from non-whitelisted peer")

        # 8. Create a chain which is invalid at a height longer than the
        # current chain, but which has more blocks on top of that
        block_289f = create_block(all_blocks[284].sha256, create_coinbase(289), all_blocks[284].nTime+1)
        block_289f.solve()
        block_290f = create_block(block_289f.sha256, create_coinbase(290), block_289f.nTime+1)
        block_290f.solve()
        block_291 = create_block(block_290f.sha256, create_coinbase(291), block_290f.nTime+1)
        # block_291 spends a coinbase below maturity!
        block_291.vtx.append(create_tx_with_script(block_290f.vtx[0], 0, script_sig=b"42", amount=1))
        block_291.hashMerkleRoot = block_291.calc_merkle_root()
        block_291.solve()
        block_292 = create_block(block_291.sha256, create_coinbase(292), block_291.nTime+1)
        block_292.solve()

        # Now send all the headers on the chain and enough blocks to trigger reorg
        headers_message = msg_headers()
        headers_message.headers.append(CBlockHeader(block_289f))
        headers_message.headers.append(CBlockHeader(block_290f))
        headers_message.headers.append(CBlockHeader(block_291))
        headers_message.headers.append(CBlockHeader(block_292))
        test_node.send_message(headers_message)

        test_node.sync_with_ping()
        tip_entry_found = False
        for x in self.nodes[0].getchaintips():
            if x['hash'] == block_292.hash:
                assert_equal(x['status'], "headers-only")
                tip_entry_found = True
        assert(tip_entry_found)
        assert_raises_rpc_error(-1, "Block not found on disk", self.nodes[0].getblock, block_292.hash)

        test_node.send_message(msg_block(block_289f))
        test_node.send_message(msg_block(block_290f))

        test_node.sync_with_ping()
        self.nodes[0].getblock(block_289f.hash)
        self.nodes[0].getblock(block_290f.hash)

        test_node.send_message(msg_block(block_291))

        # At this point we've sent an obviously-bogus block, wait for full processing
        # without assuming whether we will be disconnected or not
        try:
            # Only wait a short while so the test doesn't take forever if we do get
            # disconnected
            test_node.sync_with_ping(timeout=1)
        except AssertionError:
            test_node.wait_for_disconnect()

            self.nodes[0].disconnect_p2ps()
            test_node = self.nodes[0].add_p2p_connection(P2PInterface())

        # We should have failed reorg and switched back to 290 (but have block 291)
        assert_equal(self.nodes[0].getblockcount(), 290)
        assert_equal(self.nodes[0].getbestblockhash(), all_blocks[286].hash)
        assert_equal(self.nodes[0].getblock(block_291.hash)["confirmations"], -1)

        # Now send a new header on the invalid chain, indicating we're forked off, and expect to get disconnected
        block_293 = create_block(block_292.sha256, create_coinbase(293), block_292.nTime+1)
        block_293.solve()
        headers_message = msg_headers()
        headers_message.headers.append(CBlockHeader(block_293))
        test_node.send_message(headers_message)
        test_node.wait_for_disconnect()

        # 9. Connect node1 to node0 and ensure it is able to sync
        connect_nodes(self.nodes[0], 1)
        self.sync_blocks([self.nodes[0], self.nodes[1]])
        self.log.info("Successfully synced nodes 1 and 0")

if __name__ == '__main__':
    AcceptBlockTest().main()
