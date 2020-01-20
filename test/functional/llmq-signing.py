#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *

'''
llmq-signing.py

Checks LLMQs signing sessions

'''

class LLMQSigningTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)

    def run_test(self):

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        id = "0000000000000000000000000000000000000000000000000000000000000001"
        msgHash = "0000000000000000000000000000000000000000000000000000000000000002"
        msgHashConflict = "0000000000000000000000000000000000000000000000000000000000000003"

        def check_sigs(hasrecsigs, isconflicting1, isconflicting2):
            for mn in self.mninfo:
                if mn.node.quorum("hasrecsig", 100, id, msgHash) != hasrecsigs:
                    return False
                if mn.node.quorum("isconflicting", 100, id, msgHash) != isconflicting1:
                    return False
                if mn.node.quorum("isconflicting", 100, id, msgHashConflict) != isconflicting2:
                    return False
            return True

        def wait_for_sigs(hasrecsigs, isconflicting1, isconflicting2, timeout):
            t = time.time()
            while time.time() - t < timeout:
                if check_sigs(hasrecsigs, isconflicting1, isconflicting2):
                    return
                time.sleep(0.1)
            raise AssertionError("wait_for_sigs timed out")

        def assert_sigs_nochange(hasrecsigs, isconflicting1, isconflicting2, timeout):
            t = time.time()
            while time.time() - t < timeout:
                assert(check_sigs(hasrecsigs, isconflicting1, isconflicting2))
                time.sleep(0.1)

        # Initial state
        wait_for_sigs(False, False, False, 1)

        # Sign 2 shares, should not result in recovered sig
        for i in range(2):
            self.mninfo[i].node.quorum("sign", 100, id, msgHash)
        assert_sigs_nochange(False, False, False, 3)

        # Sign one more share, should result in recovered sig and conflict for msgHashConflict
        self.mninfo[2].node.quorum("sign", 100, id, msgHash)
        wait_for_sigs(True, False, True, 15)

        # Mine one more quorum, so that we have 2 active ones, nothing should change
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)

        # Mine 2 more quorums, so that the one used for the the recovered sig should become inactive, nothing should change
        self.mine_quorum()
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)

        # fast forward 6.5 days, recovered sig should still be valid
        self.bump_mocktime(int(60 * 60 * 24 * 6.5))
        set_node_times(self.nodes, self.mocktime)
        # Cleanup starts every 5 seconds
        wait_for_sigs(True, False, True, 15)
        # fast forward 1 day, recovered sig should not be valid anymore
        self.bump_mocktime(int(60 * 60 * 24 * 1))
        set_node_times(self.nodes, self.mocktime)
        # Cleanup starts every 5 seconds
        wait_for_sigs(False, False, False, 15)

        for i in range(2):
            self.mninfo[i].node.quorum("sign", 100, id, msgHashConflict)
        for i in range(2, 5):
            self.mninfo[i].node.quorum("sign", 100, id, msgHash)
        wait_for_sigs(True, False, True, 15)

if __name__ == '__main__':
    LLMQSigningTest().main()
