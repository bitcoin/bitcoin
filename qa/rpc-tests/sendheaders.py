#!/usr/bin/env python2
#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase

'''
SendHeadersTest -- test behavior of headers messages to announce blocks.

Setup: 

- Two nodes, two p2p connections to node0. One p2p connection should only ever
  receive inv's (omitted from testing description below, this is our control).
  Second node is used for creating reorgs.

Part 1: No headers announcements before "sendheaders"
a. node mines a block [expect: inv]
   send getdata for the block [expect: block]
b. node mines another block [expect: inv]
   send getheaders and getdata [expect: headers, then block]
c. node mines another block [expect: inv]
   peer mines a block, announces with header [expect: getdata]
d. node mines another block [expect: inv]

Part 2: After "sendheaders", headers announcements should generally work.
a. peer sends sendheaders [expect: no response]
   peer sends getheaders with current tip [expect: no response]
b. node mines a block [expect: tip header]
c. for N in 1, ..., 10:
   * for announce-type in {inv, header}
     - peer mines N blocks, announces with announce-type
       [ expect: getheaders/getdata or getdata, deliver block(s) ]
     - node mines a block [ expect: 1 header ]

Part 3: Headers announcements stop after large reorg and resume after getheaders or inv from peer.
- For response-type in {inv, getheaders}
  * node mines a 7 block reorg [ expect: headers announcement of 8 blocks ]
  * node mines an 8-block reorg [ expect: inv at tip ]
  * peer responds with getblocks/getdata [expect: inv, blocks ]
  * node mines another block [ expect: inv at tip, peer sends getdata, expect: block ]
  * node mines another block at tip [ expect: inv ]
  * peer responds with getheaders with an old hashstop more than 8 blocks back [expect: headers]
  * peer requests block [ expect: block ]
  * node mines another block at tip [ expect: inv, peer sends getdata, expect: block ]
  * peer sends response-type [expect headers if getheaders, getheaders/getdata if mining new block]
  * node mines 1 block [expect: 1 header, peer responds with getdata]

Part 4: Test direct fetch behavior
a. Announce 2 old block headers.
   Expect: no getdata requests.
b. Announce 3 new blocks via 1 headers message.
   Expect: one getdata request for all 3 blocks.
   (Send blocks.)
c. Announce 1 header that forks off the last two blocks.
   Expect: no response.
d. Announce 1 more header that builds on that fork.
   Expect: one getdata request for two blocks.
e. Announce 16 more headers that build on that fork.
   Expect: getdata request for 14 more blocks.
f. Announce 1 more header that builds on that fork.
   Expect: no response.
'''

class BaseNode(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.last_inv = None
        self.last_headers = None
        self.last_block = None
        self.ping_counter = 1
        self.last_pong = msg_pong(0)
        self.last_getdata = None
        self.sleep_time = 0.05
        self.block_announced = False

    def clear_last_announcement(self):
        with mininode_lock:
            self.block_announced = False
            self.last_inv = None
            self.last_headers = None

    def add_connection(self, conn):
        self.connection = conn

    # Request data for a list of block hashes
    def get_data(self, block_hashes):
        msg = msg_getdata()
        for x in block_hashes:
            msg.inv.append(CInv(2, x))
        self.connection.send_message(msg)

    def get_headers(self, locator, hashstop):
        msg = msg_getheaders()
        msg.locator.vHave = locator
        msg.hashstop = hashstop
        self.connection.send_message(msg)

    def send_block_inv(self, blockhash):
        msg = msg_inv()
        msg.inv = [CInv(2, blockhash)]
        self.connection.send_message(msg)

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def on_inv(self, conn, message):
        self.last_inv = message
        self.block_announced = True

    def on_headers(self, conn, message):
        self.last_headers = message
        self.block_announced = True

    def on_block(self, conn, message):
        self.last_block = message.block
        self.last_block.calc_sha256()

    def on_getdata(self, conn, message):
        self.last_getdata = message

    def on_pong(self, conn, message):
        self.last_pong = message

    # Test whether the last announcement we received had the
    # right header or the right inv
    # inv and headers should be lists of block hashes
    def check_last_announcement(self, headers=None, inv=None):
        expect_headers = headers if headers != None else []
        expect_inv = inv if inv != None else []
        test_function = lambda: self.block_announced
        self.sync(test_function)
        with mininode_lock:
            self.block_announced = False

            success = True
            compare_inv = []
            if self.last_inv != None:
                compare_inv = [x.hash for x in self.last_inv.inv]
            if compare_inv != expect_inv:
                success = False

            hash_headers = []
            if self.last_headers != None:
                # treat headers as a list of block hashes
                hash_headers = [ x.sha256 for x in self.last_headers.headers ]
            if hash_headers != expect_headers:
                success = False

            self.last_inv = None
            self.last_headers = None
        return success

    # Syncing helpers
    def sync(self, test_function, timeout=60):
        while timeout > 0:
            with mininode_lock:
                if test_function():
                    return
            time.sleep(self.sleep_time)
            timeout -= self.sleep_time
        raise AssertionError("Sync failed to complete")
        
    def sync_with_ping(self, timeout=60):
        self.send_message(msg_ping(nonce=self.ping_counter))
        test_function = lambda: self.last_pong.nonce == self.ping_counter
        self.sync(test_function, timeout)
        self.ping_counter += 1
        return

    def wait_for_block(self, blockhash, timeout=60):
        test_function = lambda: self.last_block != None and self.last_block.sha256 == blockhash
        self.sync(test_function, timeout)
        return

    def wait_for_getdata(self, hash_list, timeout=60):
        if hash_list == []:
            return

        test_function = lambda: self.last_getdata != None and [x.hash for x in self.last_getdata.inv] == hash_list
        self.sync(test_function, timeout)
        return

    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [ CBlockHeader(b) for b in new_blocks ]
        self.send_message(headers_message)

    def send_getblocks(self, locator):
        getblocks_message = msg_getblocks()
        getblocks_message.locator.vHave = locator
        self.send_message(getblocks_message)

# InvNode: This peer should only ever receive inv's, because it doesn't ever send a
# "sendheaders" message.
class InvNode(BaseNode):
    def __init__(self):
        BaseNode.__init__(self)

# TestNode: This peer is the one we use for most of the testing.
class TestNode(BaseNode):
    def __init__(self):
        BaseNode.__init__(self)

class SendHeadersTest(BitcoinTestFramework):
    def setup_chain(self):
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self):
        self.nodes = []
        self.nodes = start_nodes(2, self.options.tmpdir, [["-debug", "-logtimemicros=1"]]*2)
        connect_nodes(self.nodes[0], 1)

    # mine count blocks and return the new tip
    def mine_blocks(self, count):
        # Clear out last block announcement from each p2p listener
        [ x.clear_last_announcement() for x in self.p2p_connections ]
        self.nodes[0].generate(count)
        return int(self.nodes[0].getbestblockhash(), 16)

    # mine a reorg that invalidates length blocks (replacing them with
    # length+1 blocks).
    # Note: we clear the state of our p2p connections after the
    # to-be-reorged-out blocks are mined, so that we don't break later tests.
    # return the list of block hashes newly mined
    def mine_reorg(self, length):
        self.nodes[0].generate(length) # make sure all invalidated blocks are node0's
        sync_blocks(self.nodes, wait=0.1)
        [x.clear_last_announcement() for x in self.p2p_connections]

        tip_height = self.nodes[1].getblockcount()
        hash_to_invalidate = self.nodes[1].getblockhash(tip_height-(length-1))
        self.nodes[1].invalidateblock(hash_to_invalidate)
        all_hashes = self.nodes[1].generate(length+1) # Must be longer than the orig chain
        sync_blocks(self.nodes, wait=0.1)
        return [int(x, 16) for x in all_hashes]

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        inv_node = InvNode()
        test_node = TestNode()

        self.p2p_connections = [inv_node, test_node]

        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], inv_node))
        # Set nServices to 0 for test_node, so no block download will occur outside of
        # direct fetching
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], test_node, services=0))
        inv_node.add_connection(connections[0])
        test_node.add_connection(connections[1])

        NetworkThread().start() # Start up network handling in another thread

        # Test logic begins here
        inv_node.wait_for_verack()
        test_node.wait_for_verack()

        tip = int(self.nodes[0].getbestblockhash(), 16)

        # PART 1
        # 1. Mine a block; expect inv announcements each time
        print "Part 1: headers don't start before sendheaders message..."
        for i in xrange(4):
            old_tip = tip
            tip = self.mine_blocks(1)
            assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
            assert_equal(test_node.check_last_announcement(inv=[tip]), True)
            # Try a few different responses; none should affect next announcement
            if i == 0:
                # first request the block
                test_node.get_data([tip])
                test_node.wait_for_block(tip, timeout=5)
            elif i == 1:
                # next try requesting header and block
                test_node.get_headers(locator=[old_tip], hashstop=tip)
                test_node.get_data([tip])
                test_node.wait_for_block(tip)
                test_node.clear_last_announcement() # since we requested headers...
            elif i == 2:
                # this time announce own block via headers
                height = self.nodes[0].getblockcount()
                last_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time']
                block_time = last_time + 1
                new_block = create_block(tip, create_coinbase(height+1), block_time)
                new_block.solve()
                test_node.send_header_for_blocks([new_block])
                test_node.wait_for_getdata([new_block.sha256], timeout=5)
                test_node.send_message(msg_block(new_block))
                test_node.sync_with_ping() # make sure this block is processed
                inv_node.clear_last_announcement()
                test_node.clear_last_announcement()

        print "Part 1: success!"
        print "Part 2: announce blocks with headers after sendheaders message..."
        # PART 2
        # 2. Send a sendheaders message and test that headers announcements
        # commence and keep working.
        test_node.send_message(msg_sendheaders())
        prev_tip = int(self.nodes[0].getbestblockhash(), 16)
        test_node.get_headers(locator=[prev_tip], hashstop=0L)
        test_node.sync_with_ping()

        # Now that we've synced headers, headers announcements should work
        tip = self.mine_blocks(1)
        assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
        assert_equal(test_node.check_last_announcement(headers=[tip]), True)

        height = self.nodes[0].getblockcount()+1
        block_time += 10  # Advance far enough ahead
        for i in xrange(10):
            # Mine i blocks, and alternate announcing either via
            # inv (of tip) or via headers. After each, new blocks
            # mined by the node should successfully be announced
            # with block header, even though the blocks are never requested
            for j in xrange(2):
                blocks = []
                for b in xrange(i+1):
                    blocks.append(create_block(tip, create_coinbase(height), block_time))
                    blocks[-1].solve()
                    tip = blocks[-1].sha256
                    block_time += 1
                    height += 1
                if j == 0:
                    # Announce via inv
                    test_node.send_block_inv(tip)
                    test_node.wait_for_getdata([tip], timeout=5)
                    # Test that duplicate inv's won't result in duplicate
                    # getdata requests, or duplicate headers announcements
                    inv_node.send_block_inv(tip)
                    # Should have received a getheaders as well!
                    test_node.send_header_for_blocks(blocks)
                    test_node.wait_for_getdata([x.sha256 for x in blocks[0:-1]], timeout=5)
                    [ inv_node.send_block_inv(x.sha256) for x in blocks[0:-1] ]
                    inv_node.sync_with_ping()
                else:
                    # Announce via headers
                    test_node.send_header_for_blocks(blocks)
                    test_node.wait_for_getdata([x.sha256 for x in blocks], timeout=5)
                    # Test that duplicate headers won't result in duplicate
                    # getdata requests (the check is further down)
                    inv_node.send_header_for_blocks(blocks)
                    inv_node.sync_with_ping()
                [ test_node.send_message(msg_block(x)) for x in blocks ]
                test_node.sync_with_ping()
                inv_node.sync_with_ping()
                # This block should not be announced to the inv node (since it also
                # broadcast it)
                assert_equal(inv_node.last_inv, None)
                assert_equal(inv_node.last_headers, None)
                tip = self.mine_blocks(1)
                assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
                assert_equal(test_node.check_last_announcement(headers=[tip]), True)
                height += 1
                block_time += 1

        print "Part 2: success!"

        print "Part 3: headers announcements can stop after large reorg, and resume after headers/inv from peer..."

        # PART 3.  Headers announcements can stop after large reorg, and resume after
        # getheaders or inv from peer.
        for j in xrange(2):
            # First try mining a reorg that can propagate with header announcement
            new_block_hashes = self.mine_reorg(length=7)
            tip = new_block_hashes[-1]
            assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
            assert_equal(test_node.check_last_announcement(headers=new_block_hashes), True)

            block_time += 8 

            # Mine a too-large reorg, which should be announced with a single inv
            new_block_hashes = self.mine_reorg(length=8)
            tip = new_block_hashes[-1]
            assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
            assert_equal(test_node.check_last_announcement(inv=[tip]), True)

            block_time += 9

            fork_point = self.nodes[0].getblock("%02x" % new_block_hashes[0])["previousblockhash"]
            fork_point = int(fork_point, 16)

            # Use getblocks/getdata
            test_node.send_getblocks(locator = [fork_point])
            assert_equal(test_node.check_last_announcement(inv=new_block_hashes), True)
            test_node.get_data(new_block_hashes)
            test_node.wait_for_block(new_block_hashes[-1])

            for i in xrange(3):
                # Mine another block, still should get only an inv
                tip = self.mine_blocks(1)
                assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
                assert_equal(test_node.check_last_announcement(inv=[tip]), True)
                if i == 0:
                    # Just get the data -- shouldn't cause headers announcements to resume
                    test_node.get_data([tip])
                    test_node.wait_for_block(tip)
                elif i == 1:
                    # Send a getheaders message that shouldn't trigger headers announcements
                    # to resume (best header sent will be too old)
                    test_node.get_headers(locator=[fork_point], hashstop=new_block_hashes[1])
                    test_node.get_data([tip])
                    test_node.wait_for_block(tip)
                elif i == 2:
                    test_node.get_data([tip])
                    test_node.wait_for_block(tip)
                    # This time, try sending either a getheaders to trigger resumption
                    # of headers announcements, or mine a new block and inv it, also 
                    # triggering resumption of headers announcements.
                    if j == 0:
                        test_node.get_headers(locator=[tip], hashstop=0L)
                        test_node.sync_with_ping()
                    else:
                        test_node.send_block_inv(tip)
                        test_node.sync_with_ping()
            # New blocks should now be announced with header
            tip = self.mine_blocks(1)
            assert_equal(inv_node.check_last_announcement(inv=[tip]), True)
            assert_equal(test_node.check_last_announcement(headers=[tip]), True)

        print "Part 3: success!"

        print "Part 4: Testing direct fetch behavior..."
        tip = self.mine_blocks(1)
        height = self.nodes[0].getblockcount() + 1
        last_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time']
        block_time = last_time + 1

        # Create 2 blocks.  Send the blocks, then send the headers.
        blocks = []
        for b in xrange(2):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1
            inv_node.send_message(msg_block(blocks[-1]))

        inv_node.sync_with_ping() # Make sure blocks are processed
        test_node.last_getdata = None
        test_node.send_header_for_blocks(blocks)
        test_node.sync_with_ping()
        # should not have received any getdata messages
        with mininode_lock:
            assert_equal(test_node.last_getdata, None)

        # This time, direct fetch should work
        blocks = []
        for b in xrange(3):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1

        test_node.send_header_for_blocks(blocks)
        test_node.sync_with_ping()
        test_node.wait_for_getdata([x.sha256 for x in blocks], timeout=test_node.sleep_time)

        [ test_node.send_message(msg_block(x)) for x in blocks ]

        test_node.sync_with_ping()

        # Now announce a header that forks the last two blocks
        tip = blocks[0].sha256
        height -= 1
        blocks = []

        # Create extra blocks for later
        for b in xrange(20):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1

        # Announcing one block on fork should not trigger direct fetch
        # (less work than tip)
        test_node.last_getdata = None
        test_node.send_header_for_blocks(blocks[0:1])
        test_node.sync_with_ping()
        with mininode_lock:
            assert_equal(test_node.last_getdata, None)

        # Announcing one more block on fork should trigger direct fetch for
        # both blocks (same work as tip)
        test_node.send_header_for_blocks(blocks[1:2])
        test_node.sync_with_ping()
        test_node.wait_for_getdata([x.sha256 for x in blocks[0:2]], timeout=test_node.sleep_time)

        # Announcing 16 more headers should trigger direct fetch for 14 more
        # blocks
        test_node.send_header_for_blocks(blocks[2:18])
        test_node.sync_with_ping()
        test_node.wait_for_getdata([x.sha256 for x in blocks[2:16]], timeout=test_node.sleep_time)

        # Announcing 1 more header should not trigger any response
        test_node.last_getdata = None
        test_node.send_header_for_blocks(blocks[18:19])
        test_node.sync_with_ping()
        with mininode_lock:
            assert_equal(test_node.last_getdata, None)

        print "Part 4: success!"

        # Finally, check that the inv node never received a getdata request,
        # throughout the test
        assert_equal(inv_node.last_getdata, None)

if __name__ == '__main__':
    SendHeadersTest().main()
