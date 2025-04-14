#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers to test reconciliation-based transaction relay, both initiator and responder roles"""

import random
from io import BytesIO

from test_framework.messages import (
    msg_wtxidrelay, msg_verack, msg_sendtxrcncl,
    msg_reqtxrcncl, msg_sketch, msg_reconcildiff,
    msg_reqsketchext,msg_inv, msg_getdata,
    MSG_WTX, MSG_BLOCK, CTransaction, CInv,
)
from test_framework.key import TaggedHash
from test_framework.p2p import P2PDataStore
from test_framework.util import assert_equal
from test_framework.crypto.siphash import siphash256
from test_framework.test_framework import BitcoinTestFramework


# These parameters are specified in the BIP-0330.
Q_PRECISION = (2 << 14) - 1
FIELD_BITS = 32
FIELD_MODULUS = (1 << FIELD_BITS) + 0b10001101
BYTES_PER_SKETCH_CAPACITY = int(FIELD_BITS / 8)
# These parameters are suggested by the Erlay paper based on analysis and
# simulations.
RECON_Q = 0.25

MAX_SKETCH_CAPACITY = 2 << 12
RECON_FALSE_POSITIVE_COEF = 16

def log2_factorial(x):
    """
    Compute floor(log2(x!)), exactly up to x=57; an underestimate up to x=2^32-1
    Translated from minisketch/src/false_positives.h
    """
    T = [
        0, 4, 9, 13, 18, 22, 26, 30, 34, 37, 41, 45, 48, 52, 55, 58,
        62, 65, 68, 71, 74, 77, 80, 82, 85, 88, 90, 93, 96, 98, 101, 103
    ]
    bits = x.bit_length()
    l2_106 = 106 * (bits - 1) + T[((x << (32 - bits)) >> 26) & 31]

    return (418079 * (2 * x + 1) * l2_106 - 127870026 * x + 117504694 + 88632748 * (x < 3)) // 88632748


def base_fp_bits(bits, capacity):
    """
    Compute floor(log2(2^(bits * capacity) / sum((2^bits - 1) choose k, k=0..capacity))), for bits>1
    Translated from minisketch/src/false_positives.h
    """

    ADD5 = [1, 1, 1, 1, 2, 2, 2, 3, 4, 4, 5, 5, 6, 7, 8, 8, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 12]
    ADD6 = [1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 20, 21, 21, 22, 22, 23, 23, 23, 24, 24, 24, 24]
    ADD7 = [1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7, 8, 7, 8, 9, 9, 9, 10, 11, 11, 12, 12, 13, 13, 15, 15, 15, 16, 17, 17, 18, 19, 20, 20]
    ADD8 = [1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 3, 4, 4, 5, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 8, 9, 9]
    ADD9 = [1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 2, 1, 1, 1, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 3, 2, 3, 3, 3, 3, 4, 3, 3, 4, 4, 4, 4]

    if capacity == 0:
        return 0

    ret = 0
    if bits < 32 and capacity >= (1 << bits):
        ret = bits * (capacity - (1 << bits) + 1)
        capacity = (1 << bits) - 1

    ret += log2_factorial(capacity)
    if bits == 2:
        return ret + (0 if capacity <= 2 else 1)
    elif bits == 3:
        return ret + (0 if capacity <= 2 else (0x2a5 >> (2 * (capacity - 3))) & 3)
    elif bits == 4:
        return ret + (0 if capacity <= 3 else (0xb6d91a449 >> (3 * (capacity - 4))) & 7)
    elif bits == 5:
        return ret + (0 if capacity <= 4 else ADD5[capacity - 5])
    elif bits == 6:
        return ret + (0 if capacity <= 4 else 25 if capacity > 54 else ADD6[capacity - 5])
    elif bits == 7:
        return ret + (0 if capacity <= 4 else 21 if capacity > 57 else ADD7[capacity - 5])
    elif bits == 8:
        return ret + (0 if capacity <= 9 else 10 if capacity > 56 else ADD8[capacity - 10])
    elif bits == 9:
        return ret + (0 if capacity <= 11 else 5 if capacity > 54 else ADD9[capacity - 12])
    elif bits == 10:
        return ret + (0 if capacity <= 21 else 2 if capacity > 50 else (0x1a6665545555041 >> 2 * (capacity - 22)) & 3)
    elif bits == 11:
        return ret + (0 if capacity <= 21 else 1 if capacity > 45 else (0x5b3dc1 >> (capacity - 22)) & 1)
    elif bits == 12:
        return ret + (0 if capacity <= 21 else 0 if capacity > 57 else (0xe65522041 >> (capacity - 22)) & 1)
    elif bits == 13:
        return ret + (0 if capacity <= 27 else 0 if capacity > 55 else (0x8904081 >> (capacity - 28)) & 1)
    elif bits == 14:
        return ret + (0 if capacity <= 47 else 0 if capacity > 48 else 1)
    else:
        return ret


def compute_capacity(bits, max_elements, fpbits):
    if bits == 0:
        return 0
    if max_elements > 0xffffffff:
        return max_elements
    base_fpbits = base_fp_bits(bits, max_elements)
    if base_fpbits >= fpbits:
        return max_elements
    return max_elements + (fpbits - base_fpbits + bits - 1) // bits


def mul2(x):
    """Compute 2*x in GF(2^FIELD_BITS)"""
    return (x << 1) ^ (FIELD_MODULUS if x.bit_length() >= FIELD_BITS else 0)


def mul(x, y):
    """Compute x*y in GF(2^FIELD_BITS)"""
    ret = 0
    for bit in [(x >> i) & 1 for i in range(x.bit_length())]:
        ret, y = ret ^ bit * y, mul2(y)
    return ret


def create_sketch(shortids, capacity):
    """Compute the bytes of a sketch for given shortids and given capacity."""
    odd_sums = [0 for _ in range(capacity)]
    for shortid in shortids:
        squared = mul(shortid, shortid)
        for i in range(capacity):
            odd_sums[i] ^= shortid
            shortid = mul(shortid, squared)
    sketch_bytes = []
    for odd_sum in odd_sums:
        for i in range(4):
            sketch_bytes.append((odd_sum >> (i * 8)) & 0xff)
    return sketch_bytes


def get_short_id(wtxid, salt):
    (k0, k1) = salt
    s = siphash256(k0, k1, wtxid)
    return 1 + (s & 0xFFFFFFFF)


def estimate_sketch_capacity(theirs, ours):
    set_size_diff = abs(theirs - ours)
    min_size = min(ours, theirs)
    weighted_min_size = int(RECON_Q * min_size)
    estimated_diff = 1 + weighted_min_size + set_size_diff

    return compute_capacity(FIELD_BITS, estimated_diff, RECON_FALSE_POSITIVE_COEF)

def generate_transaction(node, from_txid):
    to_address = node.getnewaddress()
    inputs = [{"txid": from_txid, "vout": 0}]
    outputs = {to_address: 0.0001}
    rawtx = node.createrawtransaction(inputs, outputs)
    signresult = node.signrawtransactionwithwallet(rawtx)
    tx = CTransaction()
    tx.deserialize(BytesIO(bytes.fromhex(signresult['hex'])))
    tx.rehash()
    return tx


class TxReconTestP2PConn(P2PDataStore):
    def __init__(self):
        super().__init__()
        self.recon_version = 1
        self.mininode_salt = random.randrange(0xffffff)
        self.node_salt = 0
        self.combined_salt = None
        self.last_sendtxrcncl = []
        self.last_reqtxrcncl = []
        self.last_sketch = []
        self.last_reconcildiff = []
        self.last_reqsketchext = []
        self.last_inv = []
        self.last_tx = []

    def on_version(self, message):
        if self.recon_version == 1:
            if not self.p2p_connected_to_node:
                self.send_version()
            assert message.nVersion >= 70016, "We expect the node to support WTXID relay"
            self.send_without_ping(msg_wtxidrelay())
            self.send_sendtxrcncl()
            self.send_without_ping(msg_verack())
            self.nServices = message.nServices
        else:
            super().on_version(message)

    def on_sendtxrcncl(self, message):
        self.node_salt = message.salt
        self.combined_salt = self.compute_salt()

    def on_reqtxrcncl(self, message):
        self.last_reqtxrcncl.append(message)

    def on_sketch(self, message):
        self.last_sketch.append(message)

    def on_reconcildiff(self, message):
        self.last_reconcildiff.append(message)

    def on_reqsketchext(self, message):
        self.last_reqsketchext.append(message)

    def on_inv(self, message):
        self.last_inv.append([inv.hash for inv in message.inv if inv.type != MSG_BLOCK])  # ignore block invs

    def on_tx(self, message):
        self.last_tx.append(message.tx.wtxid_int)

    def send_sendtxrcncl(self):
        msg = msg_sendtxrcncl()
        msg.salt = self.mininode_salt
        msg.version = self.recon_version
        self.send_without_ping(msg)

    def send_reqtxrcncl(self, set_size, q):
        msg = msg_reqtxrcncl()
        msg.set_size = set_size
        msg.q = q
        self.send_without_ping(msg)

    def send_sketch(self, skdata):
        msg = msg_sketch()
        msg.skdata = skdata
        self.send_without_ping(msg)

    def send_reconcildiff(self, success, ask_shortids, sync_with_ping=False):
        msg = msg_reconcildiff()
        msg.success = success
        msg.ask_shortids = ask_shortids
        if sync_with_ping:
            self.send_and_ping(msg)
        else :
            self.send_without_ping(msg)

    def send_reqsketchext(self):
        self.send_without_ping(msg_reqsketchext())

    def send_inv(self, inv_wtxids):
        msg = msg_inv(inv=[CInv(MSG_WTX, h=wtxid) for wtxid in inv_wtxids])
        self.send_without_ping(msg)

    def send_getdata(self, ask_wtxids):
        msg = msg_getdata(inv=[CInv(MSG_WTX, h=wtxid) for wtxid in ask_wtxids])
        self.send_without_ping(msg)

    def compute_salt(self):
        RECON_STATIC_SALT = "Tx Relay Salting"
        salt1, salt2 = self.node_salt, self.mininode_salt
        salt = min(salt1, salt2).to_bytes(8, "little") + max(salt1, salt2).to_bytes(8, "little")
        h = TaggedHash(RECON_STATIC_SALT, salt)
        k0 = int.from_bytes(h[0:8], "little")
        k1 = int.from_bytes(h[8:16], "little")
        return k0, k1


class ReconciliationTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txreconciliation']]

    def generate_txs(self, wallet, n_mininode_unique, n_node_unique, n_shared):
        mininode_unique = [wallet.create_self_transfer()["tx"] for _ in range(n_mininode_unique)]
        node_unique = [wallet.create_self_transfer()["tx"] for _ in range(n_node_unique)]
        shared = [wallet.create_self_transfer()["tx"] for _ in range(n_shared)]

        tx_submitter = self.nodes[0].add_p2p_connection(P2PDataStore())
        tx_submitter.send_txs_and_test(
            node_unique + shared, self.nodes[0], success=True)
        tx_submitter.peer_disconnect()

        return mininode_unique, node_unique, shared

    # Wait for the next INV message to be received by the given peer.
    # Clear and check it matches the expected transactions.
    def wait_for_inv(self, peer, expected_wtxids):
        def received_inv():
            return (len(peer.last_inv) > 0)
        self.wait_until(received_inv)

        assert_equal(set(expected_wtxids), set(peer.last_inv.pop()))

    def request_transactions_from(self, peer, wtxids_to_request):
        # Make sure there were no unexpected transactions received before
        assert_equal(peer.last_tx, [])
        peer.send_getdata(wtxids_to_request)

    # Wait for the next TX message to be received by the given peer.
    # Clear and check it matches the expected transactions.
    def wait_for_txs(self, peer, expected_wtxids):
        def received_txs():
            return (len(peer.last_tx) == len(expected_wtxids))
        self.wait_until(received_txs)

        assert_equal(set(expected_wtxids), set(peer.last_tx))
        peer.last_tx.clear()

    def run_test(self):
        pass
