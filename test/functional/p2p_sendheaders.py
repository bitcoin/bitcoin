#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test behavior of headers messages to announce blocks.

Setup:

- Two nodes:
    - node0 is the node-under-test. We create two p2p connections to it. The
      first p2p connection is a control and should only ever receive inv's. The
      second p2p connection tests the headers sending logic.
    - node1 is used to create reorgs.

test_null_locators
==================

Sends two getheaders requests with null locator values. First request's hashstop
value refers to validated block, while second request's hashstop value refers to
a block which hasn't been validated. Verifies only the first request returns
headers.

test_nonnull_locators
=====================

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

Part 5: Test handling of headers that don't connect.
a. Repeat 10 times:
   1. Announce a header that doesn't connect.
      Expect: getheaders message
   2. Send headers chain.
      Expect: getdata for the missing blocks, tip update.
b. Then send 9 more headers that don't connect.
   Expect: getheaders message each time.
c. Announce a header that does connect.
   Expect: no response.
d. Announce 49 headers that don't connect.
   Expect: getheaders message each time.
e. Announce one more that doesn't connect.
   Expect: disconnect.
"""
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import CInv
from test_framework.p2p import (
    CBlockHeader,
    NODE_WITNESS,
    P2PInterface,
    p2p_lock,
    MSG_BLOCK,
    msg_block,
    msg_getblocks,
    msg_getdata,
    msg_getheaders,
    msg_headers,
    msg_inv,
    msg_sendheaders,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

DIRECT_FETCH_RESPONSE_TIME = 0.05

class BaseNode(P2PInterface):
    def __init__(self):
        super().__init__()

        self.block_announced = False
        self.last_blockhash_announced = None
        self.recent_headers_announced = []

    def send_get_data(self, block_hashes):
        """Request data for a list of block hashes."""
        msg = msg_getdata()
        for x in block_hashes:
            msg.inv.append(CInv(MSG_BLOCK, x))
        self.send_message(msg)

    def send_get_headers(self, locator, hashstop):
        msg = msg_getheaders()
        msg.locator.vHave = locator
        msg.hashstop = hashstop
        self.send_message(msg)

    def send_block_inv(self, blockhash):
        msg = msg_inv()
        msg.inv = [CInv(MSG_BLOCK, blockhash)]
        self.send_message(msg)

    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_message(headers_message)

    def send_getblocks(self, locator):
        getblocks_message = msg_getblocks()
        getblocks_message.locator.vHave = locator
        self.send_message(getblocks_message)

    def wait_for_block_announcement(self, block_hash, timeout=60):
        test_function = lambda: self.last_blockhash_announced == block_hash
        self.wait_until(test_function, timeout=timeout)

    def on_inv(self, message):
        self.block_announced = True
        self.last_blockhash_announced = message.inv[-1].hash

    def on_headers(self, message):
        if len(message.headers):
            self.block_announced = True
            for x in message.headers:
                x.calc_sha256()
                # append because headers may be announced over multiple messages.
                self.recent_headers_announced.append(x.sha256)
            self.last_blockhash_announced = message.headers[-1].sha256

    def clear_block_announcements(self):
        with p2p_lock:
            self.block_announced = False
            self.last_message.pop("inv", None)
            self.last_message.pop("headers", None)
            self.recent_headers_announced = []


    def check_last_headers_announcement(self, headers):
        """Test whether the last headers announcements received are right.
           Headers may be announced across more than one message."""
        test_function = lambda: (len(self.recent_headers_announced) >= len(headers))
        self.wait_until(test_function)
        with p2p_lock:
            assert_equal(self.recent_headers_announced, headers)
            self.block_announced = False
            self.last_message.pop("headers", None)
            self.recent_headers_announced = []

    def check_last_inv_announcement(self, inv):
        """Test whether the last announcement received had the right inv.
        inv should be a list of block hashes."""

        test_function = lambda: self.block_announced
        self.wait_until(test_function)

        with p2p_lock:
            compare_inv = []
            if "inv" in self.last_message:
                compare_inv = [x.hash for x in self.last_message["inv"].inv]
            assert_equal(compare_inv, inv)
            self.block_announced = False
            self.last_message.pop("inv", None)

class SendHeadersTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def mine_blocks(self, count):
        """Mine count blocks and return the new tip."""

        # Clear out block announcements from each p2p listener
        [x.clear_block_announcements() for x in self.nodes[0].p2ps]
        self.generatetoaddress(self.nodes[0], count, self.nodes[0].get_deterministic_priv_key().address)
        return int(self.nodes[0].getbestblockhash(), 16)

    def mine_reorg(self, length):
        """Mine a reorg that invalidates length blocks (replacing them with # length+1 blocks).

        Note: we clear the state of our p2p connections after the
        to-be-reorged-out blocks are mined, so that we don't break later tests.
        return the list of block hashes newly mined."""

        # make sure all invalidated blocks are node0's
        self.generatetoaddress(self.nodes[0], length, self.nodes[0].get_deterministic_priv_key().address)
        for x in self.nodes[0].p2ps:
            x.wait_for_block_announcement(int(self.nodes[0].getbestblockhash(), 16))
            x.clear_block_announcements()

        tip_height = self.nodes[1].getblockcount()
        hash_to_invalidate = self.nodes[1].getblockhash(tip_height - (length - 1))
        self.nodes[1].invalidateblock(hash_to_invalidate)
        all_hashes = self.generatetoaddress(self.nodes[1], length + 1, self.nodes[1].get_deterministic_priv_key().address)  # Must be longer than the orig chain
        return [int(x, 16) for x in all_hashes]

    def run_test(self):
        # Setup the p2p connections
        inv_node = self.nodes[0].add_p2p_connection(BaseNode())
        # Make sure NODE_NETWORK is not set for test_node, so no block download
        # will occur outside of direct fetching
        test_node = self.nodes[0].add_p2p_connection(BaseNode(), services=NODE_WITNESS)

        self.test_null_locators(test_node, inv_node)
        self.test_nonnull_locators(test_node, inv_node)

    def test_null_locators(self, test_node, inv_node):
        tip = self.nodes[0].getblockheader(self.generatetoaddress(self.nodes[0], 1, self.nodes[0].get_deterministic_priv_key().address)[0])
        tip_hash = int(tip["hash"], 16)

        inv_node.check_last_inv_announcement(inv=[tip_hash])
        test_node.check_last_inv_announcement(inv=[tip_hash])

        self.log.info("Verify getheaders with null locator and valid hashstop returns headers.")
        test_node.clear_block_announcements()
        test_node.send_get_headers(locator=[], hashstop=tip_hash)
        test_node.check_last_headers_announcement(headers=[tip_hash])

        self.log.info("Verify getheaders with null locator and invalid hashstop does not return headers.")
        block = create_block(int(tip["hash"], 16), create_coinbase(tip["height"] + 1), tip["mediantime"] + 1)
        block.solve()
        test_node.send_header_for_blocks([block])
        test_node.clear_block_announcements()
        test_node.send_get_headers(locator=[], hashstop=int(block.hash, 16))
        test_node.sync_with_ping()
        assert_equal(test_node.block_announced, False)
        inv_node.clear_block_announcements()
        test_node.send_message(msg_block(block))
        inv_node.check_last_inv_announcement(inv=[int(block.hash, 16)])

    def test_nonnull_locators(self, test_node, inv_node):
        tip = int(self.nodes[0].getbestblockhash(), 16)

        # PART 1
        # 1. Mine a block; expect inv announcements each time
        self.log.info("Part 1: headers don't start before sendheaders message...")
        for i in range(4):
            self.log.debug("Part 1.{}: starting...".format(i))
            old_tip = tip
            tip = self.mine_blocks(1)
            inv_node.check_last_inv_announcement(inv=[tip])
            test_node.check_last_inv_announcement(inv=[tip])
            # Try a few different responses; none should affect next announcement
            if i == 0:
                # first request the block
                test_node.send_get_data([tip])
                test_node.wait_for_block(tip)
            elif i == 1:
                # next try requesting header and block
                test_node.send_get_headers(locator=[old_tip], hashstop=tip)
                test_node.send_get_data([tip])
                test_node.wait_for_block(tip)
                test_node.clear_block_announcements()  # since we requested headers...
            elif i == 2:
                # this time announce own block via headers
                inv_node.clear_block_announcements()
                height = self.nodes[0].getblockcount()
                last_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time']
                block_time = last_time + 1
                new_block = create_block(tip, create_coinbase(height + 1), block_time)
                new_block.solve()
                test_node.send_header_for_blocks([new_block])
                test_node.wait_for_getdata([new_block.sha256])
                test_node.send_and_ping(msg_block(new_block))  # make sure this block is processed
                inv_node.wait_until(lambda: inv_node.block_announced)
                inv_node.clear_block_announcements()
                test_node.clear_block_announcements()

        self.log.info("Part 1: success!")
        self.log.info("Part 2: announce blocks with headers after sendheaders message...")
        # PART 2
        # 2. Send a sendheaders message and test that headers announcements
        # commence and keep working.
        test_node.send_message(msg_sendheaders())
        prev_tip = int(self.nodes[0].getbestblockhash(), 16)
        test_node.send_get_headers(locator=[prev_tip], hashstop=0)
        test_node.sync_with_ping()

        # Now that we've synced headers, headers announcements should work
        tip = self.mine_blocks(1)
        expected_hash = tip
        inv_node.check_last_inv_announcement(inv=[tip])
        test_node.check_last_headers_announcement(headers=[tip])

        height = self.nodes[0].getblockcount() + 1
        block_time += 10  # Advance far enough ahead
        for i in range(10):
            self.log.debug("Part 2.{}: starting...".format(i))
            # Mine i blocks, and alternate announcing either via
            # inv (of tip) or via headers. After each, new blocks
            # mined by the node should successfully be announced
            # with block header, even though the blocks are never requested
            for j in range(2):
                self.log.debug("Part 2.{}.{}: starting...".format(i, j))
                blocks = []
                for _ in range(i + 1):
                    blocks.append(create_block(tip, create_coinbase(height), block_time))
                    blocks[-1].solve()
                    tip = blocks[-1].sha256
                    block_time += 1
                    height += 1
                if j == 0:
                    # Announce via inv
                    test_node.send_block_inv(tip)
                    if i == 0:
                        test_node.wait_for_getheaders(block_hash=expected_hash)
                    else:
                        assert "getheaders" not in test_node.last_message
                    # Should have received a getheaders now
                    test_node.send_header_for_blocks(blocks)
                    # Test that duplicate inv's won't result in duplicate
                    # getdata requests, or duplicate headers announcements
                    [inv_node.send_block_inv(x.sha256) for x in blocks]
                    test_node.wait_for_getdata([x.sha256 for x in blocks])
                    inv_node.sync_with_ping()
                else:
                    # Announce via headers
                    test_node.send_header_for_blocks(blocks)
                    test_node.wait_for_getdata([x.sha256 for x in blocks])
                    # Test that duplicate headers won't result in duplicate
                    # getdata requests (the check is further down)
                    inv_node.send_header_for_blocks(blocks)
                    inv_node.sync_with_ping()
                [test_node.send_message(msg_block(x)) for x in blocks]
                test_node.sync_with_ping()
                inv_node.sync_with_ping()
                # This block should not be announced to the inv node (since it also
                # broadcast it)
                assert "inv" not in inv_node.last_message
                assert "headers" not in inv_node.last_message
                tip = self.mine_blocks(1)
                inv_node.check_last_inv_announcement(inv=[tip])
                test_node.check_last_headers_announcement(headers=[tip])
                height += 1
                block_time += 1

        self.log.info("Part 2: success!")

        self.log.info("Part 3: headers announcements can stop after large reorg, and resume after headers/inv from peer...")

        # PART 3.  Headers announcements can stop after large reorg, and resume after
        # getheaders or inv from peer.
        for j in range(2):
            self.log.debug("Part 3.{}: starting...".format(j))
            # First try mining a reorg that can propagate with header announcement
            new_block_hashes = self.mine_reorg(length=7)
            tip = new_block_hashes[-1]
            inv_node.check_last_inv_announcement(inv=[tip])
            test_node.check_last_headers_announcement(headers=new_block_hashes)

            block_time += 8

            # Mine a too-large reorg, which should be announced with a single inv
            new_block_hashes = self.mine_reorg(length=8)
            tip = new_block_hashes[-1]
            inv_node.check_last_inv_announcement(inv=[tip])
            test_node.check_last_inv_announcement(inv=[tip])

            block_time += 9

            fork_point = self.nodes[0].getblock("%064x" % new_block_hashes[0])["previousblockhash"]
            fork_point = int(fork_point, 16)

            # Use getblocks/getdata
            test_node.send_getblocks(locator=[fork_point])
            test_node.check_last_inv_announcement(inv=new_block_hashes)
            test_node.send_get_data(new_block_hashes)
            test_node.wait_for_block(new_block_hashes[-1])

            for i in range(3):
                self.log.debug("Part 3.{}.{}: starting...".format(j, i))

                # Mine another block, still should get only an inv
                tip = self.mine_blocks(1)
                inv_node.check_last_inv_announcement(inv=[tip])
                test_node.check_last_inv_announcement(inv=[tip])
                if i == 0:
                    # Just get the data -- shouldn't cause headers announcements to resume
                    test_node.send_get_data([tip])
                    test_node.wait_for_block(tip)
                elif i == 1:
                    # Send a getheaders message that shouldn't trigger headers announcements
                    # to resume (best header sent will be too old)
                    test_node.send_get_headers(locator=[fork_point], hashstop=new_block_hashes[1])
                    test_node.send_get_data([tip])
                    test_node.wait_for_block(tip)
                elif i == 2:
                    # This time, try sending either a getheaders to trigger resumption
                    # of headers announcements, or mine a new block and inv it, also
                    # triggering resumption of headers announcements.
                    test_node.send_get_data([tip])
                    test_node.wait_for_block(tip)
                    if j == 0:
                        test_node.send_get_headers(locator=[tip], hashstop=0)
                        test_node.sync_with_ping()
                    else:
                        test_node.send_block_inv(tip)
                        test_node.sync_with_ping()
            # New blocks should now be announced with header
            tip = self.mine_blocks(1)
            inv_node.check_last_inv_announcement(inv=[tip])
            test_node.check_last_headers_announcement(headers=[tip])

        self.log.info("Part 3: success!")

        self.log.info("Part 4: Testing direct fetch behavior...")
        tip = self.mine_blocks(1)
        height = self.nodes[0].getblockcount() + 1
        last_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time']
        block_time = last_time + 1

        # Create 2 blocks.  Send the blocks, then send the headers.
        blocks = []
        for _ in range(2):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1
            inv_node.send_message(msg_block(blocks[-1]))

        inv_node.sync_with_ping()  # Make sure blocks are processed
        test_node.last_message.pop("getdata", None)
        test_node.send_header_for_blocks(blocks)
        test_node.sync_with_ping()
        # should not have received any getdata messages
        with p2p_lock:
            assert "getdata" not in test_node.last_message

        # This time, direct fetch should work
        blocks = []
        for _ in range(3):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1

        test_node.send_header_for_blocks(blocks)
        test_node.sync_with_ping()
        test_node.wait_for_getdata([x.sha256 for x in blocks], timeout=DIRECT_FETCH_RESPONSE_TIME)

        [test_node.send_message(msg_block(x)) for x in blocks]

        test_node.sync_with_ping()

        # Now announce a header that forks the last two blocks
        tip = blocks[0].sha256
        height -= 2
        blocks = []

        # Create extra blocks for later
        for _ in range(20):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1

        # Announcing one block on fork should not trigger direct fetch
        # (less work than tip)
        test_node.last_message.pop("getdata", None)
        test_node.send_header_for_blocks(blocks[0:1])
        test_node.sync_with_ping()
        with p2p_lock:
            assert "getdata" not in test_node.last_message

        # Announcing one more block on fork should trigger direct fetch for
        # both blocks (same work as tip)
        test_node.send_header_for_blocks(blocks[1:2])
        test_node.sync_with_ping()
        test_node.wait_for_getdata([x.sha256 for x in blocks[0:2]], timeout=DIRECT_FETCH_RESPONSE_TIME)

        # Announcing 16 more headers should trigger direct fetch for 14 more
        # blocks
        test_node.send_header_for_blocks(blocks[2:18])
        test_node.sync_with_ping()
        test_node.wait_for_getdata([x.sha256 for x in blocks[2:16]], timeout=DIRECT_FETCH_RESPONSE_TIME)

        # Announcing 1 more header should not trigger any response
        test_node.last_message.pop("getdata", None)
        test_node.send_header_for_blocks(blocks[18:19])
        test_node.sync_with_ping()
        with p2p_lock:
            assert "getdata" not in test_node.last_message

        self.log.info("Part 4: success!")

        # Now deliver all those blocks we announced.
        [test_node.send_message(msg_block(x)) for x in blocks]

        self.log.info("Part 5: Testing handling of unconnecting headers")
        # First we test that receipt of an unconnecting header doesn't prevent
        # chain sync.
        expected_hash = tip
        for i in range(10):
            self.log.debug("Part 5.{}: starting...".format(i))
            test_node.last_message.pop("getdata", None)
            blocks = []
            # Create two more blocks.
            for _ in range(2):
                blocks.append(create_block(tip, create_coinbase(height), block_time))
                blocks[-1].solve()
                tip = blocks[-1].sha256
                block_time += 1
                height += 1
            # Send the header of the second block -> this won't connect.
            test_node.send_header_for_blocks([blocks[1]])
            test_node.wait_for_getheaders(block_hash=expected_hash)
            test_node.send_header_for_blocks(blocks)
            test_node.wait_for_getdata([x.sha256 for x in blocks])
            [test_node.send_message(msg_block(x)) for x in blocks]
            test_node.sync_with_ping()
            assert_equal(int(self.nodes[0].getbestblockhash(), 16), blocks[1].sha256)
            expected_hash = blocks[1].sha256

        blocks = []
        # Now we test that if we repeatedly don't send connecting headers, we
        # don't go into an infinite loop trying to get them to connect.
        MAX_NUM_UNCONNECTING_HEADERS_MSGS = 10
        for _ in range(MAX_NUM_UNCONNECTING_HEADERS_MSGS + 1):
            blocks.append(create_block(tip, create_coinbase(height), block_time))
            blocks[-1].solve()
            tip = blocks[-1].sha256
            block_time += 1
            height += 1

        for i in range(1, MAX_NUM_UNCONNECTING_HEADERS_MSGS):
            # Send a header that doesn't connect, check that we get a getheaders.
            test_node.send_header_for_blocks([blocks[i]])
            test_node.wait_for_getheaders(block_hash=expected_hash)

        # Next header will connect, should re-set our count:
        test_node.send_header_for_blocks([blocks[0]])
        expected_hash = blocks[0].sha256

        # Remove the first two entries (blocks[1] would connect):
        blocks = blocks[2:]

        # Now try to see how many unconnecting headers we can send
        # before we get disconnected.  Should be 5*MAX_NUM_UNCONNECTING_HEADERS_MSGS
        for i in range(5 * MAX_NUM_UNCONNECTING_HEADERS_MSGS - 1):
            # Send a header that doesn't connect, check that we get a getheaders.
            test_node.send_header_for_blocks([blocks[i % len(blocks)]])
            test_node.wait_for_getheaders(block_hash=expected_hash)

        # Eventually this stops working.
        test_node.send_header_for_blocks([blocks[-1]])

        # Should get disconnected
        test_node.wait_for_disconnect()

        self.log.info("Part 5: success!")

        # Finally, check that the inv node never received a getdata request,
        # throughout the test
        assert "getdata" not in inv_node.last_message

if __name__ == '__main__':
    SendHeadersTest().main()
