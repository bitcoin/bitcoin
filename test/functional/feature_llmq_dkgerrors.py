#!/usr/bin/env python3
# Copyright (c) 2015-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import DashTestFramework

'''
feature_llmq_dkgerrors.py

Simulate and check DKG errors

'''

class LLMQDKGErrors(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(4, 3, [["-whitelist=127.0.0.1"]] * 4)

    def run_test(self):
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        self.log.info("Mine one quorum without simulating any errors")
        qh = self.mine_quorum()
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        mninfos_valid = self.mninfo.copy()[1:]

        self.log.info("Lets omit the contribution")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'contribution-omit', '100')
        qh = self.mine_quorum(expected_contributions=2, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, False)

        self.log.info("Lets lie in the contribution but provide a correct justification")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'contribution-omit', '0')
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'contribution-lie', '100')
        qh = self.mine_quorum(expected_contributions=3, expected_complaints=2, expected_justifications=1, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        self.log.info("Lets lie in the contribution and then omit the justification")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'justify-omit', '100')
        qh = self.mine_quorum(expected_contributions=3, expected_complaints=2, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, False)

        self.log.info("Heal some damage (don't get PoSe banned)")
        self.heal_masternodes(33)

        self.log.info("Lets lie in the contribution and then also lie in the justification")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'justify-omit', '0')
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'justify-lie', '100')
        qh = self.mine_quorum(expected_contributions=3, expected_complaints=2, expected_justifications=1, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, False)

        self.log.info("Lets lie about another MN")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'contribution-lie', '0')
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'justify-lie', '0')
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'complain-lie', '100')
        qh = self.mine_quorum(expected_contributions=3, expected_complaints=1, expected_justifications=2, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        self.log.info("Lets omit 1 premature commitments")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'complain-lie', '0')
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'commit-omit', '100')
        qh = self.mine_quorum(expected_contributions=3, expected_complaints=0, expected_justifications=0, expected_commitments=2, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

        self.log.info("Lets lie in 1 premature commitments")
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'commit-omit', '0')
        self.mninfo[0].get_node(self).quorum('dkgsimerror', 'commit-lie', '100')
        qh = self.mine_quorum(expected_contributions=3, expected_complaints=0, expected_justifications=0, expected_commitments=2, mninfos_valid=mninfos_valid)
        self.assert_member_valid(qh, self.mninfo[0].proTxHash, True)

    def assert_member_valid(self, quorumHash, proTxHash, expectedValid):
        q = self.nodes[0].quorum('info', 100, quorumHash, True)
        for m in q['members']:
            if m['proTxHash'] == proTxHash:
                if expectedValid:
                    assert m['valid']
                else:
                    assert not m['valid']
            else:
                assert m['valid']

    def heal_masternodes(self, blockCount):
        # We're not testing PoSe here, so lets heal the MNs :)
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        for _ in range(blockCount):
            self.bump_mocktime(1)
            self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        self.sync_all()
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()


if __name__ == '__main__':
    LLMQDKGErrors().main()
