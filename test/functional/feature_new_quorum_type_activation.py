#!/usr/bin/env python3
# Copyright (c) 2020-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, get_bip9_details

'''
feature_new_quorum_type_activation.py

Tests the activation of a new quorum type in v17 via a bip9-like hardfork

'''


class NewQuorumTypeActivationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-vbparams=testdummy:0:999999999999:0:10:8:6:5:0"]]

    def run_test(self):
        self.log.info(get_bip9_details(self.nodes[0], 'testdummy'))
        assert_equal(get_bip9_details(self.nodes[0], 'testdummy')['status'], 'defined')
        self.generate(self.nodes[0], 9, sync_fun=self.no_op)
        assert_equal(get_bip9_details(self.nodes[0], 'testdummy')['status'], 'started')
        ql = self.nodes[0].quorum("list")
        assert_equal(len(ql), 3)
        assert "llmq_test_v17" not in ql
        self.generate(self.nodes[0], 10, sync_fun=self.no_op)
        assert_equal(get_bip9_details(self.nodes[0], 'testdummy')['status'], 'locked_in')
        ql = self.nodes[0].quorum("list")
        assert_equal(len(ql), 3)
        assert "llmq_test_v17" not in ql
        self.generate(self.nodes[0], 10, sync_fun=self.no_op)
        assert_equal(get_bip9_details(self.nodes[0], 'testdummy')['status'], 'active')
        ql = self.nodes[0].quorum("list")
        assert_equal(len(ql), 4)
        assert "llmq_test_v17" in ql


if __name__ == '__main__':
    NewQuorumTypeActivationTest().main()
