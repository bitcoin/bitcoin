#!/usr/bin/env python3
# Copyright (c) 2015-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

'''
feature_llmq_signing.py

Checks LLMQs signing sessions

'''

from test_framework.messages import CSigShare, msg_qsigshare, uint256_to_string
from test_framework.p2p import P2PInterface
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import assert_equal, assert_raises_rpc_error, force_finish_mnsync


q_type=100
class LLMQSigningTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(6, 5)
        self.set_dash_llmq_test_params(5, 3)
        for args in self.extra_args:
            args.append("-llmqtestplatformparams=3:2")

    def add_options(self, parser):
        self.add_wallet_options(parser)
        parser.add_argument("--spork21", dest="spork21", default=False, action="store_true",
                            help="Test with spork21 enabled")

    def run_test(self):

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        if self.options.spork21:
            self.nodes[0].sporkupdate("SPORK_21_QUORUM_ALL_CONNECTED", 0)
        self.wait_for_sporks_same()

        self.mine_quorum()

        if self.options.spork21:
            assert self.mninfo[0].get_node(self).getconnectioncount() == self.llmq_size
            self.assert_qsendrecsigs_symmetric()

        id = "0000000000000000000000000000000000000000000000000000000000000001"
        msgHash = "0000000000000000000000000000000000000000000000000000000000000002"
        msgHashConflict = "0000000000000000000000000000000000000000000000000000000000000003"

        def check_sigs(hasrecsigs, isconflicting1, isconflicting2):
            for mn in self.mninfo: # type: MasternodeInfo
                if mn.get_node(self).quorum("hasrecsig", q_type, id, msgHash) != hasrecsigs:
                    return False
                if mn.get_node(self).quorum("isconflicting", q_type, id, msgHash) != isconflicting1:
                    return False
                if mn.get_node(self).quorum("isconflicting", q_type, id, msgHashConflict) != isconflicting2:
                    return False
            return True

        def wait_for_sigs(hasrecsigs, isconflicting1, isconflicting2, timeout):
            self.wait_until(lambda: check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout)

        def assert_sigs_nochange(hasrecsigs, isconflicting1, isconflicting2, timeout):
            assert not self.wait_until(lambda: not check_sigs(hasrecsigs, isconflicting1, isconflicting2), timeout = timeout, do_assert = False)

        # Initial state
        wait_for_sigs(False, False, False, 1)

        # Sign first share without any optional parameter, should not result in recovered sig
        self.mninfo[0].get_node(self).quorum("sign", q_type, id, msgHash)
        assert_sigs_nochange(False, False, False, 3)
        # Sign second share and test optional quorumHash parameter, should not result in recovered sig
        # 1. Providing an invalid quorum hash should fail and cause no changes for sigs
        assert not self.mninfo[1].get_node(self).quorum("sign", q_type, id, msgHash, msgHash)
        assert_sigs_nochange(False, False, False, 3)
        # 2. Providing a valid quorum hash should succeed and cause no changes for sigss
        quorumHash = self.mninfo[1].get_node(self).quorum("selectquorum", q_type, id)["quorumHash"]
        assert self.mninfo[1].get_node(self).quorum("sign", q_type, id, msgHash, quorumHash)
        assert_sigs_nochange(False, False, False, 3)
        # Sign third share and test optional submit parameter if spork21 is enabled, should result in recovered sig
        # and conflict for msgHashConflict
        if self.options.spork21:
            # 1. Providing an invalid quorum hash and set submit=false, should throw an error
            assert_raises_rpc_error(-8, 'quorum not found', self.mninfo[2].get_node(self).quorum, "sign", q_type, id, msgHash, id, False)
            # 2. Providing a valid quorum hash and set submit=false, should return a valid sigShare object
            sig_share_rpc_1 = self.mninfo[2].get_node(self).quorum("sign", q_type, id, msgHash, quorumHash, False)
            sig_share_rpc_2 = self.mninfo[2].get_node(self).quorum("sign", q_type, id, msgHash, "", False)
            assert_equal(sig_share_rpc_1, sig_share_rpc_2)
            assert_sigs_nochange(False, False, False, 3)
            # 3. Sending the sig share received from RPC to the recovery member through P2P interface, should result
            # in a recovered sig
            sig_share = CSigShare()
            sig_share.llmqType = int(sig_share_rpc_1["llmqType"])
            sig_share.quorumHash = int(sig_share_rpc_1["quorumHash"], 16)
            sig_share.quorumMember = int(sig_share_rpc_1["quorumMember"])
            sig_share.id = int(sig_share_rpc_1["id"], 16)
            sig_share.msgHash = int(sig_share_rpc_1["msgHash"], 16)
            sig_share.sigShare = bytes.fromhex(sig_share_rpc_1["signature"])
            for mn in self.mninfo: # type: MasternodeInfo
                assert mn.get_node(self).getconnectioncount() == self.llmq_size
            # Get the current recovery member of the quorum
            q = self.nodes[0].quorum('selectquorum', q_type, id)
            mn: MasternodeInfo = self.get_mninfo(q['recoveryMembers'][0])
            # Open a P2P connection to it
            p2p_interface = mn.get_node(self).add_p2p_connection(P2PInterface())
            # Send the last required QSIGSHARE message to the recovery member
            p2p_interface.send_message(msg_qsigshare([sig_share]))
        else:
            # If spork21 is not enabled just sign regularly
            self.mninfo[2].get_node(self).quorum("sign", q_type, id, msgHash)

        wait_for_sigs(True, False, True, 15)

        if self.options.spork21:
            mn.get_node(self).disconnect_p2ps()

        # Test `quorum verify` rpc
        node = self.mninfo[0].get_node(self)
        recsig = node.quorum("getrecsig", q_type, id, msgHash)
        # Find quorum automatically
        height = node.getblockcount()
        height_bad = node.getblockheader(recsig["quorumHash"])["height"]
        hash_bad = node.getblockhash(0)
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"])
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"], "", height)
        assert not node.quorum("verify", q_type, id, msgHashConflict, recsig["sig"])
        assert not node.quorum("verify", q_type, id, msgHash, recsig["sig"], "", height_bad)
        # Use specific quorum
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"], recsig["quorumHash"])
        assert not node.quorum("verify", q_type, id, msgHashConflict, recsig["sig"], recsig["quorumHash"])
        assert_raises_rpc_error(-8, "quorum not found", node.quorum, "verify", q_type, id, msgHash, recsig["sig"], hash_bad)

        # The quorum type must not be truncated to 8 bits
        for bad_type in [q_type - 256, q_type + 256]:
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "hasrecsig", bad_type, id, msgHash)
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "getrecsig", bad_type, id, msgHash)
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "verify", bad_type, id, msgHash, recsig["sig"])
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "info", bad_type, recsig["quorumHash"])
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "sign", bad_type, "not-a-hash", msgHash)
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "isconflicting", bad_type, id, msgHash)
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "selectquorum", bad_type, id)
            assert_raises_rpc_error(-1, "JSON integer out of range", node.quorum, "getdata", 0, bad_type, recsig["quorumHash"], 1)

        # Mine one more quorum, so that we have 2 active ones, nothing should change
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)

        # Create a recovered sig for the oldest quorum i.e. the active quorum which will be moved
        # out of the active set when a new quorum appears
        request_id = 2
        oldest_quorum_hash = node.quorum("list")["llmq_test"][-1]
        # Search for a request id which selects the last active quorum
        while True:
            selected_hash = node.quorum('selectquorum', q_type, uint256_to_string(request_id))["quorumHash"]
            if selected_hash == oldest_quorum_hash:
                break
            else:
                request_id += 1
        # Produce the recovered signature
        id = uint256_to_string(request_id)
        for mn in self.mninfo: # type: MasternodeInfo
            mn.get_node(self).quorum("sign", q_type, id, msgHash)
        # And mine a quorum to move the quorum which signed out of the active set
        self.mine_quorum()
        # Verify the recovered sig. This triggers the "signHeight + dkgInterval" verification
        recsig = node.quorum("getrecsig", q_type, id, msgHash)
        assert node.quorum("verify", q_type, id, msgHash, recsig["sig"], "", node.getblockcount())

        recsig_time = self.mocktime

        # Mine 2 more quorums, so that the one used for the the recovered sig should become inactive, nothing should change
        self.mine_quorum()
        self.mine_quorum()
        assert_sigs_nochange(True, False, True, 3)

        # fast forward until 0.5 days before cleanup is expected, recovered sig should still be valid
        self.bump_mocktime(recsig_time + int(60 * 60 * 24 * 6.5) - self.mocktime, update_schedulers=False)
        # Cleanup starts every 5 seconds
        wait_for_sigs(True, False, True, 15)
        # fast forward 1 day, recovered sig should not be valid anymore
        self.bump_mocktime(int(60 * 60 * 24 * 1), update_schedulers=False)
        # Cleanup starts every 5 seconds
        wait_for_sigs(False, False, False, 15)

        for i in range(2):
            self.mninfo[i].get_node(self).quorum("sign", q_type, id, msgHashConflict)
        for i in range(2, 5):
            self.mninfo[i].get_node(self).quorum("sign", q_type, id, msgHash)
        wait_for_sigs(True, False, True, 15)

        if self.options.spork21:
            id = uint256_to_string(request_id + 1)

            # Isolate the node that is responsible for the recovery of a signature. The other members are not
            # connected to it, so they send their shares to the next recovery member straight away instead of
            # waiting for the retry timeout (mocktime is not advanced here).
            q = self.nodes[0].quorum('selectquorum', q_type, id)
            mn: MasternodeInfo = self.get_mninfo(q['recoveryMembers'][0])
            mn.get_node(self).setnetworkactive(False)
            self.wait_until(lambda: mn.get_node(self).getconnectioncount() == 0)
            self.wait_until(lambda: all(peer.get('verified_proregtx_hash') != mn.proTxHash
                                        for other in self.mninfo if other is not mn
                                        for peer in other.get_node(self).getpeerinfo()))
            for i in range(4):
                self.mninfo[i].get_node(self).quorum("sign", q_type, id, msgHash)
            self.wait_until(lambda: all(other.get_node(self).quorum("hasrecsig", q_type, id, msgHash)
                                        for other in self.mninfo if other is not mn), timeout=10)
            assert not mn.get_node(self).quorum("hasrecsig", q_type, id, msgHash)
            mn.get_node(self).setnetworkactive(True)
            self.connect_nodes(mn.nodeIdx, 0)
            force_finish_mnsync(mn.get_node(self))
            # Make sure intra-quorum connections were also restored
            self.bump_mocktime(1)  # need this to bypass quorum connection retry timeout
            self.wait_until(lambda: all(other.get_node(self).getconnectioncount() == self.llmq_size
                                        for other in self.mninfo), timeout=10)
            mn.get_node(self).ping()
            self.wait_until(lambda: all('pingwait' not in peer for peer in mn.get_node(self).getpeerinfo()))
            # Without this the platform DKG below intermittently marks a member bad
            self.bump_mocktime(2)

        self.test_platform_resigning()

    def test_platform_resigning(self):
        self.log.info("Re-sign Platform messages using a member that only received the first signature")
        for _ in range(3):
            self.dynamically_add_masternode(evo=True)
        self.mine_quorum(llmq_type_name='llmq_test_platform', expected_members=3, expected_connections=2,
                         expected_contributions=3, expected_commitments=3, llmq_type=106)
        request_id = uint256_to_string(100)
        first_message = uint256_to_string(101)
        second_message = uint256_to_string(102)
        quorum_hash = self.nodes[0].quorum('selectquorum', 106, request_id)['quorumHash']
        members = [mn.get_node(self) for mn in self.get_quorum_masternodes(quorum_hash, llmq_type=106)]

        def wait_for_message(message):
            def all_members_have_signature():
                self.bump_mocktime(1)
                return all(node.quorum('hasrecsig', 106, request_id, message) for node in members)
            self.wait_until(all_members_have_signature, sleep=1)

        for node in members[:2]:
            assert node.quorum('platformsign', request_id, first_message, quorum_hash)
        wait_for_message(first_message)
        first_sig = members[2].quorum('getrecsig', 106, request_id, first_message)

        # The third member learned the signature through relay and has never voted on this id.
        for node in members[1:]:
            assert node.quorum('platformsign', request_id, second_message, quorum_hash)
        wait_for_message(second_message)
        for node in members:
            assert_equal(node.quorum('getrecsig', 106, request_id, first_message), first_sig)
            assert_equal(node.quorum('getrecsig', 106, request_id, second_message)['msgHash'], second_message)

    def assert_qsendrecsigs_symmetric(self):
        # If only one direction's QSENDRECSIGS arrives, the receiving side keeps
        # m_wants_recsigs=false and silently drops half of all recsig pushes.
        self.log.info("Assert QSENDRECSIGS was exchanged in both directions on every MN-MN edge")
        mn_protxs = {mn.proTxHash for mn in self.mninfo}

        def all_symmetric():
            for mn in self.mninfo:
                for p in mn.get_node(self).getpeerinfo():
                    if p.get("verified_proregtx_hash", "") not in mn_protxs:
                        continue
                    if p.get("bytessent_per_msg", {}).get("qsendrecsigs", 0) == 0:
                        return False
                    if p.get("bytesrecv_per_msg", {}).get("qsendrecsigs", 0) == 0:
                        return False
            return True

        self.wait_until(all_symmetric, timeout=10)


if __name__ == '__main__':
    LLMQSigningTest().main()
