#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the network configuration where num nodes is greater than num signers"""
import time
import sys, os

# Import the test primitives
from test_framework.test_framework_signet import BaseSignetTest
from test_framework.util import assert_equal

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner
miner = import_miner()

class SignetNumNodesDiffersFromNumSigners(BaseSignetTest):

    def set_test_params(self):
        self.num_nodes = 2
        self.signet_num_signers = 1
        self.signet_num_signatures = 1
        super().set_test_params()

    def run_test(self):
        self.log.info("Tests using 1-of-1 challenge in a 2-nodes network")

        signet_challenge = self.signet_challenge
        args0 = self.node(0).args

        # Test initial state
        self.assert_blockchaininfo_property_forall_nodes("blocks", 0)

        # Generate the first block
        # Mine 1st block
        block = miner.do_generate_next_block(args0)[0]
        signed_block = miner.do_sign_block(args0, block, signet_challenge)
        miner.do_propagate_block(args0, signed_block)
        time.sleep(1)

        # Check 1st block mined correctly on node 0
        self.assert_blockchaininfo_property(0, "blocks", 1)

        # Check that 1st block propagates correctly to node 1
        assert_equal(self.nodes[1].getblockchaininfo()["blocks"], 1)


if __name__ == '__main__':
    SignetNumNodesDiffersFromNumSigners().main()
