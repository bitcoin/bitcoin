#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test LIP-0006

1. Test getdata 'mwebheader' *before* MWEB activation
2. Test getdata 'mwebheader' *after* MWEB activation
3. Test getdata 'mwebleafset' *before* MWEB activation
4. Test getdata 'mwebleafset' *after* MWEB activation
   - Request from earlier block (not tip) to make sure rewind works
"""

from test_framework.messages import (
    CBlockHeader,
    CInv,
    Hash,
    hash256,
    msg_getdata,
    MSG_MWEB_HEADER,
    MSG_MWEB_LEAFSET,
)
from test_framework.p2p import P2PInterface, p2p_lock
from test_framework.script import MAX_SCRIPT_ELEMENT_SIZE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.ltc_util import FIRST_MWEB_HEIGHT, get_hogex_tx, get_mweb_header
from test_framework.util import assert_equal

# Can be used to mimic a light client requesting MWEB data from a full node
class MockLightClient(P2PInterface):
    def __init__(self):
        super().__init__()
        self.merkle_blocks_with_mweb = {}
        self.block_headers = {}
        self.leafsets = {}

    def request_mweb_header(self, block_hash):
        want = msg_getdata([CInv(MSG_MWEB_HEADER, int(block_hash, 16))])
        self.send_message(want)

    def on_mwebheader(self, message):
        self.merkle_blocks_with_mweb[message.header_hash()] = message.merkleblockwithmweb
        
    def request_mweb_leafset(self, block_hash):
        want = msg_getdata([CInv(MSG_MWEB_LEAFSET, int(block_hash, 16))])
        self.send_message(want)

    def on_mwebleafset(self, message):
        self.leafsets[message.block_hash] = message.leafset

    def on_block(self, message):
        message.block.calc_sha256()
        self.block_headers[Hash(message.block.sha256)] = CBlockHeader(message.block)


class MWEBP2PTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.extra_args = [['-whitelist=noban@127.0.0.1']]  # immediate tx relay
        self.num_nodes = 1

    def assert_mweb_header(self, node, light_client, post_mweb_block_hash):
        assert post_mweb_block_hash in light_client.merkle_blocks_with_mweb
        merkle_block_with_mweb = light_client.merkle_blocks_with_mweb[post_mweb_block_hash]

        # Check block header is correct
        assert Hash.from_hex(post_mweb_block_hash) in light_client.block_headers
        block_header = light_client.block_headers[Hash.from_hex(post_mweb_block_hash)]
        assert_equal(block_header, merkle_block_with_mweb.merkle.header)

        # Check MWEB header is correct
        mweb_header = get_mweb_header(node, post_mweb_block_hash)
        assert_equal(mweb_header, merkle_block_with_mweb.mweb_header)

        # Check HogEx transaction is correct
        hogex_tx = get_hogex_tx(node, post_mweb_block_hash)
        assert_equal(hogex_tx, merkle_block_with_mweb.hogex)

        # Check Merkle tree
        merkle_tree = merkle_block_with_mweb.merkle.txn
        assert_equal(3, merkle_tree.nTransactions)
        assert_equal(2, len(merkle_tree.vHash))
        
        left_hash = Hash(merkle_tree.vHash[0])
        right_hash = Hash(merkle_tree.vHash[1])
        assert_equal(Hash.from_hex(hogex_tx.hash), right_hash)

        right_branch_bytes = hash256(right_hash.serialize() + right_hash.serialize())
        merkle_root_bytes = hash256(left_hash.serialize() + right_branch_bytes)
        assert_equal(Hash.from_byte_arr(merkle_root_bytes), Hash(block_header.hashMerkleRoot))

    def run_test(self):
        node = self.nodes[0]
        light_client = node.add_p2p_connection(MockLightClient())

        self.log.info("Generate pre-MWEB blocks")
        pre_mweb_block_hash = node.generate(FIRST_MWEB_HEIGHT - 1)[-1]

        self.log.info("Request 'mwebheader' and 'mwebleafset' for pre-MWEB block '{}'".format(pre_mweb_block_hash))
        light_client.request_mweb_header(pre_mweb_block_hash)
        light_client.request_mweb_leafset(pre_mweb_block_hash)
        
        self.log.info("Activate MWEB")
        node.sendtoaddress(node.getnewaddress(address_type='mweb'), 1)
        post_mweb_block_hash = node.generate(1)[0]

        self.log.info("Waiting for mweb_header")
        light_client.wait_for_block(int(post_mweb_block_hash, 16), 5)
        
        self.log.info("Pegin some additional coins")
        node.sendtoaddress(node.getnewaddress(address_type='mweb'), 10)
        post_mweb_block_hash2 = node.generate(1)[0]
        light_client.wait_for_block(int(post_mweb_block_hash2, 16), 5)
        
        self.log.info("Request 'mwebheader' and 'mwebleafset' for block '{}'".format(post_mweb_block_hash))
        light_client.request_mweb_header(post_mweb_block_hash)
        light_client.request_mweb_leafset(post_mweb_block_hash)

        self.log.info("Waiting for 'mwebheader' and 'mwebleafset'")
        light_client.wait_for_mwebheader(post_mweb_block_hash, 5)
        light_client.wait_for_mwebleafset(post_mweb_block_hash, 5)

        self.log.info("Assert results")

        # Before MWEB activation, no merkle block should be returned
        assert pre_mweb_block_hash not in light_client.merkle_blocks_with_mweb

        # After MWEB activation, the requested merkle block should be returned
        self.assert_mweb_header(node, light_client, post_mweb_block_hash)

        # Before MWEB activation, no leafset should be returned
        assert pre_mweb_block_hash not in light_client.leafsets

        # After MWEB activation, the leafset should be returned
        # Only 2 outputs should be in the UTXO set (the pegin and its change)
        # That's '11' and then padded to the right with 0's, then serialized in big endian.
        # So we expect the serialized leafset to be 0b11000000 or 0xc0
        assert Hash.from_hex(post_mweb_block_hash) in light_client.leafsets
        leafset = light_client.leafsets[Hash.from_hex(post_mweb_block_hash)]
        assert_equal([0xc0], leafset)

if __name__ == '__main__':
    MWEBP2PTest().main()