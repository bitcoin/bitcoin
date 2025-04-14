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


def estimate_capacity(theirs, ours):
    set_size_diff = abs(theirs - ours)
    min_size = min(ours, theirs)
    weighted_min_size = int(RECON_Q * min_size)
    estimated_diff = 1 + weighted_min_size + set_size_diff

    # Poor man's minisketch_compute_capacity.
    return estimated_diff if estimated_diff <= 9 else estimated_diff - 1

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
        self.last_tx.append(message.tx.calc_sha256(True))

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
    def add_options(self, parser):
        self.add_wallet_options(parser)

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

        received_wtxids = set(peer.last_inv.pop())
        assert_equal(expected_wtxids, received_wtxids)

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
