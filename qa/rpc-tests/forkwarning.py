#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase

'''
Test large work fork and large work invalid chain warning
'''

BASIC_WARNING = "This is a pre-release test build - use at your own risk - do not use for mining or merchant applications"
INVALID_WARNING = "Warning: Found invalid chain at least 6 blocks longer than our best chain! We do not appear to fully agree with our peers! Outgoing and incoming payment may not be reliable! See debug.log for details."
FORK_WARNING = "Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues. Outgoing and incoming payment may not be reliable! See debug.log for details."

class BaseNode(SingleNodeConnCB):
    def __init__(self):
        SingleNodeConnCB.__init__(self)
        self.last_inv = None
        self.last_headers = None
        self.last_block = None
        self.last_getdata = None
        self.block_announced = False
        self.last_getheaders = None
        self.disconnected = False
        self.last_blockhash_announced = None

    def on_getdata(self, conn, message):
        self.last_getdata = message

    def wait_for_getdata(self, hash_list, timeout=60):
        if hash_list == []:
            return

        test_function = lambda: self.last_getdata != None and [x.hash for x in self.last_getdata.inv] == hash_list
        assert(wait_until(test_function, timeout=timeout))
        return

    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [ CBlockHeader(b) for b in new_blocks ]
        self.send_message(headers_message)


# TestNode: This peer is the one we use for most of the testing.
class TestNode(BaseNode):
    def __init__(self):
        BaseNode.__init__(self)

class ForkWarningTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 1
        self.setup_clean_chain = True

    def setup_network(self):
        self.nodes = []
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        with open(self.alert_filename, 'w', encoding='utf8') as f:
            pass  # Just open then close to create zero-length file
        self.nodes.append(start_node(0, self.options.tmpdir,
                                     ["-alertnotify=echo %s >> \"" + self.alert_filename + "\""]))
        self.is_network_split = False

    # mine count blocks and return the new tip
    def mine_blocks(self, count, node = 0):
        # Clear out last block announcement from each p2p listener
        self.nodes[node].generate(count)
        return int(self.nodes[node].getbestblockhash(), 16)

    def run_test(self):
        self.test_node = TestNode()
        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], self.test_node, services=0))
        self.test_node.add_connection(connections[0])
        NetworkThread().start()
        self.test_node.wait_for_verack()

        # Generate 10 blocks
        tip = self.mine_blocks(10)
        height = self.nodes[0].getblockcount()
        last_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time']
        forkbase = [];
        forkbase.append(self.nodes[0].getblock(self.nodes[0].getbestblockhash())['hash'])
        block_time = last_time + 1
        height += 1

        print("Test 1: Invalid chain warning")
        # Generate 1 block with valid header but invalid nLockTime coinbase tx
        new_block = create_block(tip, create_coinbase(height), block_time)
        new_block.vtx[0].nLockTime = 100
        new_block.vtx[0].vin[0].nSequence = 0
        new_block.vtx[0].rehash()
        new_block.hashMerkleRoot = new_block.calc_merkle_root()
        new_block.rehash()
        new_block.solve()
        tip_invalid = new_block.sha256
        self.submit_header_and_block(new_block)
        assert_equal(self.nodes[0].getinfo()['blocks'], 10)

        # Build 10 blocks on top of the invalid block (Invalid block 21)
        for j in range(10):
            block_time += 1
            height += 1
            new_block = create_block(tip_invalid, create_coinbase(height), block_time)
            new_block.solve()
            tip_invalid = new_block.sha256
            self.submit_header(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], 10)
            warning = self.nodes[0].getinfo()['errors']
            if (j < 5):
                assert_equal(warning, BASIC_WARNING)
            else:
                assert_equal(warning, INVALID_WARNING)

        print("Test 2: Stop invalid chain warning")
        # Build 20 valid blocks on top of the best valid block (Block 11-30)
        height = self.nodes[0].getblockcount()
        for j in range(20):
            block_time += 1
            height += 1
            new_block = create_block(tip, create_coinbase(height), block_time)
            new_block.solve()
            tip = new_block.sha256
            self.submit_header_and_block(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], height)
            warning = self.nodes[0].getinfo()['errors']
            if (j < 4):
                assert_equal(warning, INVALID_WARNING)
            else:
                assert_equal(warning, BASIC_WARNING) # Warning stops when block 15 is found

        print("Test 3: Large-work fork warning (header only)")
        # Build 10 valid blocks on top of the block 20 to create a valid fork (Block 21a-30a)
        height = 20
        forkbase.append(self.nodes[0].getblockhash(height))
        tip = int(forkbase[1], 16)
        new_blocks = []
        for j in range(10):
            block_time += 1
            height += 1
            new_block = create_block(tip, create_coinbase(height), block_time)
            new_block.solve()
            new_blocks.append(new_block)
            tip = new_block.sha256
            self.submit_header(new_block)
            warning = self.nodes[0].getinfo()['errors']
            if (j >= 7):
                assert_equal(warning, FORK_WARNING) # Warning starts when block 28 is found
            else:
                assert_equal(warning, BASIC_WARNING)

        # Submit the blocks
        for b in new_blocks:
            self.submit_block(b)
            assert_equal(warning, FORK_WARNING)

        print("Test 4: Large-work fork warning (orphaning the original chain)")
        # Build 75 valid blocks on top of the block 30a to orphan the original chain (Block 31a-105a)
        for j in range(75): # To block 105
            block_time += 1
            height += 1
            new_block = create_block(tip, create_coinbase(height), block_time)
            new_block.solve()
            new_blocks.append(new_block)
            tip = new_block.sha256
            self.submit_header_and_block(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], height)
            warning = self.nodes[0].getinfo()['errors']
            if (j <= 70):
                assert_equal(warning, FORK_WARNING)
            else:
                assert_equal(warning, BASIC_WARNING) # Warning stops when block 103a is found (72 blocks longer)

        print("Test 5: Large-work fork warning (invalid block with header submitted only)")
        # Build 90 invalid blocks on top of the block 95 to create an invalid fork (Invalid block 96-185)
        new_blocks = []
        forkbase.append(self.nodes[0].getblockhash(95))
        height = 95
        tip_invalid = int(forkbase[2], 16)
        for j in range(90):
            block_time += 1
            height += 1
            new_block = create_block(tip_invalid, create_coinbase(height), block_time)
            new_block.vtx[0].nLockTime = 96
            new_block.vtx[0].vin[0].nSequence = 0
            new_block.vtx[0].rehash()
            new_block.hashMerkleRoot = new_block.calc_merkle_root()
            new_block.rehash()
            new_block.solve()
            new_blocks.append(new_block)
            tip_invalid = new_block.sha256
            self.submit_header(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], 105)
            warning = self.nodes[0].getinfo()['errors']
            if (j < 7):
                assert_equal(warning, BASIC_WARNING)
            else:
                # Warning starts when block 103 is found since it doesn't know the block is invalid
                assert_equal(warning, FORK_WARNING)

        # Submit the blocks. Now it knows the first block is invalid, but it doesn't show invalid chain warning because
        # only the first block is marked BLOCK_FAILED_VALID
        for b in new_blocks:
            self.submit_block(b)
            assert_equal(self.nodes[0].getinfo()['blocks'], 105)
            assert_equal(warning, FORK_WARNING)

        # Build to 185a to stop the warning
        height = self.nodes[0].getblockcount()
        for j in range(80):
            block_time += 1
            height += 1
            new_block = create_block(tip, create_coinbase(height), block_time)
            new_block.solve()
            tip = new_block.sha256
            self.submit_header_and_block(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], height)
            warning = self.nodes[0].getinfo()['errors']
            if (j < 71):
                assert_equal(warning, FORK_WARNING)
            else:
                assert_equal(warning, BASIC_WARNING)

        tip = self.mine_blocks(1080) # build to 1265a to activate BIP66
        block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time']
        print("Test 6: Invalid chain warning by negative version block")
        # Build a version -1 block on 1255a which is now invalid
        height = 1255
        forkbase.append(self.nodes[0].getblockhash(height))
        tip_invalid = int(forkbase[3], 16)
        block_time += 1
        height += 1
        new_block = create_block(tip_invalid, create_coinbase(height), block_time)
        new_block.nVersion = -1
        new_block.solve()
        tip_invalid = new_block.sha256
        self.submit_header(new_block)
        assert_equal(self.nodes[0].getinfo()['blocks'], 1265)

        # Build 20 blocks on the negative version block to start the invalid chain warning
        for j in range(20):
            block_time += 1
            height += 1
            new_block = create_block(tip_invalid, create_coinbase(height), block_time)
            new_block.nVersion = 4;
            new_block.solve()
            tip_invalid = new_block.sha256
            self.submit_header(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], 1265)
            warning = self.nodes[0].getinfo()['errors']
            if (j < 15):
                assert_equal(warning, BASIC_WARNING)
            else:
                assert_equal(warning, INVALID_WARNING)

        # Build a valid chain to stop the invalid chain warning
        height = self.nodes[0].getblockcount()
        for j in range(20):
            block_time += 1
            height += 1
            new_block = create_block(tip, create_coinbase(height), block_time)
            new_block.nVersion = 4
            new_block.solve()
            tip = new_block.sha256
            self.submit_header_and_block(new_block)
            assert_equal(self.nodes[0].getinfo()['blocks'], height)
            warning = self.nodes[0].getinfo()['errors']
            if (j < 4):
                assert_equal(warning, INVALID_WARNING)
            else:
                assert_equal(warning, BASIC_WARNING)

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text = f.readlines()
        assert_equal(len(alert_text), 4) # There should be totally 4 alerts


    def submit_header_and_block(self, block):
        self.submit_header(block)
        #self.test_node.wait_for_getdata([block.sha256], timeout=5)
        self.submit_block(block)

    def submit_header(self, block):
        self.test_node.send_header_for_blocks([block])
        self.test_node.sync_with_ping()

    def submit_block(self, block):
        self.test_node.send_message(msg_block(block))
        self.test_node.sync_with_ping()

if __name__ == '__main__':
    ForkWarningTest().main()
