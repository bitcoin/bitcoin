#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers to test reconciliation-based transaction relay, both initiator and responder roles"""

import random
import time
from io import BytesIO
import struct


from test_framework.messages import (
    msg_inv, msg_wtxidrelay,
    msg_verack, msg_sendtxrcncl,
    MSG_WTX, MSG_BLOCK, CTransaction, CInv,
)
from test_framework.key import TaggedHash
from test_framework.p2p import P2PDataStore, P2PInterface
from test_framework.crypto.siphash import siphash256
from test_framework.test_framework import BitcoinTestFramework


# These parameters are specified in the BIP-0330.
Q_PRECISION = (2 << 14) - 1
FIELD_BITS = 32
FIELD_MODULUS = (1 << FIELD_BITS) + 0b10001101
BYTES_PER_SKETCH_CAPACITY = FIELD_BITS / 8
# These parameters are suggested by the Erlay paper based on analysis and
# simulations.
RECON_Q = 0.1


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


def get_short_id(tx, salt):
    (k0, k1) = salt
    wtxid = tx.calc_sha256(with_witness=True)
    s = siphash256(k0, k1, wtxid)
    return 1 + (s & 0xFFFFFFFF)


def estimate_capacity(theirs, ours):
    capacity = int(abs(theirs - ours) + RECON_Q * min(theirs, ours)) + 1
    if capacity < 9:
        # Poor man's minisketch_compute_capacity.
        capacity += 1
    return capacity


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


class TxReconTestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()
        self.recon_version = 1
        self.mininode_salt = random.randrange(0xffffff)
        self.node_salt = 0
        self.last_inv = []
        self.last_tx = []

    def on_version(self, message):
        if self.recon_version == 1:
            if not self.p2p_connected_to_node:
                self.send_version()
            assert message.nVersion >= 70016, "We expect the node to support WTXID relay"
            self.send_message(msg_wtxidrelay())
            self.send_sendtxrcncl()
            self.send_message(msg_verack())
            self.nServices = message.nServices
        else:
            super().on_version(message)

    def on_sendtxrcncl(self, message):
        self.node_salt = message.salt

    def on_reqtxrcncl(self, message):
        # This is needed for dummy fanout destinations.
        pass

    def on_inv(self, message):
        for inv in message.inv:
            if inv.type != MSG_BLOCK:  # ignore block invs
                self.last_inv.append(inv.hash)

    def on_tx(self, message):
        self.last_tx.append(message.tx.calc_sha256(True))

    def send_sendtxrcncl(self):
        msg = msg_sendtxrcncl()
        msg.salt = self.mininode_salt
        msg.version = self.recon_version
        self.send_message(msg)

    def send_inv(self, inv_wtxids):
        msg = msg_inv(inv=[CInv(MSG_WTX, h=wtxid) for wtxid in inv_wtxids])
        self.send_message(msg)


class ReconciliationTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txreconciliation']]

    def proceed_in_time(self, jump_in_seconds):
        # We usually need the node to process some messages first.
        time.sleep(0.01)
        self.mocktime += jump_in_seconds
        self.nodes[0].setmocktime(self.mocktime)

    def compute_salt(self):
        RECON_STATIC_SALT = "Tx Relay Salting"
        salt1, salt2 = self.test_node.node_salt, self.test_node.mininode_salt
        if salt1 > salt2:
            salt1, salt2 = salt2, salt1
        salt = struct.pack("<Q", salt1) + struct.pack("<Q", salt2)
        h = TaggedHash(RECON_STATIC_SALT, salt)
        k0 = int.from_bytes(h[0:8], "little")
        k1 = int.from_bytes(h[8:16], "little")
        return k0, k1

    def generate_txs(self, n_mininode_unique, n_node_unique, n_shared, utxos):
        mininode_unique = [generate_transaction(self.nodes[0], utxos.pop()['txid']) for _ in range(0, n_mininode_unique)]
        node_unique = [generate_transaction(self.nodes[0], utxos.pop()['txid']) for _ in range(0, n_node_unique)]
        shared = [generate_transaction(self.nodes[0], utxos.pop()['txid']) for _ in range(0, n_shared)]

        tx_submitter = self.nodes[0].add_p2p_connection(P2PDataStore())
        tx_submitter.wait_for_verack()
        tx_submitter.send_txs_and_test(
            node_unique + shared, self.nodes[0], success=True)
        tx_submitter.peer_disconnect()

        return mininode_unique, node_unique, shared

    def run_test(self):
        self.mocktime = int(time.time())
        self.nodes[0].setmocktime(self.mocktime)
        self.blocks = self.generate(self.nodes[0], 512)
        self.sync_blocks()
