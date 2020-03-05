#!/usr/bin/env python3
# Copyhigh (c) 2016-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reconciliation-based transaction relay protocol."""
from enum import IntEnum
from io import BytesIO
from test_framework.siphash import siphash256
import hashlib
import random
import struct
import time

from test_framework.key import TaggedHash
from test_framework.messages import (
    msg_inv, msg_getdata,
    msg_sendrecon, msg_reqrecon,
    msg_sketch, msg_reqsketchext, msg_reconcildiff,
    CTransaction, CInv
)
from test_framework.p2p import P2PDataStore, p2p_lock
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, hex_str_to_bytes

MSG_WTX = 5

# These parameters are specified in the BIP-0330.
Q_PRECISION = (2 << 14) - 1
FIELD_BITS = 32
FIELD_MODULUS = (1 << FIELD_BITS) + 0b10001101

# These parameters are suggested by the Erlay paper based on analysis and simulations.
DEFAULT_Q = 0.02
RECON_REQUEST_INTERVAL = 16
INVENTORY_BROADCAST_INTERVAL = 2
PENALIZED_INVENTORY_BROADCAST_INTERVAL = INVENTORY_BROADCAST_INTERVAL * 2
MAX_OUTBOUND_FLOOD_TO = 8

BYTES_PER_SKETCH_CAPACITY = FIELD_BITS / 8

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

def maybe_add_checksum(capacity):
    if capacity < 10:
        capacity += 1
    return capacity

def GetShortID(tx, salt):
    (k0, k1) = salt
    wtxid = tx.calc_sha256(with_witness = True)
    s = siphash256(k0, k1, wtxid)
    return 1 + (s & 0xFFFFFFFF)

# TestP2PConn: A peer we use to send messages to bitcoind, and store responses.
class TestP2PConn(P2PDataStore):
    def __init__(self, recon_version, local_salt):
        super().__init__()
        self.recon_version = recon_version
        self.local_salt = local_salt
        self.remote_salt = 0
        self.last_sendrecon = []
        self.last_sketch = []
        self.last_inv = []
        self.last_tx = []
        self.last_reqreconcil = []
        self.last_reconcildiff = []
        self.last_reqsketchext = []
        self.last_getdata = []
        self.remote_q = DEFAULT_Q
        self.last_wtxidrelay = []

    def on_sendrecon(self, message):
        self.last_sendrecon.append(message)
        self.remote_salt = message.salt

    def on_wtxidrelay(self, message):
        self.last_wtxidrelay.append(message)

    def on_sketch(self, message):
        self.last_sketch.append(message)

    def on_inv(self, message):
        MSG_BLOCK = 2
        for inv in message.inv:
            if inv.type != MSG_BLOCK: # ignore block invs
                self.last_inv.append(inv.hash)

    def on_tx(self, message):
        self.last_tx.append(message.tx.calc_sha256(True))

    def on_reqrecon(self, message):
        self.last_reqreconcil.append(message)

    def on_reqsketchext(self, message):
        self.last_reqsketchext.append(message)

    def on_reconcildiff(self, message):
        self.last_reconcildiff.append(message)

    def send_sendrecon(self, sender, responder):
        msg = msg_sendrecon()
        msg.salt = self.local_salt
        msg.version = self.recon_version
        msg.sender = sender
        msg.responder = responder
        self.send_message(msg)

    def send_reqrecon(self, set_size, q):
        msg = msg_reqrecon()
        msg.set_size = set_size
        msg.q = q
        self.send_message(msg)

    def send_sketch(self, skdata):
        msg = msg_sketch()
        msg.skdata = skdata
        self.send_message(msg)

    def send_reqsketchext(self):
        msg = msg_reqsketchext()
        self.send_message(msg)

    def send_reconcildiff(self, success, ask_shortids):
        msg = msg_reconcildiff()
        msg.success = success
        msg.ask_shortids = ask_shortids
        self.send_message(msg)

    def send_inv(self, inv_wtxids):
        msg = msg_inv(inv=[CInv(MSG_WTX, h=wtxid) for wtxid in inv_wtxids])
        self.send_message(msg)

    def send_getdata(self, ask_wtxids):
        msg = msg_getdata(inv=[CInv(MSG_WTX, h=wtxid) for wtxid in ask_wtxids])
        self.send_message(msg)

class ReconciliationTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.blocks_offset = 0

    def proceed_in_time(self, jump_in_seconds):
        time.sleep(0.01) # We usually need the node to process some messages first.
        self.mocktime += jump_in_seconds
        self.nodes[0].setmocktime(self.mocktime)

    def generate_txs(self, n_local_unique, n_remote_unique, n_shared, blocks, send=True):
        local_unique = []
        remote_unique = []
        shared = []

        for i in range(n_local_unique):
            tx = self.generate_transaction(self.nodes[0], blocks[self.blocks_offset + i])
            local_unique.append(tx)

        for i in range(n_local_unique, n_local_unique + n_remote_unique):
            tx = self.generate_transaction(self.nodes[0], blocks[self.blocks_offset + i])
            remote_unique.append(tx)

        for i in range(n_local_unique + n_remote_unique, n_local_unique + n_remote_unique + n_shared):
            tx = self.generate_transaction(self.nodes[0], blocks[self.blocks_offset + i])
            shared.append(tx)

        if send:
            self.nodes[0].p2ps[0].send_txs_and_test(remote_unique + shared, self.nodes[0], success = True)
            self.blocks_offset += (n_local_unique + n_remote_unique + n_shared)

        return local_unique, remote_unique, shared

    def compute_salt(self):
        RECON_STATIC_SALT = "Tx Relay Salting"
        salt1, salt2 = self.test_node.remote_salt, self.test_node.local_salt
        if salt1 > salt2:
            salt1, salt2 = salt2, salt1
        salt = struct.pack("<Q", salt1) + struct.pack("<Q", salt2)
        h = TaggedHash(RECON_STATIC_SALT, salt)
        k0 = int.from_bytes(h[0:8], "little")
        k1 = int.from_bytes(h[8:16], "little")
        return k0, k1

    def generate_transaction(self, node, coinbase):
        amount = 0.001
        to_address = node.getnewaddress()
        from_txid = node.getblock(coinbase)['tx'][0]
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = node.createrawtransaction(inputs, outputs)
        signresult = node.signrawtransactionwithwallet(rawtx)
        tx = CTransaction()
        tx.deserialize(BytesIO(hex_str_to_bytes(signresult['hex'])))
        tx.rehash()
        return tx

    # - No reconciliation messages until sendrecon is sent.
    def announce_reconciliation_support(self, test_node, sender, responder):
        def received_wtxidrelay():
            return (len(test_node.last_wtxidrelay) == 1)
        self.wait_until(received_wtxidrelay, timeout=30)

        def received_sendrecon():
            return (len(test_node.last_sendrecon) == 1)
        self.wait_until(received_sendrecon, timeout=30)

        with p2p_lock:
            assert_equal(test_node.last_sendrecon[0].version, test_node.recon_version)
            self.remote_salt = test_node.last_sendrecon[0].salt
            test_node.last_sendrecon = []
        test_node.send_sendrecon(sender, responder)

    def request_reconciliation(self, test_node, expected_sketch):
        local_set_size = 0
        q_to_send = int(DEFAULT_Q * Q_PRECISION)
        test_node.send_reqrecon(local_set_size, q_to_send)
        self.proceed_in_time(3) # Hit sending sketches out (m_next_recon_respond)

        def received_sketch():
            return (len(test_node.last_sketch) == 1)
        self.wait_until(received_sketch, timeout=30)
        assert_equal(test_node.last_sketch[-1].skdata, expected_sketch)
        test_node.last_sketch = []

    def transmit_sketch(self, test_node, txs_to_sketch, extension, capacity, expected_announced_txs):
        short_txids = [GetShortID(tx, self.compute_salt()) for tx in txs_to_sketch]
        if extension:
            sketch = create_sketch(short_txids, capacity * 2)[int(capacity * BYTES_PER_SKETCH_CAPACITY):]
        else:
            test_node.last_full_local_size = len(txs_to_sketch)
            sketch = create_sketch(short_txids, capacity)
        test_node.send_sketch(sketch)
        expected_wtxds = [tx.calc_sha256(True) for tx in expected_announced_txs]
        if len(expected_wtxds) > 0:
            def received_inv():
                return (len(test_node.last_inv) == len(expected_wtxds))
            self.wait_until(received_inv, timeout=30)
            assert_equal(set(test_node.last_inv), set(expected_wtxds))
            test_node.last_inv_size = len(test_node.last_inv) # Needed to recompute remote q later
            test_node.last_inv = []
        else:
            assert_equal(test_node.last_inv, [])

    def handle_reconciliation_finalization(self, test_node, expected_success, expected_requested_txs, after_extension=False):
        expected_requested_shortids = [GetShortID(tx, self.compute_salt()) for tx in expected_requested_txs]

        def received_reconcildiff():
            return (len(test_node.last_reconcildiff) == 1)
        self.wait_until(received_reconcildiff, timeout=30)
        success = test_node.last_reconcildiff[0].success
        assert_equal(success, int(expected_success))
        assert_equal(set(test_node.last_reconcildiff[0].ask_shortids), set(expected_requested_shortids))
        if success:
            local_missing = test_node.last_inv_size
            remote_missing = len(test_node.last_reconcildiff[0].ask_shortids)
            total_missing = local_missing + remote_missing
            local_size = test_node.last_full_local_size
            remote_size = local_size + local_missing - remote_missing
            min_size = min(local_size, remote_size)
            if min_size != 0:
                test_node.remote_q = (total_missing - abs(local_missing - remote_missing)) / min_size
        else:
            test_node.remote_q = DEFAULT_Q
        test_node.last_reconcildiff = []

    def handle_extension_request(self, test_node):
        def received_reqsketchext():
            return (len(test_node.last_reqsketchext) == 1)
        self.wait_until(received_reqsketchext, timeout=30)
        test_node.last_reqsketchext = []

    def request_sketchext(self, test_node, expected_sketch):
        test_node.send_reqsketchext()

        def received_sketch():
            return (len(test_node.last_sketch) > 0)
        self.wait_until(received_sketch, timeout=30)
        assert_equal(len(test_node.last_sketch), 1)
        assert_equal(test_node.last_sketch[0].skdata, expected_sketch)
        test_node.last_sketch = []

    def finalize_reconciliation(self, test_node, success, txs_to_request, txs_to_expect):
        ask_shortids = [GetShortID(tx, self.compute_salt()) for tx in txs_to_request]
        expected_wtxids = [tx.calc_sha256(with_witness = True) for tx in txs_to_expect]
        test_node.send_reconcildiff(success, ask_shortids)
        if txs_to_expect != []:
            def received_inv():
                return (len(test_node.last_inv) == len(txs_to_expect))
            self.wait_until(received_inv, timeout=30)
            assert_equal(set(test_node.last_inv), set(expected_wtxids))
            test_node.last_inv = []

    def request_transactions(self, test_node, txs_to_request):
        assert_equal(test_node.last_tx, []) # Make sure there were no unexpected transactions received before
        wtxids_to_request = [tx.calc_sha256(with_witness = True) for tx in txs_to_request]
        test_node.send_getdata(wtxids_to_request)

        def received_tx():
            return (len(test_node.last_tx) == len(txs_to_request))
        self.wait_until(received_tx, timeout=30)
        assert_equal(set([tx.calc_sha256(True) for tx in txs_to_request]), set(test_node.last_tx))
        test_node.last_tx = []

    # The mininode is an inbound connection to the actual node being tested.
    # The mininode will initiate reconciliations.
    def test_incoming_recon(self):
        LOCAL_SALT = random.randrange(0xffffff)
        self.blocks_offset = 0

        # Needed to submit txs
        self.test_node0 = self.nodes[0].add_p2p_connection(TestP2PConn(recon_version=1, local_salt=LOCAL_SALT))
        self.test_node0.wait_for_verack()

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn(recon_version=1, local_salt=LOCAL_SALT))
        self.test_node.wait_for_verack()

        self.mocktime = int(time.time())
        self.nodes[0].setmocktime(self.mocktime)

        self.announce_reconciliation_support(self.test_node, True, False)
        self.request_reconciliation(self.test_node, expected_sketch=[])
        self.finalize_reconciliation(self.test_node, success=True, txs_to_request=[], txs_to_expect=[])

        blocks = self.nodes[0].generate(nblocks=1024)
        self.sync_blocks()

        # Initial reconciliation succeeds
        local_txs, remote_txs, _ = self.generate_txs(10, 20, 0, blocks)
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        more_remote_txs = []
        expected_capacity = int(abs(len(remote_txs) - 0) + DEFAULT_Q * min(len(remote_txs), 0)) + 1
        remote_short_ids = [GetShortID(tx, self.compute_salt()) for tx in remote_txs]
        expected_sketch = create_sketch(remote_short_ids, expected_capacity)
        self.request_reconciliation(self.test_node, expected_sketch)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks)[1])
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        txs_to_request = random.sample(remote_txs, 3)
        self.finalize_reconciliation(self.test_node, True, txs_to_request, txs_to_expect=txs_to_request)
        self.request_transactions(self.test_node, txs_to_request)
        self.check_remote_recon_set_incoming(more_remote_txs)
        more_remote_txs = []

        # Initial reconciliation succeeds (0 txs on initiator side)
        local_txs, remote_txs, _ = self.generate_txs(0, 20, 0, blocks)
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        expected_capacity = int(abs(len(remote_txs) - 0) + DEFAULT_Q * min(len(remote_txs), 0)) + 1
        remote_short_ids = [GetShortID(tx, self.compute_salt()) for tx in remote_txs]
        expected_sketch = create_sketch(remote_short_ids, expected_capacity)
        self.request_reconciliation(self.test_node, expected_sketch)
        txs_to_request = random.sample(remote_txs, 3)
        self.finalize_reconciliation(self.test_node, True, txs_to_request, txs_to_expect=txs_to_request)
        self.request_transactions(self.test_node, txs_to_request)

        # Initial reconciliation succeeds (0 txs on initiator side, small sketch requires extra checksum syndrome)
        local_txs, remote_txs, _ = self.generate_txs(0, 1, 0, blocks, self)
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        expected_capacity = int(abs(len(remote_txs) - 0) + DEFAULT_Q * min(len(remote_txs), 0)) + 1 + 1
        remote_short_ids = [GetShortID(tx, self.compute_salt()) for tx in remote_txs]
        expected_sketch = create_sketch(remote_short_ids, expected_capacity)
        self.request_reconciliation(self.test_node, expected_sketch)
        txs_to_request = random.sample(remote_txs, 1)
        self.finalize_reconciliation(self.test_node, True, txs_to_request, txs_to_expect=txs_to_request)
        self.request_transactions(self.test_node, txs_to_request)

        # Initial reconciliation fails, extension succeeds
        _, remote_txs, _ = self.generate_txs(0, 20, 0, blocks, self)
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        more_remote_txs = []
        expected_capacity = int(abs(len(remote_txs) - 0) + DEFAULT_Q * min(len(remote_txs), 0)) + 1
        remote_short_ids = [GetShortID(tx, self.compute_salt()) for tx in remote_txs]
        expected_sketch = create_sketch(remote_short_ids, expected_capacity)
        self.request_reconciliation(self.test_node, expected_sketch)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        expected_sketch = create_sketch(remote_short_ids, expected_capacity * 2)[int(expected_capacity * BYTES_PER_SKETCH_CAPACITY):]
        self.request_sketchext(self.test_node, expected_sketch)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        txs_to_request = random.sample(remote_txs, 3)
        self.finalize_reconciliation(self.test_node, True, txs_to_request, txs_to_expect=txs_to_request)
        self.request_transactions(self.test_node, txs_to_request)
        self.check_remote_recon_set_incoming(more_remote_txs)

        # Initial reconciliation fails, extension fails
        _, remote_txs, _ = self.generate_txs(0, 20, 0, blocks, self)
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        more_remote_txs = []
        expected_capacity = int(abs(len(remote_txs) - 0) + DEFAULT_Q * min(len(remote_txs), 0)) + 1
        remote_short_ids = [GetShortID(tx, self.compute_salt()) for tx in remote_txs]
        expected_sketch = create_sketch(remote_short_ids, expected_capacity)
        self.request_reconciliation(self.test_node, expected_sketch)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        expected_sketch = create_sketch(remote_short_ids, expected_capacity * 2)[int(expected_capacity * BYTES_PER_SKETCH_CAPACITY):]
        self.request_sketchext(self.test_node, expected_sketch)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1)
        self.finalize_reconciliation(self.test_node, False, txs_to_request=[], txs_to_expect=remote_txs)
        self.request_transactions(self.test_node, remote_txs)
        self.check_remote_recon_set_incoming(more_remote_txs)

    def check_remote_recon_set_incoming(self, txs_to_expect):
        expected_capacity = int(abs(len(txs_to_expect) - 0) + DEFAULT_Q * min(len(txs_to_expect), 0)) + 1
        expected_capacity = maybe_add_checksum(expected_capacity)
        short_ids = [GetShortID(tx, self.compute_salt()) for tx in txs_to_expect]
        expected_sketch = create_sketch(short_ids, expected_capacity)
        self.request_reconciliation(self.test_node, expected_sketch)
        self.finalize_reconciliation(self.test_node, success=True, txs_to_request=[], txs_to_expect=[])



    def receive_reqreconcil(self, test_node, expected_set_size):
        def received_reqreconcil():
            return (len(test_node.last_reqreconcil) == 1)
        self.wait_until(received_reqreconcil, timeout=30)
        assert_equal(test_node.last_reqreconcil[-1].set_size, expected_set_size)
        assert_equal(test_node.last_reqreconcil[-1].q, int(test_node.remote_q * Q_PRECISION))
        test_node.last_reqreconcil = []

    # Mininode is an outgoing connection to the actual node being tested.
    # The actual node will initiate reconciliations.
    def test_outgoing_recon(self):
        LOCAL_SALT = random.randrange(0xffffff)
        self.blocks_offset = 0
        # These nodes will consume regular tx flood forwarding
        for i in range(MAX_OUTBOUND_FLOOD_TO):
            flooding_node = self.nodes[0].add_p2p_connection(TestP2PConn(recon_version=1, local_salt=LOCAL_SALT), node_outgoing=True)
            flooding_node.wait_for_verack()

        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn(recon_version=1, local_salt=LOCAL_SALT), node_outgoing=True)
        self.test_node.wait_for_verack()

        blocks = self.nodes[0].generate(nblocks=1024)
        self.sync_blocks()

        self.mocktime = int(time.time())
        self.nodes[0].setmocktime(self.mocktime)

        self.announce_reconciliation_support(self.test_node, False, True)
        self.check_remote_recon_set_outgoing([], 1) # Delay should be small here (1 sec), so that only 1 extra recon is triggered.

        # 20 on their side, 0 on our side, 0 shared, initial reconciliation succeeds (test for 0 on init side)
        local_unique_txs, remote_unique_txs, shared_txs = self.generate_txs(0, 20, 0, blocks, self)
        local_txs = local_unique_txs + shared_txs
        remote_txs = remote_unique_txs + shared_txs
        more_remote_txs = []
        self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)
        self.receive_reqreconcil(self.test_node, expected_set_size=len(remote_txs))
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=False,
            capacity=0, expected_announced_txs=remote_unique_txs)
        self.handle_reconciliation_finalization(self.test_node, expected_success=False,
            expected_requested_txs=local_unique_txs, after_extension=False)
        self.request_transactions(self.test_node, remote_unique_txs)
        self.check_remote_recon_set_outgoing([])

        # 20 on their side, 20 on our side, 10 shared, initial reconciliation succeeds
        local_unique_txs, remote_unique_txs, shared_txs = self.generate_txs(20, 20, 10, blocks, self)
        local_txs = local_unique_txs + shared_txs
        remote_txs = remote_unique_txs + shared_txs
        more_remote_txs = []
        self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)
        self.receive_reqreconcil(self.test_node, expected_set_size=len(remote_txs))
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=False,
            capacity=51, expected_announced_txs=remote_unique_txs)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_reconciliation_finalization(self.test_node, expected_success=True,
            expected_requested_txs=local_unique_txs, after_extension=False)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.request_transactions(self.test_node, remote_unique_txs)
        self.check_remote_recon_set_outgoing(more_remote_txs)

        # 20 on their side, 20 on our side, 10 shared, initial reconciliation fails, extension succeeds
        local_unique_txs, remote_unique_txs, shared_txs = self.generate_txs(20, 20, 10, blocks, self)
        local_txs = local_unique_txs + shared_txs
        remote_txs = remote_unique_txs + shared_txs
        more_remote_txs = []
        self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)
        self.receive_reqreconcil(self.test_node, expected_set_size=len(remote_txs))
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=False,
            capacity=30, expected_announced_txs=[])
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_extension_request(self.test_node)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=True,
            capacity=30, expected_announced_txs=remote_unique_txs)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_reconciliation_finalization(self.test_node, expected_success=True,
            expected_requested_txs=local_unique_txs, after_extension=True)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.request_transactions(self.test_node, remote_unique_txs)
        self.check_remote_recon_set_outgoing(more_remote_txs)

        # 20 their side, 20 on our side, 10 shared, initial reconciliation fails, extension fails
        local_unique_txs, remote_unique_txs, shared_txs = self.generate_txs(20, 20, 10, blocks, self)
        local_txs = local_unique_txs + shared_txs
        remote_txs = remote_unique_txs + shared_txs
        more_remote_txs = []
        self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)

        self.receive_reqreconcil(self.test_node, expected_set_size=len(remote_txs))
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=False,
            capacity=10, expected_announced_txs=[])
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_extension_request(self.test_node)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=True,
            capacity=10, expected_announced_txs=remote_txs)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_reconciliation_finalization(self.test_node, expected_success=False,
            expected_requested_txs=[], after_extension=True)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.request_transactions(self.test_node, remote_unique_txs)
        self.check_remote_recon_set_outgoing(more_remote_txs)

        # Transactions are not lost if empty sketch is sent initially
        _, remote_txs, _ = self.generate_txs(0, 5, 0, blocks, self)
        self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)
        self.receive_reqreconcil(self.test_node, expected_set_size=len(remote_txs))
        # These txs won't be in the set being reconciled here, because they will sit in setInventoryTxToSend for a bit
        remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1) # Let them flow from setInventoryTxToSend to the recon set
        time.sleep(0.1) # Moving to the recon set should happen before processing the sketch
        self.transmit_sketch(self.test_node, txs_to_sketch=[], extension=False,
            capacity=0, expected_announced_txs=remote_txs)
        self.handle_reconciliation_finalization(self.test_node, expected_success=False,
            expected_requested_txs=[], after_extension=False)
        self.check_remote_recon_set_outgoing([])

        # Transactions are not lost if empty sketch is sent during extension
        local_txs, remote_txs, _ = self.generate_txs(5, 5, 0, blocks, self)
        more_remote_txs = []
        self.proceed_in_time(RECON_REQUEST_INTERVAL + 1)
        self.receive_reqreconcil(self.test_node, expected_set_size=len(remote_txs))
        # These txs won't be in the set being reconciled here, because they will sit in setInventoryTxToSend for a bit
        remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.proceed_in_time(PENALIZED_INVENTORY_BROADCAST_INTERVAL + 1) # Let them flow from setInventoryTxToSend to the recon set
        time.sleep(0.1) # Moving to the recon set should happen before processing the sketch
        self.transmit_sketch(self.test_node, txs_to_sketch=local_txs, extension=False,
            capacity=5, expected_announced_txs=[])
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_extension_request(self.test_node)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.transmit_sketch(self.test_node, txs_to_sketch=[], extension=True, capacity=0,
            expected_announced_txs=remote_txs)
        more_remote_txs.extend(self.generate_txs(0, 1, 0, blocks, self)[1])
        self.handle_reconciliation_finalization(self.test_node, expected_success=False,
            expected_requested_txs=[], after_extension=True)
        self.check_remote_recon_set_outgoing(more_remote_txs)

    def check_remote_recon_set_outgoing(self, txs_to_expect, delay=RECON_REQUEST_INTERVAL + 1):
        self.proceed_in_time(delay)
        self.receive_reqreconcil(self.test_node, expected_set_size=len(txs_to_expect))
        self.transmit_sketch(self.test_node, txs_to_sketch=[], extension=False, capacity=0, expected_announced_txs=txs_to_expect)
        self.handle_reconciliation_finalization(self.test_node, expected_success=False,
            expected_requested_txs=[], after_extension=False)

    def run_test(self):
        self.test_incoming_recon()
        self.restart_node(0)
        self.test_outgoing_recon()

if __name__ == '__main__':
    ReconciliationTest().main()
