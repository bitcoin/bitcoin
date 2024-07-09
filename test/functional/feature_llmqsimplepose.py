#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.test_framework import DashTestFramework
from test_framework.util import force_finish_mnsync, p2p_port


'''
feature_llmqsimplepose.py

Checks simple PoSe system based on LLMQ commitments

'''


class LLMQSimplePoSeTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.bind_to_localhost_only = False
        self.set_dash_test_params(6, 5, [["-whitelist=noban@127.0.0.1"]] * 6, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def run_test(self):
        self.sync_blocks(self.nodes, timeout=60*5)
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # check if mining quorums with all nodes being online succeeds without punishment/banning
        self.test_no_banning()

        # Now lets isolate MNs one by one and verify that punishment/banning happens
        self.test_banning(self.isolate_mn, 1)

        self.repair_masternodes(True)

        self.nodes[0].spork("SPORK_21_QUORUM_ALL_CONNECTED", 0)
        self.nodes[0].spork("SPORK_23_QUORUM_POSE", 0)
        self.bump_mocktime(60)
        self.generate(self.nodes[0], 1)
        self.wait_for_sporks_same()

        self.reset_probe_timeouts()

        # Make sure no banning happens with spork21 enabled
        self.test_no_banning()
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        # Lets restart masternodes with closed ports and verify that they get banned even though they are connected to other MNs (via outbound connections)
        self.test_banning(self.close_mn_port, 3)

        self.repair_masternodes(True)
        self.reset_probe_timeouts()
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        self.test_banning(self.force_old_mn_proto, 3)

        # With PoSe off there should be no punishing for non-reachable and outdated nodes
        self.nodes[0].spork("SPORK_23_QUORUM_POSE", 4070908800)
        self.wait_for_sporks_same()

        self.repair_masternodes(True)
        self.force_old_mn_proto(self.mninfo[0])
        self.test_no_banning(3)

        self.repair_masternodes(True)
        self.close_mn_port(self.mninfo[0])
        self.test_no_banning(3)

    def isolate_mn(self, mn):
        mn.node.setnetworkactive(False)
        self.wait_until(lambda: mn.node.getconnectioncount() == 0)
        return True, True

    def close_mn_port(self, mn):
        self.stop_node(mn.node.index)
        self.start_masternode(mn, extra_args=["-mocktime=" + str(self.mocktime), "-listen=0"])
        self.connect_nodes(mn.node.index, 0)
        # Make sure the to-be-banned node is still connected well via outbound connections
        for mn2 in self.mninfo:
            if mn2 is not mn:
                self.connect_nodes(mn.node.index, mn2.node.index, wait_for_connect=False)
        self.reset_probe_timeouts()
        return False, False

    def force_old_mn_proto(self, mn):
        self.stop_node(mn.node.index)
        self.start_masternode(mn, extra_args=["-mocktime=" + str(self.mocktime), "-pushversion=70015"])
        self.connect_nodes(mn.node.index, 0)
        self.reset_probe_timeouts()
        return False, True

    def test_no_banning(self, expected_connections=None):
        for i in range(3):
            self.mine_quorum(expected_connections=expected_connections)
        for mn in self.mninfo:
            assert not self.check_punished(mn) and not self.check_banned(mn)

    def test_banning(self, invalidate_proc, expected_connections):
        mninfos_online = self.mninfo.copy()
        mninfos_valid = self.mninfo.copy()
        expected_contributors = len(mninfos_online)
        for i in range(2):
            mn = mninfos_valid.pop()
            went_offline, instant_ban = invalidate_proc(mn)
            if went_offline:
                mninfos_online.remove(mn)
                expected_contributors -= 1

            # NOTE: Min PoSe penalty is 100 (see CDeterministicMNList::CalcMaxPoSePenalty()),
            # so nodes are PoSe-banned in the same DKG they misbehave without being PoSe-punished first.
            if not instant_ban:
                # it's ok to miss probes/quorum connections up to 5 times
                for i in range(5):
                    self.reset_probe_timeouts()
                    self.mine_quorum(expected_connections=expected_connections, expected_members=expected_contributors, expected_contributions=expected_contributors, expected_complaints=0, expected_commitments=expected_contributors, mninfos_online=mninfos_online, mninfos_valid=mninfos_valid)
                    assert not self.check_banned(mn)
            self.reset_probe_timeouts()
            self.mine_quorum(expected_connections=expected_connections, expected_members=expected_contributors, expected_contributions=expected_contributors, expected_complaints=expected_contributors-1, expected_commitments=expected_contributors, mninfos_online=mninfos_online, mninfos_valid=mninfos_valid)
            self.mine_quorum(expected_connections=expected_connections, expected_members=expected_contributors, expected_contributions=expected_contributors, expected_complaints=expected_contributors-1, expected_commitments=expected_contributors, mninfos_online=mninfos_online, mninfos_valid=mninfos_valid)
            assert self.check_banned(mn)

            if not went_offline:
                # we do not include PoSe banned mns in quorums, so the next one should have 1 contributor less
                expected_contributors -= 1

    def repair_masternodes(self, restart):
        # Repair all nodes
        for mn in self.mninfo:
            if self.check_banned(mn) or self.check_punished(mn):
                addr = self.nodes[0].getnewaddress()
                self.nodes[0].sendtoaddress(addr, 0.1)
                self.nodes[0].protx_update_service(mn.proTxHash, '127.0.0.1:%d' % p2p_port(mn.node.index), mn.keyOperator, "", addr)
                # Make sure this tx "safe" to mine even when ChainLocks are no longer functional
                self.bump_mocktime(60 * 10 + 1)
                self.generate(self.nodes[0], 1, sync_fun=self.no_op)
                assert not self.check_banned(mn)

                if restart:
                    self.stop_node(mn.node.index)
                    time.sleep(0.5)
                    self.start_masternode(mn, extra_args=["-mocktime=" + str(self.mocktime)])
                else:
                    mn.node.setnetworkactive(True)
                self.connect_nodes(mn.node.index, 0, wait_for_connect=False)
        self.sync_all()

        # Isolate and re-connect all MNs (otherwise there might be open connections with no MNAUTH for MNs which were banned before)
        for mn in self.mninfo:
            mn.node.setnetworkactive(False)
            self.wait_until(lambda: mn.node.getconnectioncount() == 0)
            mn.node.setnetworkactive(True)
            force_finish_mnsync(mn.node)
            self.connect_nodes(mn.node.index, 0, wait_for_connect=False)

    def reset_probe_timeouts(self):
        # Make sure all masternodes will reconnect/re-probe
        self.bump_mocktime(10 * 60 + 1)
        # Sleep a couple of seconds to let mn sync tick to happen
        time.sleep(2)

    def check_punished(self, mn):
        info = self.nodes[0].protx_info(mn.proTxHash)
        if info['state']['PoSePenalty'] > 0:
            return True
        return False

    def check_banned(self, mn):
        info = self.nodes[0].protx_info(mn.proTxHash)
        if info['state']['PoSeBanHeight'] != -1:
            return True
        return False

if __name__ == '__main__':
    LLMQSimplePoSeTest().main()
