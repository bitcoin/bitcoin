#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import DashTestFramework
from test_framework.util import *

'''
feature_llmq_signing.py

Checks LLMQs signing sessions

'''

class LLMQSigningTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def add_options(self, parser):
        parser.add_option("--spork21", dest="spork21", default=False, action="store_true",
                          help="Test with spork21 enabled")

    def run_test(self):

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        if self.options.spork21:
            self.nodes[0].spork("SPORK_21_QUORUM_ALL_CONNECTED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        id = "0000000000000000000000000000000000000000000000000000000000000001"
        msgHash = "0000000000000000000000000000000000000000000000000000000000000002"
        msgHashConflict = "0000000000000000000000000000000000000000000000000000000000000003"

        def check_sigs(hasrecsigs, isconflicting1, isconflicting2):
            for node in self.nodes:
                if node.quorum("hasrecsig", 100, id, msgHash) != hasrecsigs:
                    return False
                if node.quorum("isconflicting", 100, id, msgHash) != isconflicting1:
                    return False
                if node.quorum("isconflicting", 100, id, msgHashConflict) != isconflicting2:
                    return False
            return True

        def wait_for_sigs(hasrecsigs, isconflicting1, isconflicting2, timeout):
            wait_until(lambda: check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout)

        def assert_sigs_nochange(hasrecsigs, isconflicting1, isconflicting2, timeout):
            assert(not wait_until(lambda: not check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout, do_assert = False))

        # Initial state
        wait_for_sigs(False, False, False, 1)

        # Sign 2 shares, should not result in recovered sig
        for i in range(2):
            self.mninfo[i].node.quorum("sign", 100, id, msgHash)
        assert_sigs_nochange(False, False, False, 3)

        # Sign one more share and test optional quorumHash parameter.
        # Should result in recovered sig and conflict for msgHashConflict
        # 1. Providing an invalid quorum hash should fail and cause no changes for sigs
        assert(not self.mninfo[2].node.quorum("sign", 100, id, msgHash, msgHash))
        assert_sigs_nochange(False, False, False, 3)
        # 2. Providing a valid quorum hash should succeed and finally result in the recovered sig
        quorumHash = self.mninfo[2].node.quorum("selectquorum", 100, id)["quorumHash"]
        assert(self.mninfo[2].node.quorum("sign", 100, id, msgHash, quorumHash))
        wait_for_sigs(True, False, True, 15)

        # Test `quorum verify` rpc
        node = self.nodes[0]
        recsig = node.quorum("getrecsig", 100, id, msgHash)
        # Find quorum automatically
        height = node.getblockcount()
        height_bad = node.getblockheader(recsig["quorumHash"])["height"]
        hash_bad = node.getblockhash(0)
        assert(node.quorum("verify", 100, id, msgHash, recsig["sig"]))
        assert(node.quorum("verify", 100, id, msgHash, recsig["sig"], "", height))
        assert(not node.quorum("verify", 100, id, msgHashConflict, recsig["sig"]))
        assert_raises_rpc_error(-8, "quorum not found", node.quorum, "verify", 100, id, msgHash, recsig["sig"], "", height_bad)
        # Use specifc quorum
        assert(node.quorum("verify", 100, id, msgHash, recsig["sig"], recsig["quorumHash"]))
        assert(not node.quorum("verify", 100, id, msgHashConflict, recsig["sig"], recsig["quorumHash"]))
        assert_raises_rpc_error(-8, "quorum not found", node.quorum, "verify", 100, id, msgHash, recsig["sig"], hash_bad)

        recsig_time = self.mocktime

        # Mine one more quorum, so that we have 2 active ones, nothing should change
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)

        # Mine 2 more quorums, so that the one used for the the recovered sig should become inactive, nothing should change
        self.mine_quorum()
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)

        # fast forward until 0.5 days before cleanup is expected, recovered sig should still be valid
        self.bump_mocktime(recsig_time + int(60 * 60 * 24 * 6.5) - self.mocktime)
        # Cleanup starts every 5 seconds
        wait_for_sigs(True, False, True, 15)
        # fast forward 1 day, recovered sig should not be valid anymore
        self.bump_mocktime(int(60 * 60 * 24 * 1))
        # Cleanup starts every 5 seconds
        wait_for_sigs(False, False, False, 15)

        for i in range(2):
            self.mninfo[i].node.quorum("sign", 100, id, msgHashConflict)
        for i in range(2, 5):
            self.mninfo[i].node.quorum("sign", 100, id, msgHash)
        wait_for_sigs(True, False, True, 15)

        if self.options.spork21:
            id = "0000000000000000000000000000000000000000000000000000000000000002"

            # Isolate the node that is responsible for the recovery of a signature and assert that recovery fails
            q = self.nodes[0].quorum('selectquorum', 100, id)
            mn = self.get_mninfo(q['recoveryMembers'][0])
            mn.node.setnetworkactive(False)
            wait_until(lambda: mn.node.getconnectioncount() == 0)
            for i in range(4):
                self.mninfo[i].node.quorum("sign", 100, id, msgHash)
            assert_sigs_nochange(False, False, False, 3)
            # Need to re-connect so that it later gets the recovered sig
            mn.node.setnetworkactive(True)
            connect_nodes(mn.node, 0)
            # Make sure node0 has received qsendrecsigs from the previously isolated node
            mn.node.ping()
            wait_until(lambda: all('pingwait' not in peer for peer in mn.node.getpeerinfo()))
            # Let 2 seconds pass so that the next node is used for recovery, which should succeed
            self.bump_mocktime(2)
            wait_for_sigs(True, False, True, 2)

if __name__ == '__main__':
    LLMQSigningTest().main()
