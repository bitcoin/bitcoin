#!/usr/bin/env python3
# Copyright (c) 2026 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that an oversized govsync bloom filter is rejected.

A MNGOVERNANCESYNC ("govsync") request carries a peer-supplied CBloomFilter. For a
per-object request the node tests that filter against every cached vote, and
CBloomFilter::contains() loops nHashFuncs times. nHashFuncs is deserialized without
bounds, so an unbounded value would force an enormous amount of work while the message
processing mutex is held. The handler must reject any filter that is not within the
standard size constraints (vData <= 36000 bytes, nHashFuncs <= 50), matching filterload.
"""
import struct

from test_framework.messages import ser_string, ser_uint256
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import force_finish_mnsync

# CBloomFilter size constraints (src/common/bloom.h).
MAX_HASH_FUNCS = 50


class msg_govsync:
    """MNGOVERNANCESYNC: a governance-object hash followed by a CBloomFilter."""
    msgtype = b"govsync"

    def __init__(self, nprop=0, data=b"", n_hash_funcs=0, n_tweak=0, n_flags=0):
        self.nprop = nprop
        self.data = data
        self.n_hash_funcs = n_hash_funcs
        self.n_tweak = n_tweak
        self.n_flags = n_flags

    def serialize(self):
        r = ser_uint256(self.nprop)
        r += ser_string(self.data)  # CBloomFilter.vData
        r += struct.pack("<I", self.n_hash_funcs)
        r += struct.pack("<I", self.n_tweak)
        r += struct.pack("<B", self.n_flags)
        return r

    def __repr__(self):
        return f"msg_govsync(n_hash_funcs={self.n_hash_funcs})"


class GovsyncBloomCapTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        force_finish_mnsync(node)

        self.log.info("A govsync request with a well-formed bloom filter is accepted (no false positive)")
        good_peer = node.add_p2p_connection(P2PInterface())
        good_peer.send_message(msg_govsync(n_hash_funcs=MAX_HASH_FUNCS))
        good_peer.sync_with_ping()
        assert good_peer.is_connected
        node.disconnect_p2ps()

        self.log.info("A govsync request with an out-of-bounds bloom filter (nHashFuncs > 50) is rejected and the peer disconnected")
        bad_peer = node.add_p2p_connection(P2PInterface())
        bad_peer.send_message(msg_govsync(n_hash_funcs=0xFFFFFFFF))
        bad_peer.wait_for_disconnect()


if __name__ == '__main__':
    GovsyncBloomCapTest().main()
