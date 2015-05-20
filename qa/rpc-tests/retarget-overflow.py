#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test that the difficulty retarget calculation cannot overflow
# a 256-bit unsigned integer.
#

from test_framework import BitcoinTestFramework
from util import *
from mininode import compact_from_uint256, uint256_from_compact

class RetargetOverflowTest(BitcoinTestFramework):
    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self, split=False):
        self.nodes = start_nodes(2, self.options.tmpdir)

        connect_nodes_bi(self.nodes,0,1)

        self.is_network_split=False
        self.sync_all()

    def run_test(self):
        # Generate a single block to leave IBD.  Without this, calls to
        # getblocktemplate will fail.
        self.nodes[0].generate(1)

        # Fetch the first hash target.  This test is only valid for the
        # regression test network, since the POW limits for main and
        # testnet are too low to expose the overflow.
        gbtResp = self.nodes[0].getblocktemplate()
        prevTargetStr = gbtResp["target"]
        assert_equal(prevTargetStr, "7fffff0000000000000000000000000000000000000000000000000000000000")
        prevTarget = int(prevTargetStr, 16)
        
        # For 10 minute blocks and a target of 2 weeks between difficulty
        # retargets, the first retarget will occur at block 2016
        # (60 mins/hr * 24 hrs/day * 7 days/wk * 2 wks / 10 mins/block = 2016 blocks).
        # Generate one less than this, and then verify that the next
        # difficulty is within the expected bounds (indicating that the
        # calculation did not ovewflow).
        #
        # One block has already been generated above to leave IBD, so only
        # generate 2014 more.
        #
        # Use multiple calls to avoid RPC timeout in slow test environments.
        self.nodes[0].generate(500)
        self.nodes[0].generate(500)
        self.nodes[0].generate(500)
        self.nodes[0].generate(500)
        self.nodes[0].generate(14)  # now at height 2015

        # Fetch the next target difficulty for block 2016.
        gbtResp = self.nodes[0].getblocktemplate()
        retargetDifficultyStr = gbtResp["target"]
        retargetDifficulty = int(retargetDifficultyStr, 16)

        # A difficutly retarget should result in the next difficulty in
        # the range [previous/4, previous*4].  However, since this is the
        # first retarget, and the difficulty can never exceed the networks's
        # POW limit, the next difficulty must be in [previous/4, compact(powLimit)].
        minimum = uint256_from_compact(compact_from_uint256(prevTarget / 4))
        maximum = uint256_from_compact(compact_from_uint256(prevTarget))
        if retargetDifficulty < minimum or retargetDifficulty > maximum:
            raise AssertionError("Retarget difficulty %s outside expected range (possible overflow detected)"%(retargetDifficultyStr))
        prevTarget = retargetDifficulty

        # Test a second retarget for block 4032.  While the first retarget
        # likely kept the same difficulty due to the very early regtest
        # genesis block timestamp, resulting in a very long timespan between
        # retargets, the next retarget should use the timestamp in block 2016
        # and have a very short timespan.
        self.nodes[0].generate(500)
        self.nodes[0].generate(500)
        self.nodes[0].generate(500)
        self.nodes[0].generate(500)
        self.nodes[0].generate(16) # now at height 4031
        
        gbtResp = self.nodes[0].getblocktemplate()
        retargetDifficultyStr = gbtResp["target"]
        retargetDifficulty = int(retargetDifficultyStr, 16)

        minimum = uint256_from_compact(compact_from_uint256(prevTarget / 4))
        maximum = uint256_from_compact(compact_from_uint256(prevTarget))
        if retargetDifficulty < minimum or retargetDifficulty > maximum:
            raise AssertionError("Retarget difficulty %s outside expected range (possible overflow detected)"%(retargetDifficultyStr))

if __name__ == '__main__':
    RetargetOverflowTest().main()
