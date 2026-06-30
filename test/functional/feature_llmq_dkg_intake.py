#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
feature_llmq_dkg_intake.py

Adversarial P2P tests for DKG message-intake hardening:
  - pushed DKG messages (qcontrib/qcomplaint/qjustify/qpcommit) from a peer that is
    not MNAuth-verified are rejected before retention.
  - oversized DKG payloads are rejected (before deserialization / retention) even
    from a verified peer.
  - structural pre-validation: malformed DKG payloads (valid quorum prefix, garbage
    body) are rejected before retention even from a verified peer.

The node must not crash; the sending peer must be scored (Misbehaving).
"""

from test_framework.messages import ser_uint256
from test_framework.p2p import P2PInterface
from test_framework.test_framework import DashTestFramework
from test_framework.util import wait_until_helper

LLMQ_TEST = 100

# A masternode protx/operator-pubkey pair accepted by the regtest-only `mnauth`
# debug RPC, used to mark a P2P connection as MNAuth-verified without BLS signing.
FAKE_PROTX = "cecf37bf0ec05d2d22cb8227f88074bb882b94cd2081ba318a5a444b1b15b9fd"
FAKE_PUBKEY = "8e7afdb849e5e2a085b035b62e21c0940c753f2d4501325743894c37162f287bccaffbedd60c36581dabbf127a22e43f"

DKG_PUSH_TYPES = [b"qcontrib", b"qcomplaint", b"qjustify", b"qpcommit"]


class msg_dkg_raw:
    """A DKG push message carrying an arbitrary raw payload (for adversarial intake tests)."""
    __slots__ = ("msgtype", "payload")

    def __init__(self, msgtype, payload=b""):
        self.msgtype = msgtype
        self.payload = payload

    def serialize(self):
        return self.payload

    def __repr__(self):
        return "msg_dkg_raw(type=%s, len=%d)" % (self.msgtype, len(self.payload))


def get_p2p_id(node):
    def get_id():
        for p in node.getpeerinfo():
            for p2p in node.p2ps:
                if p["subver"] == p2p.strSubVer:
                    return p["id"]
        return None
    wait_until_helper(lambda: get_id() is not None, timeout=10)
    return get_id()


def wait_for_banscore(node, peer_id, expected_score):
    def get_score():
        for peer in node.getpeerinfo():
            if peer["id"] == peer_id:
                return peer["banscore"]
        return None
    wait_until_helper(lambda: get_score() == expected_score, timeout=10)


class DkgIntakeTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        # -whitelist keeps the adversarial peer connected even after it crosses the
        #   discouragement threshold, so banscore stays observable for the score==100 cases.
        # -debug=net surfaces the Misbehaving reason strings in debug.log.
        extra_args = [["-whitelist=127.0.0.1", "-debug=net", "-deprecatedrpc=banscore"]] * 4
        self.set_dash_test_params(4, 3, extra_args=extra_args)

    def quorum_hash_prefix(self):
        # llmqType (1 byte) + quorumHash (32 bytes, little-endian) -- the on-wire prefix
        # shared by every DKG message, used so oversized/malformed payloads resolve to a
        # real in-progress quorum and reach the size/structural checks.
        return bytes([LLMQ_TEST]) + ser_uint256(int(self.quorum_hash, 16))

    def add_verified_peer(self, node):
        peer = node.add_p2p_connection(P2PInterface())
        peer_id = get_p2p_id(node)
        assert node.mnauth(peer_id, FAKE_PROTX, FAKE_PUBKEY)
        return peer, peer_id

    def run_test(self):
        node0 = self.nodes[0]
        node0.sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", 0)
        self.wait_for_sporks_same()

        # Mine a quorum so we have a quorumHash that resolves to a valid DKG base block.
        self.quorum_hash = self.mine_quorum()

        # Target an active masternode -- the realistic victim of these messages.
        mn_node = self.mninfo[0].get_node(self)

        self.test_unverified_sender_rejected(mn_node)
        self.test_oversized_rejected(mn_node)
        self.test_malformed_rejected(mn_node)

    def test_unverified_sender_rejected(self, node):
        self.log.info("Pushed DKG messages from a non-verified peer are rejected (Misbehaving 10 each)")
        peer = node.add_p2p_connection(P2PInterface())
        peer_id = get_p2p_id(node)
        wait_for_banscore(node, peer_id, 0)
        score = 0
        for msgtype in DKG_PUSH_TYPES:
            with node.assert_debug_log(["DKG message from non-verified peer"]):
                peer.send_message(msg_dkg_raw(msgtype, self.quorum_hash_prefix()))
                peer.sync_with_ping()
            score += 10
            wait_for_banscore(node, peer_id, score)
        node.disconnect_p2ps()

    def test_oversized_rejected(self, node):
        self.log.info("Oversized DKG payloads are rejected even from a verified peer (Misbehaving 100)")
        peer, peer_id = self.add_verified_peer(node)
        wait_for_banscore(node, peer_id, 0)
        # >1 MiB clears the hard ceiling regardless of quorum params, and stays under the
        # 3 MiB transport cap so the message is delivered to the handler.
        payload = self.quorum_hash_prefix() + b"\x00" * (1024 * 1024 + 4096)
        with node.assert_debug_log(["oversized DKG message"]):
            peer.send_message(msg_dkg_raw(b"qcontrib", payload))
            peer.sync_with_ping()
        wait_for_banscore(node, peer_id, 100)
        node.disconnect_p2ps()

    def test_malformed_rejected(self, node):
        self.log.info("Malformed DKG payloads are rejected even from a verified peer (Misbehaving 100)")
        peer, peer_id = self.add_verified_peer(node)
        wait_for_banscore(node, peer_id, 0)
        # Valid llmqType + quorumHash prefix, then too few bytes to deserialize a
        # CDKGContribution -> structural pre-validation rejects it before retention.
        payload = self.quorum_hash_prefix() + b"\x00\x00\x00\x00"
        with node.assert_debug_log(["malformed DKG message"]):
            peer.send_message(msg_dkg_raw(b"qcontrib", payload))
            peer.sync_with_ping()
        wait_for_banscore(node, peer_id, 100)
        node.disconnect_p2ps()


if __name__ == '__main__':
    DkgIntakeTest().main()
