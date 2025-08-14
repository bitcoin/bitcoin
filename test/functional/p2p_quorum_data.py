#!/usr/bin/env python3
# Copyright (c) 2021-2024 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import time

from test_framework.messages import msg_qgetdata, msg_qwatch
from test_framework.p2p import (
    p2p_lock,
    P2PInterface,
)
from test_framework.test_framework import (
    DashTestFramework,
    MasternodeInfo,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    force_finish_mnsync,
    wait_until_helper,
)

'''
p2p_quorum_data.py

Tests QGETDATA/QDATA functionality
'''

# Possible error values of QDATA
QUORUM_TYPE_INVALID = 1
QUORUM_BLOCK_NOT_FOUND = 2
QUORUM_NOT_FOUND = 3
MASTERNODE_IS_NO_MEMBER = 4
QUORUM_VERIFICATION_VECTOR_MISSING = 5
ENCRYPTED_CONTRIBUTIONS_MISSING = 6

# Used to overwrite MNAUTH for mininode connections
fake_mnauth_1 = ["cecf37bf0ec05d2d22cb8227f88074bb882b94cd2081ba318a5a444b1b15b9fd",
                 "8e7afdb849e5e2a085b035b62e21c0940c753f2d4501325743894c37162f287bccaffbedd60c36581dabbf127a22e43f"]
fake_mnauth_2 = ["6ad7ed7a2d6c2c1db30fc364114602b36b2730a9aa96d8f11f1871a9cee37378",
                 "ad38860c03c3d1d875771f41b8a9b933415f72929c21a4276c101d8f0268f6fcdfeed46507c16c00e74f26ce1181e69f"]

# Used to distinguish mininode connections
uacomment_m3_1 = "MN3_1"
uacomment_m3_2 = "MN3_2"


def assert_qdata(qdata, qgetdata, error, len_vvec=0, len_contributions=0):
    assert qdata is not None and qgetdata is not None
    assert_equal(qdata.quorum_type, qgetdata.quorum_type)
    assert_equal(qdata.quorum_hash, qgetdata.quorum_hash)
    assert_equal(qdata.data_mask, qgetdata.data_mask)
    assert_equal(qdata.protx_hash, qgetdata.protx_hash)
    assert_equal(qdata.error, error)
    assert_equal(len(qdata.quorum_vvec), len_vvec)
    assert_equal(len(qdata.enc_contributions), len_contributions)


def wait_for_banscore(node, peer_id, expected_score):
    def get_score():
        for peer in node.getpeerinfo():
            if peer["id"] == peer_id:
                if (peer["banscore"] == expected_score):
                    # The score matches the one we expected.
                    # Wait a bit to make sure it won't change
                    # to avoid false positives.
                    time.sleep(0.1)
                return peer["banscore"]
        return None
    wait_until_helper(lambda: get_score() == expected_score, timeout=6)


def p2p_connection(node, uacomment=None):
    return node.add_p2p_connection(QuorumDataInterface(), uacomment=uacomment)


def get_p2p_id(node, uacomment=None):
    def get_id():
        for p in node.getpeerinfo():
            for p2p in node.p2ps:
                if uacomment is not None and p2p.uacomment != uacomment:
                    continue
                if p["subver"] == p2p.strSubVer:
                    return p["id"]
        return None
    wait_until_helper(lambda: get_id() is not None, timeout=10)
    return get_id()


def mnauth(node, node_id, protx_hash, operator_pubkey):
    assert node.mnauth(node_id, protx_hash, operator_pubkey)
    mnauth_peer_id = None
    for peer in node.getpeerinfo():
        if "verified_proregtx_hash" in peer and peer["verified_proregtx_hash"] == protx_hash:
            assert_equal(mnauth_peer_id, None)
            mnauth_peer_id = peer["id"]
    assert_equal(mnauth_peer_id, node_id)


class QuorumDataInterface(P2PInterface):
    def __init__(self):
        super().__init__()

    def test_qgetdata(self, qgetdata, expected_error=0, len_vvec=0, len_contributions=0, response_expected=True):
        self.send_message(qgetdata)
        self.wait_for_qmessage("qdata", message_expected=response_expected)
        if response_expected:
            assert_qdata(self.get_qdata(), qgetdata, expected_error, len_vvec, len_contributions)

    def wait_for_qmessage(self, message=None, timeout=3, message_expected=True):
        wait_until_helper(lambda: self.message_count[message] > 0, timeout=timeout, lock=p2p_lock, do_assert=message_expected)
        if not message_expected:
            assert self.message_count[message] == 0
        self.message_count[message] = 0

    def get_qdata(self):
        return self.last_message["qdata"]


class QuorumDataMessagesTest(DashTestFramework):
    def set_test_params(self):
        extra_args = [["-llmq-data-recovery=0", "-deprecatedrpc=banscore"]] * 4
        self.set_dash_test_params(4, 3, extra_args=extra_args)

    def restart_mn(self, mn: MasternodeInfo, reindex=False):
        args = self.extra_args[mn.nodeIdx] + ['-masternodeblsprivkey=%s' % mn.keyOperator]
        if reindex:
            args.append('-reindex')
        self.restart_node(mn.nodeIdx, args)
        force_finish_mnsync(mn.get_node(self))
        self.connect_nodes(mn.nodeIdx, 0)
        self.sync_blocks()

    def run_test(self):

        def force_request_expire(bump_seconds=self.quorum_data_request_expiration_timeout + 1):
            self.bump_mocktime(bump_seconds)
            # Test with/without expired request cleanup
            if self.cleanup:
                self.generate(node0, 1, sync_fun=lambda: self.sync_blocks())

        def test_basics():
            self.log.info("Testing basics of QGETDATA/QDATA")
            force_request_expire()
            p2p_node0 = p2p_connection(node0)
            p2p_mn2 = p2p_connection(mn2.get_node(self))
            id_p2p_node0 = get_p2p_id(node0)
            id_p2p_mn2 = get_p2p_id(mn2.get_node(self))

            # Ensure that both nodes start with zero ban score
            wait_for_banscore(node0, id_p2p_node0, 0)
            wait_for_banscore(mn2.get_node(self), id_p2p_mn2, 0)

            self.log.info("Check that normal node doesn't respond to qgetdata "
                          "and does bump our score")
            p2p_node0.test_qgetdata(qgetdata_all, response_expected=False)
            wait_for_banscore(node0, id_p2p_node0, 10)
            self.log.info("Check that normal node bumps our score for qwatch")
            p2p_node0.send_message(msg_qwatch())
            wait_for_banscore(node0, id_p2p_node0, 20)
            # The masternode should not respond to qgetdata for non-masternode connections
            self.log.info("Check that masternode doesn't respond to "
                          "non-masternode connection and does bump our score")
            p2p_mn2.test_qgetdata(qgetdata_all, response_expected=False)
            wait_for_banscore(mn2.get_node(self), id_p2p_mn2, 10)
            # Open a fake MNAUTH authenticated P2P connection to the masternode to allow qgetdata
            node0.disconnect_p2ps()
            mn2.get_node(self).disconnect_p2ps()
            p2p_mn2 = p2p_connection(mn2.get_node(self))
            id_p2p_mn2 = get_p2p_id(mn2.get_node(self))
            mnauth(mn2.get_node(self), id_p2p_mn2, fake_mnauth_2[0], fake_mnauth_2[1])
            # The masternode should now respond to qgetdata requests
            self.log.info("Request verification vector")
            p2p_mn2.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold, 0)
            wait_for_banscore(mn2.get_node(self), id_p2p_mn2, 0)
            # Note: our banscore is bumped as we are requesting too rapidly,
            # however the node still returns the data
            self.log.info("Request encrypted contributions")
            p2p_mn2.test_qgetdata(qgetdata_contributions, 0, 0, self.llmq_size)
            wait_for_banscore(mn2.get_node(self), id_p2p_mn2, 25)
            # Request both
            # Note: our banscore is bumped as we are requesting too rapidly,
            # however the node still returns the data
            self.log.info("Request both")
            p2p_mn2.test_qgetdata(qgetdata_all, 0, self.llmq_threshold, self.llmq_size)
            wait_for_banscore(mn2.get_node(self), id_p2p_mn2, 50)
            mn2.get_node(self).disconnect_p2ps()

            self.log.info("Test ban score increase for invalid / unexpected QDATA")
            p2p_mn1 = p2p_connection(mn1.get_node(self))
            p2p_mn2 = p2p_connection(mn2.get_node(self))
            id_p2p_mn1 = get_p2p_id(mn1.get_node(self))
            id_p2p_mn2 = get_p2p_id(mn2.get_node(self))
            mnauth(mn1.get_node(self), id_p2p_mn1, fake_mnauth_1[0], fake_mnauth_1[1])
            mnauth(mn2.get_node(self), id_p2p_mn2, fake_mnauth_2[0], fake_mnauth_2[1])
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 0)
            p2p_mn2.test_qgetdata(qgetdata_all, 0, self.llmq_threshold, self.llmq_size)
            qdata_valid = p2p_mn2.get_qdata()
            # - Not requested
            p2p_mn1.send_message(qdata_valid)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 10)
            # - Already received
            force_request_expire()
            assert mn1.get_node(self).quorum("getdata", id_p2p_mn1, 100, quorum_hash, 0x03, mn1.proTxHash)
            p2p_mn1.wait_for_qmessage("qgetdata")
            p2p_mn1.send_message(qdata_valid)
            p2p_mn1.send_message(qdata_valid)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 20)
            # - Not like requested
            force_request_expire()
            assert mn1.get_node(self).quorum("getdata", id_p2p_mn1, 100, quorum_hash, 0x03, mn1.proTxHash)
            p2p_mn1.wait_for_qmessage("qgetdata")
            qdata_invalid_request = qdata_valid
            qdata_invalid_request.data_mask = 2
            p2p_mn1.send_message(qdata_invalid_request)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 30)
            # - Invalid verification vector
            force_request_expire()
            assert mn1.get_node(self).quorum("getdata", id_p2p_mn1, 100, quorum_hash, 0x03, mn1.proTxHash)
            p2p_mn1.wait_for_qmessage("qgetdata")
            qdata_invalid_vvec = qdata_valid
            qdata_invalid_vvec.quorum_vvec.pop()
            p2p_mn1.send_message(qdata_invalid_vvec)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 40)
            # - Invalid contributions
            force_request_expire()
            assert mn1.get_node(self).quorum("getdata", id_p2p_mn1, 100, quorum_hash, 0x03, mn1.proTxHash)
            p2p_mn1.wait_for_qmessage("qgetdata")
            qdata_invalid_contribution = qdata_valid
            qdata_invalid_contribution.enc_contributions.pop()
            p2p_mn1.send_message(qdata_invalid_contribution)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 50)
            mn1.get_node(self).disconnect_p2ps()
            mn2.get_node(self).disconnect_p2ps()

            self.log.info("Test all available error codes")
            p2p_mn2 = p2p_connection(mn2.get_node(self))
            id_p2p_mn2 = get_p2p_id(mn2.get_node(self))
            mnauth(mn2.get_node(self), id_p2p_mn2, fake_mnauth_2[0], fake_mnauth_2[1])
            qgetdata_invalid_type = msg_qgetdata(quorum_hash_int, 105, 0x01, protx_hash_int)
            qgetdata_invalid_block = msg_qgetdata(protx_hash_int, 100, 0x01, protx_hash_int)
            qgetdata_invalid_quorum = msg_qgetdata(int(mn2.get_node(self).getblockhash(0), 16), 100, 0x01, protx_hash_int)
            qgetdata_invalid_no_member = msg_qgetdata(quorum_hash_int, 100, 0x02, quorum_hash_int)
            p2p_mn2.test_qgetdata(qgetdata_invalid_type, QUORUM_TYPE_INVALID)
            p2p_mn2.test_qgetdata(qgetdata_invalid_block, QUORUM_BLOCK_NOT_FOUND)
            p2p_mn2.test_qgetdata(qgetdata_invalid_quorum, QUORUM_NOT_FOUND)
            p2p_mn2.test_qgetdata(qgetdata_invalid_no_member, MASTERNODE_IS_NO_MEMBER)
            # The last two error case require the node to miss its DKG data so we just reindex the node.
            mn2.get_node(self).disconnect_p2ps()
            self.restart_mn(mn1, reindex=True)
            # Re-connect to the masternode
            p2p_mn1 = p2p_connection(mn1.get_node(self))
            p2p_mn2 = p2p_connection(mn2.get_node(self))
            id_p2p_mn1 = get_p2p_id(mn1.get_node(self))
            id_p2p_mn2 = get_p2p_id(mn2.get_node(self))
            assert id_p2p_mn1 is not None
            assert id_p2p_mn2 is not None
            mnauth(mn1.get_node(self), id_p2p_mn1, fake_mnauth_1[0], fake_mnauth_1[1])
            mnauth(mn2.get_node(self), id_p2p_mn2, fake_mnauth_2[0], fake_mnauth_2[1])
            # Validate the DKG data is missing
            p2p_mn1.test_qgetdata(qgetdata_vvec, QUORUM_VERIFICATION_VECTOR_MISSING)
            p2p_mn1.test_qgetdata(qgetdata_contributions, ENCRYPTED_CONTRIBUTIONS_MISSING)
            self.test_mn_quorum_data(mn1, 100, quorum_hash, expect_secret=False)

            self.log.info("Test DKG data recovery with QDATA")
            # Now that mn1 is missing its DKG data try to recover it by querying the data from mn2 and then sending it
            # to mn1 with a direct QDATA message.
            #
            # mininode - QGETDATA -> mn2 - QDATA -> mininode - QDATA -> mn1
            #
            # However, mn1 only accepts self requested QDATA messages, that's why we trigger mn1 - QGETDATA -> mininode
            # via the RPC command "quorum getdata".
            #
            # Get the required DKG data for mn1
            p2p_mn2.test_qgetdata(qgetdata_all, 0, self.llmq_threshold, self.llmq_size)
            # Trigger mn1 - QGETDATA -> p2p_mn1
            assert mn1.get_node(self).quorum("getdata", id_p2p_mn1, 100, quorum_hash, 0x03, mn1.proTxHash)
            # Wait until mn1 sent the QGETDATA to p2p_mn1
            p2p_mn1.wait_for_qmessage("qgetdata")
            # Send the QDATA received from mn2 to mn1
            p2p_mn1.send_message(p2p_mn2.get_qdata())
            # Now mn1 should have its data back!
            self.wait_for_quorum_data([mn1], 100, quorum_hash, recover=False)
            # Restart one more time and make sure data gets saved to db
            mn1.get_node(self).disconnect_p2ps()
            mn2.get_node(self).disconnect_p2ps()
            self.restart_mn(mn1)
            self.wait_for_quorum_data([mn1], 100, quorum_hash, recover=False)

        # Test request limiting / banscore increase
        def test_request_limit():

            def test_send_from_two_to_one(send_1, expected_score_1, send_2, expected_score_2, clear_requests=False):
                if clear_requests:
                    force_request_expire()
                if send_1:
                    p2p_mn3_1.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold, 0)
                if send_2:
                    p2p_mn3_2.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold, 0)
                wait_for_banscore(mn3.get_node(self), id_p2p_mn3_1, expected_score_1)
                wait_for_banscore(mn3.get_node(self), id_p2p_mn3_2, expected_score_2)

            self.log.info("Test request limiting / banscore increases")

            p2p_mn1 = p2p_connection(mn1.get_node(self))
            id_p2p_mn1 = get_p2p_id(mn1.get_node(self))
            mnauth(mn1.get_node(self), id_p2p_mn1, fake_mnauth_1[0], fake_mnauth_1[1])
            p2p_mn1.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold, 0)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 0)
            force_request_expire(299)  # This shouldn't clear requests, next request should bump score
            p2p_mn1.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold, 0)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 25)
            force_request_expire(1)  # This should clear the requests now, next request should not bump score
            p2p_mn1.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold, 0)
            wait_for_banscore(mn1.get_node(self), id_p2p_mn1, 25)
            mn1.get_node(self).disconnect_p2ps()
            # Requesting one QDATA with mn1 and mn2 from mn3 should not result
            # in banscore increase for either of both.
            p2p_mn3_1 = p2p_connection(mn3.get_node(self), uacomment_m3_1)
            p2p_mn3_2 = p2p_connection(mn3.get_node(self), uacomment_m3_2)
            id_p2p_mn3_1 = get_p2p_id(mn3.get_node(self), uacomment_m3_1)
            id_p2p_mn3_2 = get_p2p_id(mn3.get_node(self), uacomment_m3_2)
            assert id_p2p_mn3_1 != id_p2p_mn3_2
            mnauth(mn3.get_node(self), id_p2p_mn3_1, fake_mnauth_1[0], fake_mnauth_1[1])
            mnauth(mn3.get_node(self), id_p2p_mn3_2, fake_mnauth_2[0], fake_mnauth_2[1])
            # Now try some {mn1, mn2} - QGETDATA -> mn3 combinations to make
            # sure request limit works connection based
            test_send_from_two_to_one(False, 0, True, 0, True)
            test_send_from_two_to_one(True, 0, True, 25)
            test_send_from_two_to_one(True, 25, False, 25)
            test_send_from_two_to_one(False, 25, True, 25, True)
            test_send_from_two_to_one(True, 25, True, 50)
            test_send_from_two_to_one(True, 50, True, 75)
            test_send_from_two_to_one(True, 50, True, 75, True)
            test_send_from_two_to_one(True, 75, False, 75)
            test_send_from_two_to_one(False, 75, True, None)
            # mn1 should still have a score of 75
            wait_for_banscore(mn3.get_node(self), id_p2p_mn3_1, 75)
            # mn2 should be "banned" now
            self.wait_until(lambda: not p2p_mn3_2.is_connected, timeout=10)
            mn3.get_node(self).disconnect_p2ps()

        # Test that QWATCH connections are also allowed to query data but all
        # QWATCH connections share one request limit slot
        def test_qwatch_connections():
            self.log.info("Test QWATCH connections")
            force_request_expire()
            p2p_mn3_1 = p2p_connection(mn3.get_node(self), uacomment_m3_1)
            p2p_mn3_2 = p2p_connection(mn3.get_node(self), uacomment_m3_2)
            id_p2p_mn3_1 = get_p2p_id(mn3.get_node(self), uacomment_m3_1)
            id_p2p_mn3_2 = get_p2p_id(mn3.get_node(self), uacomment_m3_2)
            assert id_p2p_mn3_1 != id_p2p_mn3_2

            wait_for_banscore(mn3.get_node(self), id_p2p_mn3_1, 0)
            wait_for_banscore(mn3.get_node(self), id_p2p_mn3_2, 0)

            # Send QWATCH for both connections
            p2p_mn3_1.send_message(msg_qwatch())
            p2p_mn3_2.send_message(msg_qwatch())

            # Now send alternating and make sure they share the same request limit
            p2p_mn3_1.test_qgetdata(qgetdata_all, 0, self.llmq_threshold, self.llmq_size)
            wait_for_banscore(mn3.get_node(self), id_p2p_mn3_1, 0)
            p2p_mn3_2.test_qgetdata(qgetdata_all, 0, self.llmq_threshold, self.llmq_size)
            wait_for_banscore(mn3.get_node(self), id_p2p_mn3_2, 25)
            p2p_mn3_1.test_qgetdata(qgetdata_all, 0, self.llmq_threshold, self.llmq_size)
            wait_for_banscore(mn3.get_node(self), id_p2p_mn3_1, 25)
            mn3.get_node(self).disconnect_p2ps()

        def test_watchquorums():
            self.log.info("Test -watchquorums support")
            for extra_args in [[], ["-watchquorums"]]:
                force_request_expire()
                self.restart_node(0, self.extra_args[0] + extra_args)
                for i in range(self.num_nodes - 1):
                    self.connect_nodes(0, i + 1)
                p2p_node0 = p2p_connection(node0)
                p2p_mn2 = p2p_connection(mn2.get_node(self))
                id_p2p_node0 = get_p2p_id(node0)
                id_p2p_mn2 = get_p2p_id(mn2.get_node(self))
                mnauth(node0, id_p2p_node0, fake_mnauth_1[0], fake_mnauth_1[1])
                mnauth(mn2.get_node(self), id_p2p_mn2, fake_mnauth_2[0], fake_mnauth_2[1])
                p2p_mn2.test_qgetdata(qgetdata_vvec, 0, self.llmq_threshold)
                assert node0.quorum("getdata", id_p2p_node0, 100, quorum_hash, 0x01)
                p2p_node0.wait_for_qmessage("qgetdata")
                p2p_node0.send_message(p2p_mn2.get_qdata())
                wait_for_banscore(node0, id_p2p_node0, (1 - len(extra_args)) * 10)
                # Non-masternodes should bump peer's score for qwatch no matter
                # whether they (non-masternodes) are watching or not.
                p2p_node0.send_message(msg_qwatch())
                wait_for_banscore(node0, id_p2p_node0, (1 - len(extra_args)) * 10 + 10)
                node0.disconnect_p2ps()
                mn2.get_node(self).disconnect_p2ps()

        def test_rpc_quorum_getdata_protx_hash():
            self.log.info("Test optional proTxHash of `quorum getdata`")
            assert_raises_rpc_error(-8, "proTxHash missing",
                                    mn1.get_node(self).quorum, "getdata", 0, 100, quorum_hash, 0x02)
            assert_raises_rpc_error(-8, "proTxHash invalid",
                                    mn1.get_node(self).quorum, "getdata", 0, 100, quorum_hash, 0x03,
                                    "0000000000000000000000000000000000000000000000000000000000000000")

        # Enable DKG and disable ChainLocks
        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", 4070908800)

        self.wait_for_sporks_same()
        quorum_hash = self.mine_quorum()

        node0 = self.nodes[0]
        mn1: MasternodeInfo = self.mninfo[0]
        mn2: MasternodeInfo = self.mninfo[1]
        mn3: MasternodeInfo = self.mninfo[2]

        # Convert the hex values into integer values
        quorum_hash_int = int(quorum_hash, 16)
        protx_hash_int = int(mn1.proTxHash, 16)

        # Valid requests
        qgetdata_vvec = msg_qgetdata(quorum_hash_int, 100, 0x01)
        qgetdata_contributions = msg_qgetdata(quorum_hash_int, 100, 0x02, protx_hash_int)
        qgetdata_all = msg_qgetdata(quorum_hash_int, 100, 0x03, protx_hash_int)

        # Test with/without expired request cleanup
        for self.cleanup in [True, False]:
            test_basics()
            test_request_limit()
            test_qwatch_connections()
            test_watchquorums()
            test_rpc_quorum_getdata_protx_hash()


if __name__ == '__main__':
    QuorumDataMessagesTest().main()
