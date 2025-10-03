#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test logic for skipping signature validation on old blocks.

Test logic for skipping signature validation on blocks which we've assumed
valid (https://github.com/bitcoin/bitcoin/pull/9484)

We build a chain that includes an invalid signature for one of the transactions:

    0:        genesis block
    1:        block 1 with coinbase transaction output.
    2-101:    bury that block with 100 blocks so the coinbase transaction
              output can be spent
    102:      a block containing a transaction spending the coinbase
              transaction output. The transaction has an invalid signature.
    103-2202: bury the bad block with just over two weeks' worth of blocks
              (2100 blocks)

Start a few nodes:

    - node0 has no -assumevalid parameter. Try to sync to block 2202. It will
      reject block 102 and only sync as far as block 101
    - node1 has -assumevalid set to the hash of block 102. Try to sync to
      block 2202. node1 will sync all the way to block 2202.
    - node2 has -assumevalid set to the hash of block 102. Try to sync to
      block 200. node2 will reject block 102 since it's assumed valid, but it
      isn't buried by at least two weeks' work.
    - node3 has -assumevalid set to the hash of block 102. Feed a longer
      competing headers-only branch so block #1 is not on the best header chain.
    - node4 has -assumevalid set to the hash of block 102. Submit an alternative
      block #1 that is not part of the assumevalid chain.
    - node5 starts with no -assumevalid parameter. Reindex to hit
      "assumevalid hash not in headers" and "below minimum chainwork".
"""

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
    create_coinbase,
)
from test_framework.messages import (
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
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet_util import generate_keypair


class BaseNode(P2PInterface):
    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_without_ping(headers_message)


class AssumeValidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 6
        self.rpc_timeout = 120

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Start node0. We don't start the other nodes yet since
        # we need to pre-mine a block with an invalid transaction
        # signature so we can pass in the block hash as assumevalid.
        self.start_node(0)

    def send_blocks_until_disconnected(self, p2p_conn):
        """Keep sending blocks to the node until we're disconnected."""
        for i in range(len(self.blocks)):
            if not p2p_conn.is_connected:
                break
            try:
                p2p_conn.send_without_ping(msg_block(self.blocks[i]))
            except IOError:
                assert not p2p_conn.is_connected
                break

    def run_test(self):
        # Build the blockchain
        self.tip = int(self.nodes[0].getbestblockhash(), 16)
        self.block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1

        self.blocks = []

        # Get a pubkey for the coinbase TXO
        _, coinbase_pubkey = generate_keypair()

        # Create the first block with a coinbase output to our key
        height = 1
        block = create_block(self.tip, create_coinbase(height, coinbase_pubkey), self.block_time)
        self.blocks.append(block)
        self.block_time += 1
        block.solve()
        # Save the coinbase for later
        self.block1 = block
        self.tip = block.hash_int
        height += 1

        # Bury the block 100 deep so the coinbase output is spendable
        for _ in range(100):
            block = create_block(self.tip, create_coinbase(height), self.block_time)
            block.solve()
            self.blocks.append(block)
            self.tip = block.hash_int
            self.block_time += 1
            height += 1

        # Create a transaction spending the coinbase output with an invalid (null) signature
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(self.block1.vtx[0].txid_int, 0), scriptSig=b""))
        tx.vout.append(CTxOut(49 * 100000000, CScript([OP_TRUE])))

        block102 = create_block(self.tip, create_coinbase(height), self.block_time, txlist=[tx])
        self.block_time += 1
        block102.solve()
        self.blocks.append(block102)
        self.tip = block102.hash_int
        self.block_time += 1
        height += 1

        # Bury the assumed valid block 2100 deep
        for _ in range(2100):
            block = create_block(self.tip, create_coinbase(height), self.block_time)
            block.solve()
            self.blocks.append(block)
            self.tip = block.hash_int
            self.block_time += 1
            height += 1
        block_1_hash = self.blocks[0].hash_hex

        self.start_node(1, extra_args=[f"-assumevalid={block102.hash_hex}"])
        self.start_node(2, extra_args=[f"-assumevalid={block102.hash_hex}"])
        self.start_node(3, extra_args=[f"-assumevalid={block102.hash_hex}"])
        self.start_node(4, extra_args=[f"-assumevalid={block102.hash_hex}"])
        self.start_node(5)


        # nodes[0]
        # Send blocks to node0. Block 102 will be rejected.
        with self.nodes[0].assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #1 ({block_1_hash}): assumevalid=0 (always verify).",
            "Block validation error: block-script-verify-flag-failed",
        ]):
            p2p0 = self.nodes[0].add_p2p_connection(BaseNode())

            p2p0.send_header_for_blocks(self.blocks[0:2000])
            p2p0.send_header_for_blocks(self.blocks[2000:])

            self.send_blocks_until_disconnected(p2p0)
            self.wait_until(lambda: self.nodes[0].getblockcount() >= COINBASE_MATURITY + 1)
            assert_equal(self.nodes[0].getblockcount(), COINBASE_MATURITY + 1)


        # nodes[1]
        with self.nodes[1].assert_debug_log(expected_msgs=[
            f"Disabling script verification at block #1 ({self.blocks[0].hash_hex}).",
            f"Enabling script verification at block #103 ({self.blocks[102].hash_hex}): block height above assumevalid height.",
        ]):
            p2p1 = self.nodes[1].add_p2p_connection(BaseNode())

            p2p1.send_header_for_blocks(self.blocks[0:2000])
            p2p1.send_header_for_blocks(self.blocks[2000:])
            # Send all blocks to node1. All blocks will be accepted.
            for i in range(2202):
                p2p1.send_without_ping(msg_block(self.blocks[i]))
            # Syncing 2200 blocks can take a while on slow systems. Give it plenty of time to sync.
            p2p1.sync_with_ping(timeout=960)
            assert_equal(self.nodes[1].getblock(self.nodes[1].getbestblockhash())['height'], 2202)


        # nodes[2]
        # Send blocks to node2. Block 102 will be rejected.
        with self.nodes[2].assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #1 ({block_1_hash}): block too recent relative to best header.",
            "Block validation error: block-script-verify-flag-failed",
        ]):
            p2p2 = self.nodes[2].add_p2p_connection(BaseNode())
            p2p2.send_header_for_blocks(self.blocks[0:200])

            self.send_blocks_until_disconnected(p2p2)

            self.wait_until(lambda: self.nodes[2].getblockcount() >= COINBASE_MATURITY + 1)
            assert_equal(self.nodes[2].getblockcount(), COINBASE_MATURITY + 1)


        # nodes[3]
        with self.nodes[3].assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #1 ({block_1_hash}): block not in best header chain.",
        ]):
            best_hash = self.nodes[3].getbestblockhash()
            tip_block = self.nodes[3].getblock(best_hash)
            second_chain_tip, second_chain_time, second_chain_height = int(best_hash, 16), tip_block["time"] + 1, tip_block["height"] + 1
            second_chain = []
            for _ in range(150):
                block = create_block(second_chain_tip, create_coinbase(second_chain_height), second_chain_time)
                block.solve()
                second_chain.append(block)
                second_chain_tip, second_chain_time, second_chain_height = block.hash_int, second_chain_time + 1, second_chain_height + 1

            p2p3 = self.nodes[3].add_p2p_connection(BaseNode())

            p2p3.send_header_for_blocks(second_chain)
            p2p3.send_header_for_blocks(self.blocks[0:103])

            p2p3.send_without_ping(msg_block(self.blocks[0]))
            self.wait_until(lambda: self.nodes[3].getblockcount() == 1)


        # nodes[4]
        genesis_hash = self.nodes[4].getbestblockhash()
        genesis_time = self.nodes[4].getblock(genesis_hash)['time']
        alt1 = create_block(int(genesis_hash, 16), create_coinbase(1), genesis_time + 2)
        alt1.solve()
        with self.nodes[4].assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #1 ({alt1.hash_hex}): block not in assumevalid chain.",
        ]):
            p2p4 = self.nodes[4].add_p2p_connection(BaseNode())
            p2p4.send_header_for_blocks(self.blocks[0:103])

            p2p4.send_without_ping(msg_block(alt1))
            self.wait_until(lambda: self.nodes[4].getblockcount() == 1)


        # nodes[5]
        # Reindex to hit specific assumevalid gates (no races with header downloads/chainwork during startup).
        p2p5 = self.nodes[5].add_p2p_connection(BaseNode())
        p2p5.send_header_for_blocks(self.blocks[0:200])
        p2p5.send_without_ping(msg_block(self.blocks[0]))
        self.wait_until(lambda: self.nodes[5].getblockcount() == 1)
        with self.nodes[5].assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #1 ({block_1_hash}): assumevalid hash not in headers.",
        ]):
            self.restart_node(5, extra_args=["-reindex-chainstate", "-assumevalid=1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"])
            assert_equal(self.nodes[5].getblockcount(), 1)
        with self.nodes[5].assert_debug_log(expected_msgs=[
            f"Enabling script verification at block #1 ({block_1_hash}): best header chainwork below minimumchainwork.",
        ]):
            self.restart_node(5, extra_args=["-reindex-chainstate", f"-assumevalid={block102.hash_hex}", "-minimumchainwork=0xffff"])
            assert_equal(self.nodes[5].getblockcount(), 1)


if __name__ == '__main__':
    AssumeValidTest(__file__).main()
