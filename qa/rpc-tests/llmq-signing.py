#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *
from time import *

'''
llmq-signing.py

Checks LLMQs signing sessions

'''

class LLMQSigningTest(DashTestFramework):
    def __init__(self):
        super().__init__(11, 10, [], fast_dip3_activation=True)

    def run_test(self):

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        id = "0000000000000000000000000000000000000000000000000000000000000001"
        msgHash = "0000000000000000000000000000000000000000000000000000000000000002"
        msgHashConflict = "0000000000000000000000000000000000000000000000000000000000000003"

        def assert_sigs(hasrecsigs, isconflicting1, isconflicting2):
            for mn in self.mninfo:
                assert(mn.node.quorum("hasrecsig", 100, id, msgHash) == hasrecsigs)
                assert(mn.node.quorum("isconflicting", 100, id, msgHash) == isconflicting1)
                assert(mn.node.quorum("isconflicting", 100, id, msgHashConflict) == isconflicting2)

        # Initial state
        assert_sigs(False, False, False)

        # Sign 5 shares, should not result in recovered sig
        for i in range(5):
            self.mninfo[i].node.quorum("sign", 100, id, msgHash)
        sleep(2)
        assert_sigs(False, False, False)

        # Sign one more share, should result in recovered sig and conflict for msgHashConflict
        self.mninfo[6].node.quorum("sign", 100, id, msgHash)
        sleep(2)
        assert_sigs(True, False, True)

        # Mine one more quorum, so that we have 2 active ones, nothing should change
        self.mine_quorum()
        assert_sigs(True, False, True)

        # Mine 2 more quorums, so that the one used for the the recovered sig should become inactive, nothing should change
        self.mine_quorum()
        self.mine_quorum()
        assert_sigs(True, False, True)

        # fast forward 6.5 days, recovered sig should still be valid
        set_mocktime(get_mocktime() + int(60 * 60 * 24 * 6.5))
        set_node_times(self.nodes, get_mocktime())
        sleep(6) # Cleanup starts every 5 seconds
        assert_sigs(True, False, True)
        # fast forward 1 day, recovered sig should not be valid anymore
        set_mocktime(get_mocktime() + int(60 * 60 * 24 * 1))
        set_node_times(self.nodes, get_mocktime())
        sleep(6) # Cleanup starts every 5 seconds
        assert_sigs(False, False, False)

        for i in range(4):
            self.mninfo[i].node.quorum("sign", 100, id, msgHashConflict)
        for i in range(4, 10):
            self.mninfo[i].node.quorum("sign", 100, id, msgHash)
        sleep(2)
        assert_sigs(True, False, True)

if __name__ == '__main__':
    LLMQSigningTest().main()
