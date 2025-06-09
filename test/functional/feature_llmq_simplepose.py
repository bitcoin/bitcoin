#!/usr/bin/env python3
# Copyright (c) 2015-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_simplepose.py

Checks simple PoSe system based on LLMQ commitments

'''

import time

from test_framework.masternodes import check_banned, check_punished
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import assert_equal, force_finish_mnsync

class LLMQSimplePoSeTest(DashTestFramework):
    def set_test_params(self):
        # rotating quorums add instability for this functional tests
        self.extra_args = [[ '-testactivationheight=dip0024@9999' ]] * 6
        self.set_dash_test_params(6, 5)
        self.set_dash_llmq_test_params(5, 3)
        self.delay_v20_and_mn_rr(height=9999)

    def add_options(self, parser):
        parser.add_argument("--disable-spork23", dest="disable_spork23", default=False, action="store_true",
                            help="Test with spork23 disabled")

    def run_test(self):
        if self.options.disable_spork23:
            self.nodes[0].sporkupdate("SPORK_23_QUORUM_POSE", 4070908800)
        else:
            self.nodes[0].sporkupdate("SPORK_23_QUORUM_POSE", 0)

        self.deaf_mns = []
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # Lets isolate MNs one by one and verify that punishment/banning happens
        self.test_banning(self.isolate_mn, 2)

        self.repair_masternodes(False)

        self.nodes[0].sporkupdate("SPORK_21_QUORUM_ALL_CONNECTED", 0)
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

    def isolate_mn(self, mn: MasternodeInfo):
        mn.node.setnetworkactive(False)
        self.wait_until(lambda: mn.node.getconnectioncount() == 0)
        return True, True

    def close_mn_port(self, mn: MasternodeInfo):
        self.deaf_mns.append(mn)
        self.stop_node(mn.node.index)
        self.start_masternode(mn, ["-listen=0", "-nobind"])
        self.connect_nodes(mn.node.index, 0)
        # Make sure the to-be-banned node is still connected well via outbound connections
        for mn2 in self.mninfo: # type: MasternodeInfo
            if self.deaf_mns.count(mn2) == 0:
                self.connect_nodes(mn.node.index, mn2.node.index)
        self.reset_probe_timeouts()
        return False, False

    def force_old_mn_proto(self, mn: MasternodeInfo):
        self.stop_node(mn.node.index)
        self.start_masternode(mn, ["-pushversion=70216"])
        self.connect_nodes(mn.node.index, 0)
        self.reset_probe_timeouts()
        return False, True

    def test_no_banning(self, invalidate_proc, expected_connections=None):
        invalidate_proc(self.mninfo[0])
        for i in range(3):
            self.log.info(f"Testing no PoSe banning in normal conditions {i + 1}/3")
            self.mine_quorum(expected_connections=expected_connections)

    def mine_quorum_less_checks(self, expected_good_nodes, mninfos_online):
        # Unlike in mine_quorum we skip most of the checks and only care about
        # nodes moving forward from phase to phase correctly and the fact that the quorum is actually mined.
        self.log.info("Mining a quorum with less checks")
        nodes = [self.nodes[0]] + [mn.node for mn in mninfos_online]

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        if skip_count != 0:
            self.bump_mocktime(skip_count, nodes=nodes)
            self.generate(self.nodes[0], skip_count, sync_fun=lambda: self.sync_blocks(nodes))

        q = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_hash: "+str(q))
        self.log.info("Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q, 1, expected_good_nodes, None, 0, mninfos_online)
        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 2 (contribute)")
        self.wait_for_quorum_phase(q, 2, expected_good_nodes, "receivedContributions", expected_good_nodes, mninfos_online)
        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 3 (complain)")
        self.wait_for_quorum_phase(q, 3, expected_good_nodes, None, 0, mninfos_online)
        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 4 (justify)")
        self.wait_for_quorum_phase(q, 4, expected_good_nodes, None, 0, mninfos_online)
        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 5 (commit)")
        self.wait_for_quorum_phase(q, 5, expected_good_nodes, "receivedPrematureCommitments", expected_good_nodes, mninfos_online)
        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 6 (mining)")
        self.wait_for_quorum_phase(q, 6, expected_good_nodes, None, 0, mninfos_online)

        self.log.info("Waiting final commitment")
        self.wait_for_quorum_commitment(q, nodes)

        self.log.info("Mining final commitment")
        self.bump_mocktime(1, nodes=nodes)
        self.nodes[0].getblocktemplate() # this calls CreateNewBlock
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(nodes))

        self.log.info("Waiting for quorum to appear in the list")
        self.wait_for_quorum_list(q, nodes)

        new_quorum = self.nodes[0].quorum("list", 1)["llmq_test"][0]
        assert_equal(q, new_quorum)
        quorum_info = self.nodes[0].quorum("info", 100, new_quorum)

        # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligible for signing sessions
        self.bump_mocktime(8)
        self.generate(self.nodes[0], 8, sync_fun=lambda: self.sync_blocks(nodes))
        self.log.info("New quorum: height=%d, quorumHash=%s, quorumIndex=%d, minedBlock=%s" % (quorum_info["height"], new_quorum, quorum_info["quorumIndex"], quorum_info["minedBlock"]))

        return new_quorum

    def test_banning(self, invalidate_proc, expected_connections=None):
        mninfos_online = self.mninfo.copy()
        mninfos_valid = self.mninfo.copy()

        for mn in mninfos_valid:
            assert not check_punished(self.nodes[0], mn)
            assert not check_banned(self.nodes[0], mn)

        expected_contributors = len(mninfos_online)
        for i in range(2):
            self.log.info(f"Testing PoSe banning due to {invalidate_proc.__name__} {i + 1}/2")
            mn: MasternodeInfo = mninfos_valid.pop()
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
                # It's ok to miss probes/quorum connections up to 5 times.
                # 6th time is when it should be banned for sure.
                assert expected_connections is None
                for j in range(6):
                    self.log.info(f"Accumulating PoSe penalty {j + 1}/6")
                    self.reset_probe_timeouts()
                    self.mine_quorum_less_checks(expected_contributors - 1, mninfos_online)

            assert check_banned(self.nodes[0], mn)

            if not went_offline:
                # we do not include PoSe banned mns in quorums, so the next one should have 1 contributor less
                expected_contributors -= 1

    def repair_masternodes(self, restart):
        self.log.info("Repairing all banned and punished masternodes")
        for mn in self.mninfo: # type: MasternodeInfo
            if check_banned(self.nodes[0], mn) or check_punished(self.nodes[0], mn):
                addr = self.nodes[0].getnewaddress()
                self.nodes[0].sendtoaddress(addr, 0.1)
                self.nodes[0].protx('update_service', mn.proTxHash, f'127.0.0.1:{mn.nodePort}', mn.keyOperator, "", addr)
                if restart:
                    self.stop_node(mn.node.index)
                    self.start_masternode(mn)
                else:
                    mn.node.setnetworkactive(True)
                self.connect_nodes(mn.node.index, 0)

        # syncing blocks only since node 0 has txes waiting to be mined
        self.sync_blocks()

        # Make sure protxes are "safe" to mine even when InstantSend and ChainLocks are no longer functional
        self.bump_mocktime(60 * 10 + 1)
        self.generate(self.nodes[0], 1)

        # Isolate and re-connect all MNs (otherwise there might be open connections with no MNAUTH for MNs which were banned before)
        for mn in self.mninfo: # type: MasternodeInfo
            assert not check_banned(self.nodes[0], mn)
            mn.node.setnetworkactive(False)
            self.wait_until(lambda: mn.node.getconnectioncount() == 0)
            mn.node.setnetworkactive(True)
            force_finish_mnsync(mn.node)
            self.connect_nodes(mn.node.index, 0)

    def reset_probe_timeouts(self):
        # Make sure all masternodes will reconnect/re-probe
        self.bump_mocktime(10 * 60 + 1)
        # Sleep a couple of seconds to let mn sync tick to happen
        time.sleep(2)


if __name__ == '__main__':
    LLMQSimplePoSeTest().main()
