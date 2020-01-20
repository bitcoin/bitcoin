#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import DashTestFramework
from test_framework.util import *

'''
llmq-dkgerrors.py

Simulate and check DKG errors

'''

class LLMQDKGErrors(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, [["-whitelist=127.0.0.1"]] * 6, fast_dip3_enforcement=True)

    def run_test(self):

        while self.nodes[0].getblockchaininfo()["bip9_softforks"]["dip0008"]["status"] != "active":
            self.nodes[0].generate(10)
        sync_blocks(self.nodes, timeout=60*5)

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # Mine one quorum without simulating any errors
        qh = self.mine_quorum()
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        # Lets omit the contribution
        self.mninfo[0].node.quorum('dkgsimerror', 'contribution-omit', '1')
        qh = self.mine_quorum(expected_contributions=4)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, False)

        # Lets lie in the contribution but provide a correct justification
        self.mninfo[0].node.quorum('dkgsimerror', 'contribution-omit', '0')
        self.mninfo[0].node.quorum('dkgsimerror', 'contribution-lie', '1')
        qh = self.mine_quorum(expected_contributions=5, expected_complaints=4, expected_justifications=1)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        # Lets lie in the contribution and then omit the justification
        self.mninfo[0].node.quorum('dkgsimerror', 'justify-omit', '1')
        qh = self.mine_quorum(expected_contributions=4, expected_complaints=4)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, False)

        # Heal some damage (don't get PoSe banned)
        self.heal_masternodes(33)

        # Lets lie in the contribution and then also lie in the justification
        self.mninfo[0].node.quorum('dkgsimerror', 'justify-omit', '0')
        self.mninfo[0].node.quorum('dkgsimerror', 'justify-lie', '1')
        qh = self.mine_quorum(expected_contributions=4, expected_complaints=4, expected_justifications=1)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, False)

        # Lets lie about another MN
        self.mninfo[0].node.quorum('dkgsimerror', 'contribution-lie', '0')
        self.mninfo[0].node.quorum('dkgsimerror', 'justify-lie', '0')
        self.mninfo[0].node.quorum('dkgsimerror', 'complain-lie', '1')
        qh = self.mine_quorum(expected_contributions=5, expected_complaints=1, expected_justifications=4)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        # Lets omit 2 premature commitments
        self.mninfo[0].node.quorum('dkgsimerror', 'complain-lie', '0')
        self.mninfo[0].node.quorum('dkgsimerror', 'commit-omit', '1')
        self.mninfo[1].node.quorum('dkgsimerror', 'commit-omit', '1')
        qh = self.mine_quorum(expected_contributions=5, expected_complaints=0, expected_justifications=0, expected_commitments=3)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        # Lets lie in 2 premature commitments
        self.mninfo[0].node.quorum('dkgsimerror', 'commit-omit', '0')
        self.mninfo[1].node.quorum('dkgsimerror', 'commit-omit', '0')
        self.mninfo[0].node.quorum('dkgsimerror', 'commit-lie', '1')
        self.mninfo[1].node.quorum('dkgsimerror', 'commit-lie', '1')
        qh = self.mine_quorum(expected_contributions=5, expected_complaints=0, expected_justifications=0, expected_commitments=3)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

    def assert_member_valid(self, quorumHash, proTxHash, expectedValid):
        q = self.nodes[0].quorum('info', 100, quorumHash, True)
        for m in q['members']:
            if m['proTxHash'] == proTxHash:
                if expectedValid:
                    assert(m['valid'])
                else:
                    assert(not m['valid'])
            else:
                assert(m['valid'])

    def heal_masternodes(self, blockCount):
        # We're not testing PoSe here, so lets heal the MNs :)
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        for i in range(blockCount):
            self.bump_mocktime(1)
            set_node_times(self.nodes, self.mocktime)
            self.nodes[0].generate(1)
        self.sync_all()
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()


if __name__ == '__main__':
    LLMQDKGErrors().main()
