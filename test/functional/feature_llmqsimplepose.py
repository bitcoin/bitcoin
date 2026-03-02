#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time

from test_framework.test_framework import DashTestFramework
from test_framework.util import force_finish_mnsync, p2p_port
from test_framework.masternodes import check_banned, check_punished

'''
feature_llmqsimplepose.py

Checks simple PoSe system based on LLMQ commitments

'''


class LLMQSimplePoSeTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.bind_to_localhost_only = False
        self.set_dash_test_params(6, 5, fast_dip3_enforcement=True)
        self.set_dash_llmq_test_params(5, 3)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def add_options(self, parser):
        parser.add_argument("--disable-spork23", dest="disable_spork23", default=False, action="store_true",
                            help="Test with spork23 disabled")

    def run_test(self):
        if self.options.disable_spork23:
            self.nodes[0].spork("SPORK_23_QUORUM_POSE", 4070908800)
        else:
            self.nodes[0].spork("SPORK_23_QUORUM_POSE", 0)
        self.deaf_mns = []
        self.sync_blocks(self.nodes, timeout=60)
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # Lets isolate MNs one by one and verify that punishment/banning happens
        self.test_banning(self.isolate_mn, 2)

        self.repair_masternodes(False)

        self.nodes[0].spork("SPORK_21_QUORUM_ALL_CONNECTED", 0)
        self.wait_for_sporks_same()

        self.reset_probe_timeouts()

        if not self.options.disable_spork23:
            # Lets restart masternodes with closed ports and verify that they get banned even though they are connected to other MNs (via outbound connections)
            self.test_banning(self.close_mn_port)
        else:
            # With PoSe off there should be no punishing for non-reachable nodes
            self.test_no_banning(self.close_mn_port, 3)


        self.deaf_mns.clear()
        self.repair_masternodes(True)

        if not self.options.disable_spork23:
            self.test_banning(self.force_old_mn_proto, 3)
        else:
            # With PoSe off there should be no punishing for outdated nodes
            self.test_no_banning(self.force_old_mn_proto, 3)

    def isolate_mn(self, mn):
        mn.node.setnetworkactive(False)
        self.wait_until(lambda: mn.node.getconnectioncount() == 0)
        return True, True

    def close_mn_port(self, mn):
        self.deaf_mns.append(mn)
        self.stop_node(mn.node.index)
        self.start_masternode(mn, extra_args=["-mocktime=" + str(self.mocktime), "-listen=0"])
        self.connect_nodes(mn.node.index, 0)
        # Make sure the to-be-banned node is still connected well via outbound connections
        for mn2 in self.mninfo:
            if self.deaf_mns.count(mn2) == 0:
                self.connect_nodes(mn.node.index, mn2.node.index)
        self.reset_probe_timeouts()
        return False, False

    def force_old_mn_proto(self, mn):
        self.stop_node(mn.node.index)
        self.start_masternode(mn, extra_args=["-mocktime=" + str(self.mocktime), "-pushversion=70015"])
        self.connect_nodes(mn.node.index, 0)
        self.reset_probe_timeouts()
        return False, True

    def test_no_banning(self, invalidate_proc, expected_connections=None):
        invalidate_proc(self.mninfo[0])
        for i in range(3):
            self.mine_quorum(expected_connections=expected_connections)

    def test_banning(self, invalidate_proc, expected_connections=None):
        mninfos_online = self.mninfo.copy()
        mninfos_valid = self.mninfo.copy()

        for mn in mninfos_valid:
            assert not check_punished(self.nodes[0], mn)
            assert not check_banned(self.nodes[0], mn)

        expected_contributors = len(mninfos_online)
        for i in range(2):
            self.log.info(f"Testing PoSe banning due to {invalidate_proc.__name__} {i + 1}/2")
            mn = mninfos_valid.pop()
            went_offline, instant_ban = invalidate_proc(mn)
            expected_complaints = expected_contributors - 1
            if went_offline:
                mninfos_online.remove(mn)
                expected_contributors -= 1

            # NOTE: Min PoSe penalty is 100 (see CDeterministicMNList::CalcMaxPoSePenalty()),
            # so nodes are PoSe-banned in the same DKG they misbehave without being PoSe-punished first.
            if instant_ban:
                assert expected_connections is not None
                self.log.info("Expecting instant PoSe banning")
                self.reset_probe_timeouts()
                self.mine_quorum(expected_connections=expected_connections, expected_members=expected_contributors, expected_contributions=expected_contributors, expected_complaints=expected_complaints, expected_commitments=expected_contributors, mninfos_online=mninfos_online, mninfos_valid=mninfos_valid)

                if not check_banned(self.nodes[0], mn):
                    self.log.info("Instant ban still requires 2 missing DKG round. If it is not banned yet, mine 2nd one")
                    self.reset_probe_timeouts()
                    self.mine_quorum(expected_connections=expected_connections, expected_members=expected_contributors, expected_contributions=expected_contributors, expected_complaints=expected_complaints, expected_commitments=expected_contributors, mninfos_online=mninfos_online, mninfos_valid=mninfos_valid)
            else:
                # It's ok to miss probes/quorum connections up to 6 times.
                # 7th time is when it should be banned for sure.
                assert expected_connections is None
                for j in range(7):
                    self.log.info(f"Accumulating PoSe penalty {j + 1}/7")
                    self.reset_probe_timeouts()
                    self.mine_quorum(expected_connections=0, expected_members=expected_contributors - 1, expected_contributions=0, expected_complaints=0, expected_commitments=0, mninfos_online=mninfos_online, mninfos_valid=mninfos_valid)

            assert check_banned(self.nodes[0], mn)

            if not went_offline:
                # we do not include PoSe banned mns in quorums, so the next one should have 1 contributor less
                expected_contributors -= 1

    def repair_masternodes(self, restart):
        # Repair all nodes
        for mn in self.mninfo:
            if check_banned(self.nodes[0], mn) or check_punished(self.nodes[0], mn):
                addr = self.nodes[0].getnewaddress()
                self.nodes[0].sendtoaddress(addr, 0.1)
                self.nodes[0].protx_update_service(mn.proTxHash, '127.0.0.1:%d' % p2p_port(mn.node.index), mn.keyOperator, "", addr)
                if restart:
                    self.stop_node(mn.node.index)
                    self.start_masternode(mn, extra_args=["-mocktime=" + str(self.mocktime)])
                else:
                    mn.node.setnetworkactive(True)
                self.connect_nodes(mn.node.index, 0, wait_for_connect=False)
        self.sync_blocks()

        self.bump_mocktime(60 * 10 + 1)
        self.generate(self.nodes[0], 1)
        
        # Isolate and re-connect all MNs (otherwise there might be open connections with no MNAUTH for MNs which were banned before)
        for mn in self.mninfo:
            assert not check_banned(self.nodes[0], mn)
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


if __name__ == '__main__':
    LLMQSimplePoSeTest().main()
