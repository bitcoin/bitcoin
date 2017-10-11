#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test processing of unrequested blocks.

We create one NodeConn connection to test_node.

The test:
1. Generate one block, to leave IBD.

2. Mine a new block on the tip, and deliver from node's peer.
   The tip should advance.

3. Mine a block that forks from the genesis block, and deliver to test_node.
   Node should not process this block (just accept the header), because it is
   unrequested and doesn't have more or equal work to the tip.

4. Send another two blocks that builds on the forking block.
   Node should process the second block but be stuck on the shorter chain,
   because it's missing an intermediate block.

4c.Send 288 more blocks on the longer chain.
   Node should process all but the last block (too far ahead in height).
   Send all headers to Node1, and then send the last block in that chain.
   Node1 should accept the block because it's coming from a whitelisted peer.

5. Send a duplicate of the block in #3 to Node0.
   Node0 should not process the block because it is unrequested, and stay on
   the shorter chain.

6. Send Node0 an inv for the height 3 block produced in #4 above.
   Node0 should figure out that Node0 has the missing height 2 block and send a
   getdata.

7. Send Node0 the missing block again.
   Node0 should process and the tip should advance.
"""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import time
from test_framework.blocktools import create_block, create_coinbase

class AcceptBlockTest(BitcoinTestFramework):
    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("BITCOIND", "bitcoind"),
                          help="bitcoind binary to test")

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]

    def setup_network(self):
        # Node0 will be used to test behavior of processing unrequested blocks
        # from peers which are not whitelisted, while Node1 will be used for
        # the whitelisted case.
        self.setup_nodes()

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        test_node = NodeConnCB()   # connects to node (not whitelisted)

        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_node))
        test_node.add_connection(connections[0])

        NetworkThread().start() # Start up network handling in another thread

        # Test logic begins here
        test_node.wait_for_verack()

        # 1. Have nodes mine a block (leave IBD)
        self.nodes[0].generate(1)
        tip = int("0x" + self.nodes[0].getbestblockhash(), 0)

        # 2. Send one block that builds on the tip.
        # This should be accepted.
        block_time = int(time.time()) + 1
        block_h2 = create_block(tip, create_coinbase(2), block_time)
        block_h2.solve()
        block_time += 1
        test_node.send_message(msg_block(block_h2))

        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 2)
        self.log.info("First height 2 block accepted by node")

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

        # 4. Send another two block that build on the fork.
        block_h2f = []  # Blocks at height 2 that fork off the main chain
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

        # But this block should be accepted by node since it has more work.
        self.nodes[0].getblock(block_h3.hash)
        self.log.info("Unrequested more-work block accepted from non-whitelisted peer")

        # 4c. Now mine 288 more blocks and deliver; all should be processed but
        # the last (height-too-high) on node (as long as its not missing any headers)
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
        connections[0].disconnect_node()
        test_node.wait_for_disconnect()

        test_node = NodeConnCB()   # connects to node (not whitelisted)
        connections[0] = NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_node)
        test_node.add_connection(connections[0])

        NetworkThread().start() # Start up network handling in another thread

        test_node.wait_for_verack()
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
        self.log.info("Successfully reorged to longer chain from non-whitelisted peer")

        [ c.disconnect_node() for c in connections ]

if __name__ == '__main__':
    AcceptBlockTest().main()
