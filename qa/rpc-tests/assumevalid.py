#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
assumevalid.py

Test logic for skipping signature validation on blocks which we've assumed
valid (https://github.com/bitcoin/bitcoin/pull/9484)

We build a chain that includes and invalid signature for one of the
transactions:

    0:        genesis block
    1:        block 1 with coinbase transaction output.
    2-101:    bury that block with 100 blocks so the coinbase transaction
              output can be spent
    102:      a block containing a transaction spending the coinbase
              transaction output. The transaction has an invalid signature. 
    103-2202: bury the bad block with just over two weeks' worth of blocks
              (2100 blocks)

Start three nodes:

    - node0 has no -assumevalid parameter. Try to sync to block 2202. It will
      reject block 102 and only sync as far as block 101
    - node1 has -assumevalid set to the hash of block 102. Try to sync to
      block 2202. node1 will sync all the way to block 2202.
    - node2 has -assumevalid set to the hash of block 102. Try to sync to
      block 200. node2 will reject block 102 since it's assumed valid, but it
      isn't buried by at least two weeks' work.
'''

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase
from test_framework.key import CECKey
from test_framework.script import *

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

    def on_close(self, conn):
        self.disconnected = True

    def wait_for_disconnect(self, timeout=60):
        test_function = lambda: self.disconnected
        assert(wait_until(test_function, timeout=timeout))
        return

    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [ CBlockHeader(b) for b in new_blocks ]
        self.send_message(headers_message)

class SendHeadersTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self):
        # Start node0. We don't start the other nodes yet since
        # we need to pre-mine a block with an invalid transaction
        # signature so we can pass in the block hash as assumevalid.
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))

    def run_test(self):

        # Connect to node0
        node0 = BaseNode()
        connections = []
        connections.append(NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], node0))
        node0.add_connection(connections[0])

        NetworkThread().start() # Start up network handling in another thread
        node0.wait_for_verack()

        # Build the blockchain
        self.tip = int(self.nodes[0].getbestblockhash(), 16)
        self.block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1

        self.blocks = []

        # Get a pubkey for the coinbase TXO
        coinbase_key = CECKey()
        coinbase_key.set_secretbytes(b"horsebattery")
        coinbase_pubkey = coinbase_key.get_pubkey()

        # Create the first block with a coinbase output to our key
        height = 1
        block = create_block(self.tip, create_coinbase(height, coinbase_pubkey), self.block_time)
        self.blocks.append(block)
        self.block_time += 1
        block.solve()
        # Save the coinbase for later
        self.block1 = block
        self.tip = block.sha256
        height += 1

        # Bury the block 100 deep so the coinbase output is spendable
        for i in range(100):
            block = create_block(self.tip, create_coinbase(height), self.block_time)
            block.solve()
            self.blocks.append(block)
            self.tip = block.sha256
            self.block_time += 1
            height += 1

        # Create a transaction spending the coinbase output with an invalid (null) signature
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.block1.vtx[0].sha256, 0), scriptSig=b""))
        tx.vout.append(CTxOut(49*100000000, CScript([OP_TRUE])))
        tx.calc_sha256()

        block102 = create_block(self.tip, create_coinbase(height), self.block_time)
        self.block_time += 1
        block102.vtx.extend([tx])
        block102.hashMerkleRoot = block102.calc_merkle_root()
        block102.rehash()
        block102.solve()
        self.blocks.append(block102)
        self.tip = block102.sha256
        self.block_time += 1
        height += 1

        # Bury the assumed valid block 2100 deep
        for i in range(2100):
            block = create_block(self.tip, create_coinbase(height), self.block_time)
            block.nVersion = 4
            block.solve()
            self.blocks.append(block)
            self.tip = block.sha256
            self.block_time += 1
            height += 1

        # Start node1 and node2 with assumevalid so they accept a block with a bad signature.
        self.nodes.append(start_node(1, self.options.tmpdir,
                                     ["-debug", "-assumevalid=" + hex(block102.sha256)]))
        node1 = BaseNode()  # connects to node1
        connections.append(NodeConn('127.0.0.1', p2p_port(1), self.nodes[1], node1))
        node1.add_connection(connections[1])
        node1.wait_for_verack()

        self.nodes.append(start_node(2, self.options.tmpdir,
                                     ["-debug", "-assumevalid=" + hex(block102.sha256)]))
        node2 = BaseNode()  # connects to node2
        connections.append(NodeConn('127.0.0.1', p2p_port(2), self.nodes[2], node2))
        node2.add_connection(connections[2])
        node2.wait_for_verack()

        # send header lists to all three nodes
        node0.send_header_for_blocks(self.blocks[0:2000])
        node0.send_header_for_blocks(self.blocks[2000:])
        node1.send_header_for_blocks(self.blocks[0:2000])
        node1.send_header_for_blocks(self.blocks[2000:])
        node2.send_header_for_blocks(self.blocks[0:200])

        # Send 102 blocks to node0. Block 102 will be rejected.
        for i in range(101):
            node0.send_message(msg_block(self.blocks[i]))
        node0.sync_with_ping() # make sure the most recent block is synced
        node0.send_message(msg_block(self.blocks[101]))
        assert_equal(self.nodes[0].getblock(self.nodes[0].getbestblockhash())['height'], 101)

        # Send 3102 blocks to node1. All blocks will be accepted.
        for i in range(2202):
            node1.send_message(msg_block(self.blocks[i]))
        node1.sync_with_ping() # make sure the most recent block is synced
        assert_equal(self.nodes[1].getblock(self.nodes[1].getbestblockhash())['height'], 2202)

        # Send 102 blocks to node2. Block 102 will be rejected.
        for i in range(101):
            node2.send_message(msg_block(self.blocks[i]))
        node2.sync_with_ping() # make sure the most recent block is synced
        node2.send_message(msg_block(self.blocks[101]))
        assert_equal(self.nodes[2].getblock(self.nodes[2].getbestblockhash())['height'], 101)

if __name__ == '__main__':
    SendHeadersTest().main()
