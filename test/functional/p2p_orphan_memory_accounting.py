#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the orphanage limits the memory used by orphans, not their weight.

A transaction's memory usage is not proportional to its weight: every witness stack
element is individually heap-allocated, so a transaction of standard weight whose
witness is a stack of many tiny elements uses roughly 28 times as much memory as its
weight suggests. Witness standardness, which would restrict this, cannot be checked
while a transaction's inputs are missing. If the orphanage limited weight, a peer
could therefore pin megabytes of memory with a single standard-weight orphan, and one
such orphan per connection.
"""
import time

from test_framework.mempool_util import (
    create_large_orphan,
    tx_in_orphanage,
)
from test_framework.messages import (
    CInv,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    MSG_WTX,
    msg_inv,
    msg_tx,
)
from test_framework.p2p import (
    NONPREF_PEER_TX_DELAY,
    OVERLOADED_PEER_TX_DELAY,
    P2PInterface,
    TXID_RELAY_DELAY,
)
from test_framework.script import CScript, OP_RETURN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)

TXREQUEST_TIME_SKIP = NONPREF_PEER_TX_DELAY + TXID_RELAY_DELAY + OVERLOADED_PEER_TX_DELAY + 1
# MAX_ORPHAN_TX_USAGE, the maximum memory usage of an orphan.
MAX_ORPHAN_TX_USAGE = 600_000
# DEFAULT_RESERVED_ORPHAN_USAGE_PER_PEER, the memory usage the orphanage reserves for each peer.
RESERVED_ORPHAN_USAGE_PER_PEER = 604_000
# Approximate memory usage of a 1-byte witness stack element: its slot in the stack vector plus a
# minimally-sized heap allocation. This is an upper bound: it is about half of this on platforms with
# 32-bit pointers, so tests must not depend on the exact value.
APPROX_USAGE_PER_ELEMENT = 24 + 32


def witness_dense_orphan(num_elements, seq=0):
    """Create an orphan whose witness is a stack of num_elements 1-byte elements.

    Its weight is about 2 * num_elements, but its memory usage is about 56 * num_elements.
    """
    tx = CTransaction()
    tx.vin = [CTxIn(COutPoint(0xdeadbeef, 0), nSequence=seq)]
    tx.wit.vtxinwit = [CTxInWitness()]
    tx.wit.vtxinwit[0].scriptWitness.stack = [b"\x01"] * num_elements
    tx.vout = [CTxOut(100, CScript([OP_RETURN, b"a" * 20]))]
    return tx


class OrphanMemoryAccountingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.nodes[0].setmocktime(int(time.time()))
        self.test_memory_fills_up_the_allowance()
        self.test_memory_dense_orphan_not_stored()
        self.test_memory_dense_orphans_do_not_scale()
        self.test_large_standard_orphan_stored()

    def send_orphan(self, peer, tx):
        """Announce tx by wtxid, then serve it when it is requested."""
        node = self.nodes[0]
        peer.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=int(tx.wtxid_hex, 16))]))
        node.bumpmocktime(TXREQUEST_TIME_SKIP)
        peer.wait_until(lambda: peer.last_message.get("getdata"))
        peer.send_and_ping(msg_tx(tx))
        node.bumpmocktime(TXREQUEST_TIME_SKIP)

    def disconnect_and_clear(self):
        self.nodes[0].disconnect_p2ps()
        self.wait_until(lambda: len(self.nodes[0].getorphantxs()) == 0)

    def test_memory_fills_up_the_allowance(self):
        self.log.info("Test that what fills up a peer's allowance is memory usage, not weight")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())

        # Each of these orphans is memory-dense but featherweight: it uses roughly an eighth of what a
        # peer is granted (a sixteenth where a witness element costs half as much memory), while
        # weighing a small fraction of that. Send them until one of them causes an eviction, which
        # must happen long before their combined weight approaches the allowance. If usage were
        # weight, none of them would ever be evicted.
        num_elements = RESERVED_ORPHAN_USAGE_PER_PEER // (8 * APPROX_USAGE_PER_ELEMENT)
        orphans = [witness_dense_orphan(num_elements, seq=i) for i in range(24)]

        num_stored = 0
        for num_sent, tx in enumerate(orphans, start=1):
            self.send_orphan(peer, tx)
            in_orphanage = len(node.getorphantxs())
            if in_orphanage <= num_stored:
                break
            num_stored = in_orphanage
        else:
            raise AssertionError("no orphan was evicted: usage is not accounted as memory")

        self.log.info(f"Eviction after {num_sent} orphans of {orphans[0].get_weight()}WU each")
        assert_greater_than(num_sent, 1)
        assert_greater_than(RESERVED_ORPHAN_USAGE_PER_PEER, sum(tx.get_weight() for tx in orphans[:num_sent]))
        # The oldest one is the one that was evicted.
        assert not tx_in_orphanage(node, orphans[0])
        assert tx_in_orphanage(node, orphans[num_sent - 1])
        self.disconnect_and_clear()

    def test_memory_dense_orphan_not_stored(self):
        self.log.info("Test that an orphan using more memory than allowed is not stored")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())

        # A normal orphan, which must not be affected by what follows.
        small = witness_dense_orphan(1)
        self.send_orphan(peer, small)
        assert tx_in_orphanage(node, small)

        # Enough elements to use several times the memory that may be stored, while still being of
        # standard weight. If usage were measured in weight, this would fit into the peer's allowance
        # and stay there forever.
        num_elements = 4 * MAX_ORPHAN_TX_USAGE // APPROX_USAGE_PER_ELEMENT
        tx = witness_dense_orphan(num_elements)
        assert_greater_than(400_000, tx.get_weight())
        self.send_orphan(peer, tx)

        assert not tx_in_orphanage(node, tx)
        # The peer's other orphan was not pushed out to make room for it.
        assert tx_in_orphanage(node, small)
        assert_equal(len(node.getorphantxs()), 1)
        self.disconnect_and_clear()

    def test_memory_dense_orphans_do_not_scale(self):
        self.log.info("Test that memory-dense orphans cannot be spread over many connections")
        node = self.nodes[0]
        num_elements = 4 * MAX_ORPHAN_TX_USAGE // APPROX_USAGE_PER_ELEMENT

        # The orphanage's global limit grows with the number of peers, but each of these
        # transactions is refused on its own, so none of them is retained.
        for i in range(10):
            peer = node.add_p2p_connection(P2PInterface())
            self.send_orphan(peer, witness_dense_orphan(num_elements, seq=i))
        assert_equal(len(node.getorphantxs()), 0)

        # Normal orphans are still accepted afterwards.
        peer = node.add_p2p_connection(P2PInterface())
        small = witness_dense_orphan(1, seq=1)
        self.send_orphan(peer, small)
        assert tx_in_orphanage(node, small)
        self.disconnect_and_clear()

    def test_large_standard_orphan_stored(self):
        self.log.info("Test that an orphan of maximum standard weight is still stored")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())

        # All of this transaction's data is in a single witness element, so its memory usage is
        # close to its size and it fits within a peer's allowance.
        large = create_large_orphan()
        assert_greater_than(large.get_weight(), 390_000)
        self.send_orphan(peer, large)
        assert tx_in_orphanage(node, large)
        self.disconnect_and_clear()


if __name__ == "__main__":
    OrphanMemoryAccountingTest(__file__).main()
