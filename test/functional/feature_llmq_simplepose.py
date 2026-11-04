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
from test_framework.messages import NODE_COMPACT_FILTERS
from test_framework.p2p import P2P_SERVICES, P2PInterface
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import assert_equal, force_finish_mnsync

# See version.h
MIN_MASTERNODE_PROTO_VERSION = 70242


class MasternodePeer(P2PInterface):
    """Connects to a masternode the way another masternode does, advertising the given relay flag"""
    def __init__(self, relay):
        super().__init__()
        self.version_relay = relay

    def peer_connect_send_version(self, services):
        super().peer_connect_send_version(services)
        self.on_connection_send_msg.relay = self.version_relay
        self.on_connection_send_msg.other_masternode = True


class LLMQSimplePoSeTest(DashTestFramework):
    def set_test_params(self):
        # rotating quorums add instability for this functional tests
        self.extra_args = [[ '-testactivationheight=dip0024@9999' ]] * 6
        self.set_dash_test_params(6, 5)
        self.set_dash_llmq_test_params(5, 3)
        self.delay_v20_and_mn_rr(height=9999)

    def add_options(self, parser):
        self.add_wallet_options(parser)
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

        self.test_cloned_contributions()

        if not self.options.disable_spork23:
            # Lets isolate MNs one by one and verify that punishment/banning happens
            self.test_banning(self.isolate_mn, 2)

            self.repair_masternodes(False)
        else:
            # The contribution-miss ban path (MarkBadMember -> PoSePunish) is not
            # gated on spork23 (spork23 only gates connection/proto-version checks
            # and probes), so it behaves identically with spork23 disabled and is
            # already covered by the spork23-enabled run of this test.
            self.log.info("Skipping contribution-miss banning, not affected by spork23")
            # Mine one quorum in normal conditions so that the sections below start
            # from the same state as in the spork23-enabled run: an existing quorum
            # and all masternodes healthy.
            self.reset_probe_timeouts()
            self.mine_quorum()

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

        self.repair_masternodes(True)
        self.reset_probe_timeouts()

        for relay, services, reason in [(0, P2P_SERVICES | NODE_COMPACT_FILTERS, "does not relay transactions"),
                                        (1, P2P_SERVICES, "does not serve compact block filters")]:
            self.test_no_service(relay, services, reason, banned=not self.options.disable_spork23)
            self.repair_masternodes(True)
            self.reset_probe_timeouts()

    def test_cloned_contributions(self):
        self.log.info("Two members dealing identical contributions are both excluded and the quorum still forms")
        cloners = self.mninfo[:2]
        others = self.mninfo[2:]
        for mn in cloners:
            mn.get_node(self).quorum('dkgsimerror', 'contribution-clone', 100)
        qh = self.mine_quorum(mninfos_valid=others)
        for mn in cloners:
            mn.get_node(self).quorum('dkgsimerror', 'contribution-clone', 0)

        valid_members = {m['proTxHash'] for m in self.nodes[0].quorum('info', 100, qh)['members'] if m['valid']}
        assert_equal(valid_members, {mn.proTxHash for mn in others})
        for mn in cloners:
            assert check_punished(self.nodes[0], mn)
            assert not check_banned(self.nodes[0], mn)

        self.log.info("Let the PoSe penalty of the cloners decay without running any DKG")
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 4070908800)
        self.wait_for_sporks_same()
        self.generate(self.nodes[0], 66)
        for mn in self.mninfo:
            assert not check_punished(self.nodes[0], mn)
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

    def isolate_mn(self, mn: MasternodeInfo):
        mn.get_node(self).setnetworkactive(False)
        self.wait_until(lambda: mn.get_node(self).getconnectioncount() == 0)
        return True, True

    def close_mn_port(self, mn: MasternodeInfo):
        self.deaf_mns.append(mn)
        self.stop_node(mn.nodeIdx)
        self.start_masternode(mn, ["-listen=0", "-nobind"])
        self.connect_nodes(mn.nodeIdx, 0)
        # Make sure the to-be-banned node is still connected well via outbound connections
        for mn2 in self.mninfo: # type: MasternodeInfo
            if self.deaf_mns.count(mn2) == 0:
                self.connect_nodes(mn.nodeIdx, mn2.get_node(self).index)
        self.reset_probe_timeouts()
        return False, False

    def force_old_mn_proto(self, mn: MasternodeInfo):
        self.stop_node(mn.nodeIdx)
        self.start_masternode(mn, [f"-pushversion={MIN_MASTERNODE_PROTO_VERSION - 1}"])
        self.connect_nodes(mn.nodeIdx, 0)
        self.reset_probe_timeouts()
        return False, True

    def test_no_service(self, relay, services, reason, banned):
        # A member is judged by what our masternode connections to it advertise in their version message. A stock
        # masternode always advertises both services, so a P2P peer stands in for the connections to one member.
        mn = self.mninfo[0]
        node = mn.get_node(self)
        others = [m for m in self.mninfo if m is not mn]
        expected_complaints = len(others) if banned else 0
        for i in range(2):
            self.log.info(f"Testing PoSe {'banning' if banned else 'no banning'} of a masternode that {reason} {i + 1}/2")
            self.reset_probe_timeouts()
            with others[0].get_node(self).assert_debug_log([reason] if banned else [], unexpected_msgs=[] if banned else [reason]):
                self.mine_quorum_no_service(mn, others, relay, services, expected_complaints)
            for other in others:
                other.get_node(self).disconnect_p2ps()
            node.setnetworkactive(True)
            force_finish_mnsync(node)
            self.connect_nodes(mn.nodeIdx, 0)
            self.sync_blocks()
            if check_banned(self.nodes[0], mn):
                break
        assert_equal(check_banned(self.nodes[0], mn), banned)
        if not banned:
            assert not check_punished(self.nodes[0], mn)

    def mine_quorum_no_service(self, mn, others, relay, services, expected_complaints):
        # Like mine_quorum, except that mn goes offline once every member holds its contribution and a peer stands
        # in for it on each other member with a masternode connection that lacks the service, so that the complain
        # phase judges mn by that connection alone.
        self.log.info(f"Mining quorum with a stand-in for {mn.proTxHash}: expected_complaints={expected_complaints}")
        nodes = [self.nodes[0]] + [m.get_node(self) for m in others]
        spork23_active = self.nodes[0].spork('show')['SPORK_23_QUORUM_POSE'] <= 1

        # move forward to next DKG
        skip_count = 24 - (self.nodes[0].getblockcount() % 24)
        self.bump_mocktime(1)
        self.generate(self.nodes[0], skip_count)

        q = self.nodes[0].getbestblockhash()
        self.log.info("Expected quorum_hash:"+str(q))
        self.log.info("Waiting for phase 1 (init)")
        self.wait_for_quorum_phase(q, 1, len(others), None, 0, others)
        self.wait_for_quorum_connections(q, self.llmq_size - 1, others, wait_proc=lambda: self.bump_mocktime(1))
        if spork23_active:
            self.wait_for_masternode_probes(q, others, wait_proc=lambda: self.bump_mocktime(1))

        self.move_blocks(self.nodes, 2)

        self.log.info("Waiting for phase 2 (contribute)")
        self.wait_for_quorum_phase(q, 2, len(others), "receivedContributions", self.llmq_size, others)

        self.log.info("Replacing the connections to the masternode with stand-ins")
        mn.get_node(self).setnetworkactive(False)
        for other in others:
            other_node = other.get_node(self)
            self.wait_until(lambda: all(p.get("verified_proregtx_hash") != mn.proTxHash for p in other_node.getpeerinfo()))
            peer = other_node.add_p2p_connection(MasternodePeer(relay), services=services)
            peer_ids = [p["id"] for p in other_node.getpeerinfo() if p["subver"] == peer.strSubVer]
            assert_equal(len(peer_ids), 1)
            assert other_node.mnauth(peer_ids[0], mn.proTxHash, mn.pubKeyOperator)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 3 (complain)")
        self.wait_for_quorum_phase(q, 3, len(others), "receivedComplaints", expected_complaints, others)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 4 (justify)")
        self.wait_for_quorum_phase(q, 4, len(others), "receivedJustifications", 0, others)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 5 (commit)")
        self.wait_for_quorum_phase(q, 5, len(others), "receivedPrematureCommitments", len(others), others)

        self.move_blocks(nodes, 2)

        self.log.info("Waiting for phase 6 (mining)")
        self.wait_for_quorum_phase(q, 6, len(others), None, 0, others)

        self.log.info("Waiting final commitment")
        self.wait_for_quorum_commitment(q, others)

        self.log.info("Waiting final commitments on mining node")
        self.wait_for_quorum_commitments_on_miner(q, others)

        self.log.info("Mining final commitment")
        self.bump_mocktime(1)
        self.nodes[0].getblocktemplate() # this calls CreateNewBlock
        self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(nodes))

        self.log.info("Waiting for quorum to appear in the list")
        self.wait_for_quorum_list(q, nodes)
        assert_equal(q, self.nodes[0].quorum("list", 1)["llmq_test"][0])

        # Mine 8 (SIGN_HEIGHT_OFFSET) more blocks to make sure that the new quorum gets eligible for signing sessions
        self.generate(self.nodes[0], 8, sync_fun=lambda: self.sync_blocks(nodes))

        for m in others:
            assert not check_punished(self.nodes[0], m)
            assert not check_banned(self.nodes[0], m)

    def test_no_banning(self, invalidate_proc, expected_connections=None):
        [_, instant_ban] = invalidate_proc(self.mninfo[0])
        iters = 2 if instant_ban else 6
        for i in range(iters):
            self.log.info(f"Testing no PoSe banning in normal conditions {i + 1}/{iters}")
            self.mine_quorum(expected_connections=expected_connections)

    def mine_quorum_less_checks(self, expected_good_nodes, mninfos_online):
        # Unlike in mine_quorum we skip most of the checks and only care about
        # nodes moving forward from phase to phase correctly and the fact that the quorum is actually mined.
        self.log.info("Mining a quorum with less checks")
        nodes = [self.nodes[0]] + [mn.get_node(self) for mn in mninfos_online]

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
        self.wait_for_quorum_commitment(q, mninfos_online)

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
                    if check_banned(self.nodes[0], mn):
                        break

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
                mn.update_service(self.nodes[0], submit=True, fundsAddr=addr)
                if restart:
                    self.stop_node(mn.nodeIdx)
                    self.start_masternode(mn)
                else:
                    mn.get_node(self).setnetworkactive(True)
                self.connect_nodes(mn.nodeIdx, 0)

        # syncing blocks only since node 0 has txes waiting to be mined
        self.sync_blocks()

        # Make sure protxes are "safe" to mine even when InstantSend and ChainLocks are no longer functional
        self.bump_mocktime(60 * 10 + 1)
        self.generate(self.nodes[0], 1)

        # Isolate and re-connect all MNs (otherwise there might be open connections with no MNAUTH for MNs which were banned before)
        for mn in self.mninfo: # type: MasternodeInfo
            assert not check_banned(self.nodes[0], mn)
            mn.get_node(self).setnetworkactive(False)
            self.wait_until(lambda: mn.get_node(self).getconnectioncount() == 0)
            mn.get_node(self).setnetworkactive(True)
            force_finish_mnsync(mn.get_node(self))
            self.connect_nodes(mn.nodeIdx, 0)

    def reset_probe_timeouts(self):
        # Make sure all masternodes will reconnect/re-probe
        self.bump_mocktime(10 * 60 + 1)
        # Sleep a couple of seconds to let mn sync tick to happen
        time.sleep(2)


if __name__ == '__main__':
    LLMQSimplePoSeTest().main()
