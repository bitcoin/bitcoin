#!/usr/bin/env python3
# Copyright (c) 2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, get_bip9_status

'''
feature_new_quorum_type_activation.py

Tests the activation of a new quorum type in v17 via a bip9-like hardfork

'''


class NewQuorumTypeActivationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-vbparams=dip0020:0:999999999999:10:8:6:5"]]

    def run_test(self):
        assert_equal(get_bip9_status(self.nodes[0], 'dip0020')['status'], 'defined')
        self.nodes[0].generate(9)
        assert_equal(get_bip9_status(self.nodes[0], 'dip0020')['status'], 'started')
        ql = self.nodes[0].quorum("list")
        assert_equal(len(ql), 1)
        assert("llmq_test_v17" not in ql)
        self.nodes[0].generate(10)
        assert_equal(get_bip9_status(self.nodes[0], 'dip0020')['status'], 'locked_in')
        ql = self.nodes[0].quorum("list")
        assert_equal(len(ql), 1)
        assert("llmq_test_v17" not in ql)
        self.nodes[0].generate(10)
        assert_equal(get_bip9_status(self.nodes[0], 'dip0020')['status'], 'active')
        ql = self.nodes[0].quorum("list")
        assert_equal(len(ql), 2)
        assert("llmq_test_v17" in ql)


if __name__ == '__main__':
    NewQuorumTypeActivationTest().main()
