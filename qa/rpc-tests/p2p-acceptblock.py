#!/usr/bin/env python2
#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import time
from test_framework.blocktools import create_block, create_coinbase

'''
AcceptBlockTest -- test processing of unrequested blocks.

Since behavior differs when receiving unrequested blocks from whitelisted peers
versus non-whitelisted peers, this tests the behavior of both (effectively two
separate tests running in parallel).

Setup: two nodes, node0 and node1, not connected to each other.  Node0 does not
whitelist localhost, but node1 does. They will each be on their own chain for
this test.

We have one NodeConn connection to each, test_node and white_node respectively.

The test:
1. Generate one block on each node, to leave IBD.

2. Mine a new block on each tip, and deliver to each node from node's peer.
   The tip should advance.

3. Mine a block that forks the previous block, and deliver to each node from
   corresponding peer.
   Node0 should not process this block (just accept the header), because it is
   unrequested and doesn't have more work than the tip.
   Node1 should process because this is coming from a whitelisted peer.

4. Send another block that builds on the forking block.
   Node0 should process this block but be stuck on the shorter chain, because
   it's missing an intermediate block.
   Node1 should reorg to this longer chain.

4b.Send 288 more blocks on the longer chain.
   Node0 should process all but the last block (too far ahead in height).
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
'''

# TestNode: bare-bones "peer".  Used mostly as a conduit for a test to sending
# p2p messages to a node, generating the messages in the main testing logic.
class TestNode(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.ping_counter = 1
        self.last_pong = msg_pong()

    def add_connection(self, conn):
        self.connection = conn

    # Track the last getdata message we receive (used in the test)
    def on_getdata(self, conn, message):
        self.last_getdata = message

    # Spin until verack message is received from the node.
    # We use this to signal that our test can begin. This
    # is called from the testing thread, so it needs to acquire
    # the global lock.
    def wait_for_verack(self):
        while True:
            with mininode_lock:
                if self.verack_received:
                    return
            time.sleep(0.05)

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def on_pong(self, conn, message):
        self.last_pong = message

    # Sync up with the node after delivery of a block
    def sync_with_ping(self, timeout=30):
        self.connection.send_message(msg_ping(nonce=self.ping_counter))
        received_pong = False
        sleep_time = 0.05
        while not received_pong and timeout > 0:
            time.sleep(sleep_time)
            timeout -= sleep_time
            with mininode_lock:
                if self.last_pong.nonce == self.ping_counter:
                    received_pong = True
        self.ping_counter += 1
        return received_pong


class AcceptBlockTest(BitcoinTestFramework):
    def add_options(self, parser):
        parser.add_option("--testbinary", dest="testbinary",
                          default=os.getenv("DASHD", "dashd"),
                          help="bitcoind binary to test")

    def setup_chain(self):
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self):
        # Node0 will be used to test behavior of processing unrequested blocks
        # from peers which are not whitelisted, while Node1 will be used for
        # the whitelisted case.
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"],
                                     binary=self.options.testbinary))
        self.nodes.append(start_node(1, self.options.tmpdir,
                                     ["-debug", "-whitelist=127.0.0.1"],
                                     binary=self.options.testbinary))

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        test_node = TestNode()   # connects to node0 (not whitelisted)
        white_node = TestNode()  # connects to node1 (whitelisted)

        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_node))
        connections.append(NodeConn('127.0.0.1', p2p_port(1), self.nodes[1], white_node))
        test_node.add_connection(connections[0])
        white_node.add_connection(connections[1])

        NetworkThread().start() # Start up network handling in another thread

        # Test logic begins here
        test_node.wait_for_verack()
        white_node.wait_for_verack()

        # 1. Have both nodes mine a block (leave IBD)
        [ n.generate(1) for n in self.nodes ]
        tips = [ int ("0x" + n.getbestblockhash() + "L", 0) for n in self.nodes ]

        # 2. Send one block that builds on each tip.
        # This should be accepted.
        blocks_h2 = []  # the height 2 blocks on each node's chain
        block_time = int(time.time()) + 1
        for i in xrange(2):
            blocks_h2.append(create_block(tips[i], create_coinbase(2), block_time))
            blocks_h2[i].solve()
            block_time += 1
        test_node.send_message(msg_block(blocks_h2[0]))
        white_node.send_message(msg_block(blocks_h2[1]))

        [ x.sync_with_ping() for x in [test_node, white_node] ]
        assert_equal(self.nodes[0].getblockcount(), 2)
        assert_equal(self.nodes[1].getblockcount(), 2)
        print "First height 2 block accepted by both nodes"

        # 3. Send another block that builds on the original tip.
        blocks_h2f = []  # Blocks at height 2 that fork off the main chain
        for i in xrange(2):
            blocks_h2f.append(create_block(tips[i], create_coinbase(2), blocks_h2[i].nTime+1))
            blocks_h2f[i].solve()
        test_node.send_message(msg_block(blocks_h2f[0]))
        white_node.send_message(msg_block(blocks_h2f[1]))

        [ x.sync_with_ping() for x in [test_node, white_node] ]
        for x in self.nodes[0].getchaintips():
            if x['hash'] == blocks_h2f[0].hash:
                assert_equal(x['status'], "headers-only")

        for x in self.nodes[1].getchaintips():
            if x['hash'] == blocks_h2f[1].hash:
                assert_equal(x['status'], "valid-headers")

        print "Second height 2 block accepted only from whitelisted peer"

        # 4. Now send another block that builds on the forking chain.
        blocks_h3 = []
        for i in xrange(2):
            blocks_h3.append(create_block(blocks_h2f[i].sha256, create_coinbase(3), blocks_h2f[i].nTime+1))
            blocks_h3[i].solve()
        test_node.send_message(msg_block(blocks_h3[0]))
        white_node.send_message(msg_block(blocks_h3[1]))

        [ x.sync_with_ping() for x in [test_node, white_node] ]
        # Since the earlier block was not processed by node0, the new block
        # can't be fully validated.
        for x in self.nodes[0].getchaintips():
            if x['hash'] == blocks_h3[0].hash:
                assert_equal(x['status'], "headers-only")

        # But this block should be accepted by node0 since it has more work.
        try:
            self.nodes[0].getblock(blocks_h3[0].hash)
            print "Unrequested more-work block accepted from non-whitelisted peer"
        except:
            raise AssertionError("Unrequested more work block was not processed")

        # Node1 should have accepted and reorged.
        assert_equal(self.nodes[1].getblockcount(), 3)
        print "Successfully reorged to length 3 chain from whitelisted peer"

        # 4b. Now mine 288 more blocks and deliver; all should be processed but
        # the last (height-too-high) on node0.  Node1 should process the tip if
        # we give it the headers chain leading to the tip.
        tips = blocks_h3
        headers_message = msg_headers()
        all_blocks = []   # node0's blocks
        for j in xrange(2):
            for i in xrange(288):
                next_block = create_block(tips[j].sha256, create_coinbase(i + 4), tips[j].nTime+1)
                next_block.solve()
                if j==0:
                    test_node.send_message(msg_block(next_block))
                    all_blocks.append(next_block)
                else:
                    headers_message.headers.append(CBlockHeader(next_block))
                tips[j] = next_block

        time.sleep(2)
        for x in all_blocks:
            try:
                self.nodes[0].getblock(x.hash)
                if x == all_blocks[287]:
                    raise AssertionError("Unrequested block too far-ahead should have been ignored")
            except:
                if x == all_blocks[287]:
                    print "Unrequested block too far-ahead not processed"
                else:
                    raise AssertionError("Unrequested block with more work should have been accepted")

        headers_message.headers.pop() # Ensure the last block is unrequested
        white_node.send_message(headers_message) # Send headers leading to tip
        white_node.send_message(msg_block(tips[1]))  # Now deliver the tip
        try:
            white_node.sync_with_ping()
            self.nodes[1].getblock(tips[1].hash)
            print "Unrequested block far ahead of tip accepted from whitelisted peer"
        except:
            raise AssertionError("Unrequested block from whitelisted peer not accepted")

        # 5. Test handling of unrequested block on the node that didn't process
        # Should still not be processed (even though it has a child that has more
        # work).
        test_node.send_message(msg_block(blocks_h2f[0]))

        # Here, if the sleep is too short, the test could falsely succeed (if the
        # node hasn't processed the block by the time the sleep returns, and then
        # the node processes it and incorrectly advances the tip).
        # But this would be caught later on, when we verify that an inv triggers
        # a getdata request for this block.
        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 2)
        print "Unrequested block that would complete more-work chain was ignored"

        # 6. Try to get node to request the missing block.
        # Poke the node with an inv for block at height 3 and see if that
        # triggers a getdata on block 2 (it should if block 2 is missing).
        with mininode_lock:
            # Clear state so we can check the getdata request
            test_node.last_getdata = None
            test_node.send_message(msg_inv([CInv(2, blocks_h3[0].sha256)]))

        test_node.sync_with_ping()
        with mininode_lock:
            getdata = test_node.last_getdata

        # Check that the getdata includes the right block
        assert_equal(getdata.inv[0].hash, blocks_h2f[0].sha256)
        print "Inv at tip triggered getdata for unprocessed block"

        # 7. Send the missing block for the third time (now it is requested)
        test_node.send_message(msg_block(blocks_h2f[0]))

        test_node.sync_with_ping()
        assert_equal(self.nodes[0].getblockcount(), 290)
        print "Successfully reorged to longer chain from non-whitelisted peer"

        [ c.disconnect_node() for c in connections ]

if __name__ == '__main__':
    AcceptBlockTest().main()
