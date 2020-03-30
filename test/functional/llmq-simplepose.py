#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.test_framework import DashTestFramework
from test_framework.util import *

'''
llmq-simplepose.py

Checks simple PoSe system based on LLMQ commitments

'''

class LLMQSimplePoSeTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def run_test(self):

        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # check if mining quorums with all nodes being online succeeds without punishment/banning
        self.test_no_banning()

        # Now lets isolate MNs one by one and verify that punishment/banning happens
        def isolate_mn(mn):
            mn.node.setnetworkactive(False)
            wait_until(lambda: mn.node.getconnectioncount() == 0)
        self.test_banning(isolate_mn)

    def test_no_banning(self, expected_connections=1, expected_probes=0):
        for i in range(3):
            self.mine_quorum(expected_connections=expected_connections, expected_probes=expected_probes)
        for mn in self.mninfo:
            assert(not self.check_punished(mn) and not self.check_banned(mn))

    def test_banning(self, invalidate_proc):
        online_mninfos = self.mninfo.copy()
        for i in range(len(online_mninfos), len(online_mninfos) - 2, -1):
            mn = online_mninfos[len(online_mninfos) - 1]
            online_mninfos.remove(mn)
            invalidate_proc(mn)

            t = time.time()
            while (not self.check_punished(mn) or not self.check_banned(mn)) and (time.time() - t) < 120:
                self.mine_quorum(expected_connections=1, expected_members=i-1, expected_contributions=i-1, expected_complaints=i-1, expected_commitments=i-1, mninfos=online_mninfos)

            assert(self.check_punished(mn) and self.check_banned(mn))

    def check_punished(self, mn):
        info = self.nodes[0].protx('info', mn.proTxHash)
        if info['state']['PoSePenalty'] > 0:
            return True
        return False

    def check_banned(self, mn):
        info = self.nodes[0].protx('info', mn.proTxHash)
        if info['state']['PoSeBanHeight'] != -1:
            return True
        return False

if __name__ == '__main__':
    LLMQSimplePoSeTest().main()
