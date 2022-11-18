#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test signature independent merkle root signet functionality"""
import time
import sys, os

# Import the test primitives
from test_framework.test_framework_itcoin import BaseItcoinTest
from test_framework.util import assert_equal

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner
miner = import_miner()

class SignetSignatureIndependentMerkleRootTest(BaseItcoinTest):

    def set_test_params(self):
        self.num_nodes = 4
        self.signet_num_signers = 4
        self.signet_num_signatures = 3
        super().set_test_params()

    def run_test(self):
        self.log.info("Tests using 3-of-4 challenge")


        signet_challenge = self.signet_challenge
        args0 = self.node(0).args
        args1 = self.node(1).args

        # Test initial state
        self.assert_blockchaininfo_property_forall_nodes("blocks", 0)

        # Generate the first block
        # Mine 1st block
        block = miner.do_generate_next_block(args0)[0]
        signed_block = self.do_multisign_block({0, 1, 2}, block, signet_challenge)
        miner.do_propagate_block(args0, signed_block)
        time.sleep(1)

        # Check 1st block mined correctly on node 0
        self.assert_blockchaininfo_property(0, "blocks", 1)

        # Check that 1st block propagates correctly to node 1
        self.assert_blockchaininfo_property(1, "blocks", 1)

        # Mine 2nd block
        block = miner.do_generate_next_block(args0)[0]
        signed_block = self.do_multisign_block({1, 2, 3}, block, signet_challenge)
        miner.do_propagate_block(args0, signed_block)
        time.sleep(1)

        # Check 2nd block mined correctly on node 0
        self.assert_blockchaininfo_property(0, "blocks", 2)

        # Check that 2nd block propagates correctly to node 1
        self.assert_blockchaininfo_property(1, "blocks", 2)

        # 3rd block, we try to cause a fork based on different signatures.
        self.disconnect_nodes(0, 1)

        # Node 0 generates the block
        block = miner.do_generate_next_block(args0)[0]

        # Block signed by node 0, 2, 3
        signed_block_0 = self.do_multisign_block({0, 2, 3}, block, signet_challenge)
        miner.do_propagate_block(args0, signed_block_0)

        # Block signed by node 1, 2, 3
        signed_block_1 = self.do_multisign_block({1, 2, 3}, block, signet_challenge)
        miner.do_propagate_block(args1, signed_block_1)

        # Check block hashes of different nodes
        self.assert_blockchaininfo_property_forall_nodes("blocks", 3)

        # Assert the block hashes are the same
        expected_bestblockhash = self.get_blockchaininfo_field_from_node(0, "bestblockhash")
        self.assert_blockchaininfo_property_forall_nodes("bestblockhash", expected_bestblockhash)

        # Save best block hash of nodes during the partition
        bestblockhash_before_connect = expected_bestblockhash

        # Reconnect the nodes
        self.connect_nodes(0, 1)
        time.sleep(1)

        # Check they are at the same height
        self.assert_blockchaininfo_property_forall_nodes("blocks", 3)
        expected_bestblockhash = self.get_blockchaininfo_field_from_node(0, "bestblockhash")
        self.assert_blockchaininfo_property_forall_nodes("bestblockhash", expected_bestblockhash)

        # Check a reorg did not occurr after the partition is resolved
        assert(expected_bestblockhash == bestblockhash_before_connect)

        #  but they kept the same different coinbase txs
        block3_coinbase_0 = self.nodes[0].getblock(self.nodes[0].getblockhash(3))["tx"][0]
        block3_coinbase_1 = self.nodes[1].getblock(self.nodes[1].getblockhash(3))["tx"][0]
        assert(block3_coinbase_0 != block3_coinbase_1)

        # Mine 4th block
        block = miner.do_generate_next_block(args0)[0]
        signed_block = self.do_multisign_block({0, 1, 3}, block, signet_challenge)
        miner.do_propagate_block(args0, signed_block)
        time.sleep(1)

        # Check 4th block mined correctly on node 0
        self.assert_blockchaininfo_property(0, "blocks", 4)

        # Check that 4th block propagates correctly to node 1
        self.assert_blockchaininfo_property(1, "blocks", 4)

        # Assert the two chains are in sync even if they differ in one coinbase transaction
        expected_bestblockhash = self.get_blockchaininfo_field_from_node(0, "bestblockhash")
        self.assert_blockchaininfo_property_forall_nodes('bestblockhash', expected_bestblockhash)
        block3_coinbase_0 = self.nodes[0].getblock(self.nodes[0].getblockhash(3))["tx"][0]
        block3_coinbase_1 = self.nodes[1].getblock(self.nodes[1].getblockhash(3))["tx"][0]
        block4_coinbase_0 = self.nodes[0].getblock(self.nodes[0].getblockhash(4))["tx"][0]
        block4_coinbase_1 = self.nodes[1].getblock(self.nodes[1].getblockhash(4))["tx"][0]
        assert(block3_coinbase_0 != block3_coinbase_1)
        assert_equal(block4_coinbase_0, block4_coinbase_1)


if __name__ == '__main__':
    SignetSignatureIndependentMerkleRootTest().main()
