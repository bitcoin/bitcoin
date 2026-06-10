#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test BIP434 feature negotiation."""

from test_framework.messages import (
    msg_version,
    ser_compact_size,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    P2P_SUBVERSION,
    P2P_VERSION_RELAY,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

# Pre-BIP434 protocol version
PRE_FEATURE_VERSION = 70016
# Protocol version which enables BIP434 FEATURE negotiation
FEATURE_VERSION = 70017
# BIP434 wire-format limits
MIN_FEATUREID_LENGTH = 4
MAX_FEATUREID_LENGTH = 80
MAX_FEATUREDATA_LENGTH = 512


class RawFeature:
    """A FEATURE message with a hand-crafted payload."""
    msgtype = b"feature"

    def __init__(self, payload):
        self.payload = payload

    def serialize(self):
        return self.payload

    def __repr__(self):
        return f"RawFeature(payload_len={len(self.payload)})"


def feature_wire(feature_id, feature_data, *, trailing=b""):
    """Build a FEATURE payload plus optional trailing bytes."""
    if isinstance(feature_id, str):
        feature_id = feature_id.encode()
    return (ser_compact_size(len(feature_id)) + feature_id
            + ser_compact_size(len(feature_data)) + feature_data
            + trailing)


def _version_msg(nversion):
    v = msg_version()
    v.nVersion = nversion
    v.strSubVer = P2P_SUBVERSION
    v.nServices = P2P_SERVICES
    v.relay = P2P_VERSION_RELAY
    return v


class FeaturePeer(P2PInterface):
    """P2PInterface that counts FEATURE messages received."""

    def __init__(self):
        super().__init__()
        self.got_feature_count = 0
        self.last_feature = None

    def on_feature(self, message):
        self.got_feature_count += 1
        self.last_feature = message


class FeaturePeerNoVerack(FeaturePeer):
    """Peer that records but does not auto-reply to the node's VERSION, so the
    node stays in the post-VERSION / pre-VERACK window where FEATURE messages
    are valid to send."""

    def on_version(self, message):
        self.nServices = message.nServices
        self.relay = message.relay


class P2PBIP434FeatureTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # peertimeout=999 prevents the node from kicking the peer for being idle
        self.extra_args = [["-debug=net", "-peertimeout=999"]]

    def run_test(self):
        self.test_advertised_version()
        self.test_no_feature_to_pre_70017_peer()
        self.test_features_announced_to_modern_peer()
        self.test_feature_after_verack_disconnects()
        self.test_feature_before_version_ignored()
        self.test_feature_id_length_boundaries()
        self.test_feature_data_length_boundaries()
        self.test_trailing_bytes_disconnect()
        self.test_truncated_feature_id_disconnect()
        self.test_truncated_feature_data_disconnect()
        self.test_non_ascii_feature_id_accepted()
        self.test_many_features_in_handshake()
        self.test_recv_feature_from_pre_70017_peer()

    def _silent_peer(self, *, nversion=FEATURE_VERSION):
        peer = self.nodes[0].add_p2p_connection(
            FeaturePeerNoVerack(),
            send_version=False, wait_for_verack=False,
        )
        peer.send_without_ping(_version_msg(nversion))
        peer.wait_for_verack()
        return peer

    def _expect_accept(self, payload, *, log_substring="unknown feature advertised",
                      nversion=FEATURE_VERSION):
        peer = self._silent_peer(nversion=nversion)
        with self.nodes[0].assert_debug_log([log_substring], timeout=2):
            peer.send_without_ping(RawFeature(payload))
        assert peer.is_connected, "peer disconnected after a well-formed FEATURE"
        self.nodes[0].disconnect_p2ps()

    def _expect_disconnect(self, payload, *, log_substring="invalid feature payload",
                          nversion=FEATURE_VERSION):
        peer = self._silent_peer(nversion=nversion)
        with self.nodes[0].assert_debug_log([log_substring], timeout=2):
            peer.send_without_ping(RawFeature(payload))
            peer.wait_for_disconnect()

    def test_advertised_version(self):
        self.log.info("Test that node advertises FEATURE to peer with protocol version 70017")
        peer = self.nodes[0].add_p2p_connection(FeaturePeer())
        assert_equal(peer.last_message["version"].nVersion, FEATURE_VERSION)
        self.nodes[0].disconnect_p2ps()

    def test_no_feature_to_pre_70017_peer(self):
        self.log.info("Test that node doesn't send FEATURE to a peer with protocol version <70017")
        peer = self.nodes[0].add_p2p_connection(
            FeaturePeer(), send_version=False, wait_for_verack=False,
        )
        peer.send_without_ping(_version_msg(PRE_FEATURE_VERSION))
        peer.wait_for_verack()
        # The node's full handshake has now been delivered to us; if any
        # FEATURE would have been sent it would be in last_message by now.
        assert_equal(peer.got_feature_count, 0)
        self.nodes[0].disconnect_p2ps()

    def test_features_announced_to_modern_peer(self):
        self.log.info("Test that node announces correct number of features to a 70017 peer")
        peer = self.nodes[0].add_p2p_connection(FeaturePeer())
        assert_equal(peer.got_feature_count, 0)
        self.nodes[0].disconnect_p2ps()

    def test_feature_after_verack_disconnects(self):
        self.log.info("Test that FEATURE after VERACK triggers disconnect")
        peer = self.nodes[0].add_p2p_connection(FeaturePeer())
        peer.sync_with_ping()  # ensure node has set fSuccessfullyConnected
        with self.nodes[0].assert_debug_log(["feature received after verack"], timeout=2):
            peer.send_without_ping(RawFeature(feature_wire(b"abcd", b"")))
            peer.wait_for_disconnect()

    def test_feature_before_version_ignored(self):
        self.log.info("Test that FEATURE before any VERSION is silently ignored")
        peer = self.nodes[0].add_p2p_connection(
            FeaturePeer(), send_version=False, wait_for_verack=False,
        )
        with self.nodes[0].assert_debug_log(
            ["non-version message before version handshake"], timeout=2,
        ):
            peer.send_without_ping(RawFeature(feature_wire(b"abcd", b"")))
        assert peer.is_connected
        self.nodes[0].disconnect_p2ps()

    def test_feature_id_length_boundaries(self):
        self.log.info("Test feature_id length boundaries")
        for length, accept in [(0, False),
                               (3, False),
                               (MIN_FEATUREID_LENGTH, True),
                               (MAX_FEATUREID_LENGTH, True),
                               (MAX_FEATUREID_LENGTH + 1, False)]:
            payload = feature_wire(b"a" * length, b"")
            if accept:
                self._expect_accept(payload)
            else:
                self._expect_disconnect(payload)

    def test_feature_data_length_boundaries(self):
        self.log.info("Test feature_data length boundaries")
        for length, accept in [(0, True),
                               (MAX_FEATUREDATA_LENGTH, True),
                               (MAX_FEATUREDATA_LENGTH + 1, False)]:
            payload = feature_wire(b"abcd", b"\x00" * length)
            if accept:
                self._expect_accept(payload)
            else:
                self._expect_disconnect(payload)

    def test_trailing_bytes_disconnect(self):
        self.log.info("Test that trailing bytes after data triggers disconnect")
        self._expect_disconnect(
            feature_wire(b"abcd", b"", trailing=b"\x00"),
        )

    def test_truncated_feature_id_disconnect(self):
        self.log.info("Test that truncated feature_id triggers disconnect")
        payload = ser_compact_size(10) + b"abcde"
        self._expect_disconnect(payload)

    def test_truncated_feature_data_disconnect(self):
        self.log.info("Test that truncated feature_data triggers disconnect")
        payload = (ser_compact_size(MIN_FEATUREID_LENGTH) + b"abcd"
                   + ser_compact_size(10) + b"xx")
        self._expect_disconnect(payload)

    def test_non_ascii_feature_id_accepted(self):
        self.log.info("Test that feature_id with non-ASCII bytes is still accepted")
        # BIP says SHOULD, not MUST, on this
        self._expect_accept(feature_wire(b"\x00\xff\x01\x7f", b""))

    def test_many_features_in_handshake(self):
        self.log.info("Test multiple FEATURE advertisements")
        peer = self._silent_peer()
        with self.nodes[0].assert_debug_log(["unknown feature advertised"], timeout=2):
            for i in range(16):
                peer.send_without_ping(
                    RawFeature(feature_wire(f"feat{i:04d}".encode(), b""))
                )
        assert peer.is_connected
        self.nodes[0].disconnect_p2ps()

    def test_recv_feature_from_pre_70017_peer(self):
        self.log.info("Test that FEATURE from <70017 peer triggers disconnect")
        self._expect_disconnect(
            feature_wire(b"abcd", b""),
            log_substring="feature received with incompatible version",
            nversion=PRE_FEATURE_VERSION,
        )


if __name__ == "__main__":
    P2PBIP434FeatureTest(__file__).main()
