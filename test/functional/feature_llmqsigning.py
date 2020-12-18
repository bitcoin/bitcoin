#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from test_framework.test_framework import DashTestFramework

'''
feature_llmqsigning.py

Checks LLMQs signing sessions

'''

class LLMQSigningTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].spork("SPORK_21_QUORUM_ALL_CONNECTED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        id = "0000000000000000000000000000000000000000000000000000000000000001"
        msgHash = "0000000000000000000000000000000000000000000000000000000000000002"
        msgHashConflict = "0000000000000000000000000000000000000000000000000000000000000003"

        def check_sigs(hasrecsigs, isconflicting1, isconflicting2):
            self.log.info('check_sigs hasrecsigs {} isconflicting1 {} isconflicting2 {}'.format(isconflicting1, isconflicting1, isconflicting2))
            for node in self.nodes:
                if node.quorum_hasrecsig(100, id, msgHash) != hasrecsigs:
                    self.log.info('quorum_hasrecsig failed value {} hasrecsigs {}'.format(node.quorum_hasrecsig(100, id, msgHash) , hasrecsigs))
                    return False
                if node.quorum_isconflicting(100, id, msgHash) != isconflicting1:
                    self.log.info('quorum_isconflicting1 failed value {} isconflicting1 {}'.format(node.quorum_isconflicting(100, id, msgHash) , isconflicting1))
                    return False
                if node.quorum_isconflicting(100, id, msgHashConflict) != isconflicting2:
                    self.log.info('quorum_isconflicting2 failed value {} isconflicting2 {}'.format(node.quorum_isconflicting(100, id, msgHashConflict) , isconflicting2))
                    return False
            self.log.info('check_sigs passed') 
            return True

        def wait_for_sigs(hasrecsigs, isconflicting1, isconflicting2, timeout):
            t = time.time()
            while time.time() - t < timeout:
                if check_sigs(hasrecsigs, isconflicting1, isconflicting2):
                    return
                self.bump_mocktime(1)
                time.sleep(1)
            raise AssertionError("wait_for_sigs timed out")

        def assert_sigs_nochange(hasrecsigs, isconflicting1, isconflicting2, timeout):
            assert(not self.wait_until(lambda: not check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout, bumptime=1, do_assert = False))

        # Initial state
        wait_for_sigs(False, False, False, 1)

        # Sign 2 shares, should not result in recovered sig
        for i in range(2):
            self.mninfo[i].node.quorum_sign(100, id, msgHash)
        self.bump_mocktime(5)
        self.nodes[0].generate(1)
        assert_sigs_nochange(False, False, False, 3)

        # Sign one more share, should result in recovered sig and conflict for msgHashConflict
        self.mninfo[2].node.quorum_sign(100, id, msgHash)
        self.bump_mocktime(5)
        self.nodes[0].generate(1)
        self.log.info('before wait_for_sigs')
        wait_for_sigs(True, False, True, 15)

        recsig_time = self.mocktime

        # Mine one more quorum, so that we have 2 active ones, nothing should change
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 5)

        # Mine 2 more quorums, so that the one used for the the recovered sig should become inactive, nothing should change
        self.mine_quorum()
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)
        # fast forward until 6.5 days before cleanup is expected, recovered sig should still be valid
        self.bump_mocktime(recsig_time + int(60 * 60 * 24 * 6.5) - self.mocktime)
        self.nodes[0].generate(1)
        # Cleanup starts every 5 seconds
        wait_for_sigs(True, False, True, 15)
        # fast forward 1 day, recovered sig should not be valid anymore
        self.bump_mocktime(int(60 * 60 * 24 * 1))
        self.nodes[0].generate(1)
        # Cleanup starts every 5 seconds
        wait_for_sigs(False, False, False, 15)

        for i in range(2):
            self.mninfo[i].node.quorum_sign(100, id, msgHashConflict)
        for i in range(2, 5):
            self.mninfo[i].node.quorum_sign(100, id, msgHash)
        self.bump_mocktime(5)
        self.nodes[0].generate(1)
        wait_for_sigs(True, False, True, 15)

        id = "0000000000000000000000000000000000000000000000000000000000000002"

        # Isolate the node that is responsible for the recovery of a signature and assert that recovery fails
        q = self.nodes[0].quorum_selectquorum(100, id)
        mn = self.get_mninfo(q['recoveryMembers'][0])
        mn.node.setnetworkactive(False)
        self.wait_until(lambda: mn.node.getconnectioncount() == 0, bumptime=1)
        for i in range(4):
            self.mninfo[i].node.quorum_sign(100, id, msgHash)
        assert_sigs_nochange(False, False, False, 3)
        # Need to re-connect so that it later gets the recovered sig
        mn.node.setnetworkactive(True)
        self.connect_nodes(mn.node.index, 0)
        # Make sure node0 has received qsendrecsigs from the previously isolated node
        mn.node.ping()
        self.wait_until(lambda: all('pingwait' not in peer for peer in mn.node.getpeerinfo()), bumptime=1)
        # Let 5 seconds pass so that the next node is used for recovery, which should succeed
        self.bump_mocktime(5)
        self.nodes[0].generate(1)
        wait_for_sigs(True, False, True, 15)
if __name__ == '__main__':
    LLMQSigningTest().main()
