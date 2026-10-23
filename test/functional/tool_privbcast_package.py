#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-privbcast's package mode (the extension): a child announced with its unconfirmed
parent served on request.

The harness and the shared scripted recipients are in test_framework/privbcast.py.
"""


from decimal import Decimal
import threading

from test_framework.messages import (
    CInv,
    MSG_TX,
    MSG_WITNESS_TX,
    MSG_WTX,
    msg_getdata,
    msg_notfound,
    msg_tx,
    tx_from_hex,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)


from test_framework.privbcast import (
    TIME_DIVISOR,
    REQUEST_WINDOW_S,
    PONG_WAIT_S,
    Recipient,
    PrivbcastToolTest,
)


PARENT_HOLD_S = 30


MAX_STANDARD_TX_WEIGHT = 400_000


# How long after the INV the late recipient asks for the child: half of PARENT_HOLD before the PING must go
# out (PONG_WAIT before the request window ends), so the whole hold no longer fits.
LATE_CHILD_REQUEST_S = (REQUEST_WINDOW_S - PONG_WAIT_S - PARENT_HOLD_S / 2) / TIME_DIVISOR


class PackageRecipient(Recipient):
    """A recipient in package mode that knows the parent's and child's ids."""

    def __init__(self, parent_txid, parent_wtxid, child_txid, child_wtxid):
        super().__init__()
        self.parent_txid, self.parent_wtxid = parent_txid, parent_wtxid
        self.child_txid, self.child_wtxid = child_txid, child_wtxid


class BatchedParentRecipient(PackageRecipient):
    """Resolves the child as a node lacking its parent does when the child's other inputs are confirmed
    coins it no longer remembers: it asks for every one of them by txid, ours last, in GETDATAs of up
    to 1000 entries, for as many inputs as a child of maximum standard weight can have. The request
    names the child's txid too, which the tool must neither serve nor call not found."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.unknown = [0x1000 + i for i in range(MAX_STANDARD_TX_WEIGHT // (4 * 41) - 2)]

    def on_tx(self, message):
        super().on_tx(message)
        if len(self.txs_received) == 1:
            wanted = [CInv(MSG_WITNESS_TX, h) for h in self.unknown + [self.child_txid, self.parent_txid]]
            for i in range(0, len(wanted), 1000):
                self.send_without_ping(msg_getdata(wanted[i:i + 1000]))


class BothNamedRecipient(PackageRecipient):
    """Asks for the child and the parent in one request before anything was served, which the tool must
    ignore outright, then for the child alone, then for the parent. Once it has the parent it asks for
    it again, with a transaction the tool does not have: after the parent phase nothing more is served
    and no NOTFOUND is sent."""

    def on_inv(self, message):
        self.send_without_ping(msg_getdata([CInv(MSG_WTX, self.child_wtxid), CInv(MSG_WITNESS_TX, self.parent_txid)]))
        self.send_without_ping(msg_getdata([CInv(MSG_WTX, self.child_wtxid)]))

    def on_tx(self, message):
        super().on_tx(message)
        if len(self.txs_received) == 1:
            self.send_without_ping(msg_getdata([CInv(MSG_WITNESS_TX, self.parent_txid)]))
        elif len(self.txs_received) == 2:
            self.send_without_ping(msg_getdata([CInv(MSG_WITNESS_TX, self.parent_txid), CInv(MSG_WITNESS_TX, 0x2000)]))


class ParentWrongFormRecipient(PackageRecipient):
    """After the child, asks for the parent only in forms F2 does not answer: as MSG_TX by txid and as
    MSG_WTX by wtxid. It is never sent the parent, and gets no NOTFOUND: both name the job's own parent."""

    def on_tx(self, message):
        super().on_tx(message)
        self.send_without_ping(msg_getdata([CInv(MSG_TX, self.parent_txid)]))
        self.send_without_ping(msg_getdata([CInv(MSG_WTX, self.parent_wtxid)]))


class LateChildRecipient(PackageRecipient):
    """Asks for the child late in the request window and never for the parent."""

    def on_inv(self, message):
        threading.Timer(LATE_CHILD_REQUEST_S, self.request, args=([self.child_wtxid],)).start()


class ChildOnlyPeer(P2PInterface):
    """Delivers a child to the node and answers the node's request for its parent with NOTFOUND,
    so the node keeps the child as an orphan and turns to the next announcer for the parent."""

    def on_getdata(self, message):
        self.send_without_ping(msg_notfound(vec=message.inv))


class ToolPrivbcastPackage(PrivbcastToolTest):
    def set_test_params(self):
        super().set_test_params()

    def run_test(self):
        self.setup_wallet()
        self.test_package_requests()
        self.test_package_node_recipient()
        self.test_package_existing_orphan()

    def test_package_requests(self):
        self.log.info("Package mode against scripted recipients: batched, both-named, wrong-form and unasked parent requests, and a late child request")
        parent = self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        ids = dict(parent_txid=int(parent["txid"], 16), parent_wtxid=int(parent["wtxid"], 16),
                   child_txid=int(child["txid"], 16), child_wtxid=int(child["wtxid"], 16))
        batched, both, unasked, wrong_form, late = "9.6.0.1", "9.6.0.2", "9.6.0.3", "9.6.1.1", "9.6.1.2"
        self.start_proxy({"p.seed.": [both, unasked], "q.seed.": [wrong_form, late]},
                         {both: (lambda: BothNamedRecipient(**ids), True),
                          unasked: (Recipient, True),
                          wrong_form: (lambda: ParentWrongFormRecipient(**ids), True),
                          late: (lambda: LateChildRecipient(**ids), True)})
        report, _ = self.run_send(child["hex"] + "\n" + parent["hex"], "-seed=p.seed.", "-seed=q.seed.")
        # Each was announced the child alone, by wtxid, once; the parent is never announced.
        def check_announced(report, endpoint):
            listener = self.listeners[endpoint][0]
            assert_equal(listener.message_count["inv"], 1)
            assert_equal([(i.type, i.hash) for i in listener.last_message["inv"].inv], [(MSG_WTX, ids["child_wtxid"])])
            assert set(listener.last_message) <= {"version", "wtxidrelay", "verack", "inv", "tx", "ping", "notfound"}, listener.last_message
            assert_equal([a["outcome"] for a in self.attempts_to(report, endpoint)], ["pong_received"])
        for endpoint in (both, unasked, wrong_form, late):
            check_announced(report, endpoint)
        # Both named before the child was served: ignored outright, no NOTFOUND; then served in turn.
        # The ignored request and the repeat after the parent are the two that got no reply.
        listener = self.listeners[both][0]
        assert_equal([t.txid_hex for t in listener.txs_received], [child["txid"], parent["txid"]])
        assert_equal(listener.notfound, [])
        assert_equal(self.attempts_to(report, both)[0]["extra_requests"], 2)
        # Never asked for the parent: the PING waited out the hold, then went.
        listener = self.listeners[unasked][0]
        assert_equal([t.txid_hex for t in listener.txs_received], [child["txid"]])
        a = self.attempts_to(report, unasked)[0]
        assert a["parent_hold_expired_ms"] is not None and a["parent_getdata_ms"] is None
        assert_greater_than_or_equal(a["ping_written_ms"] - a["tx_written_ms"], PARENT_HOLD_S * 1000 / TIME_DIVISOR - 100)
        assert_greater_than_or_equal(PARENT_HOLD_S * 1000 / TIME_DIVISOR + 1000, a["ping_written_ms"] - a["tx_written_ms"])
        # Asked for the parent only in forms F2 does not serve: never sent it, and no NOTFOUND, since
        # both name the job's own parent. The PING waited out the hold.
        listener = self.listeners[wrong_form][0]
        assert_equal([t.txid_hex for t in listener.txs_received], [child["txid"]])
        assert_equal(listener.notfound, [])
        a = self.attempts_to(report, wrong_form)[0]
        assert a["parent_getdata_ms"] is None and a["parent_hold_expired_ms"] is not None
        # Asked for the child late: the hold was cut so that the PING went out PONG_WAIT before the
        # request window ends.
        a = self.attempts_to(report, late)[0]
        cut_ms = (REQUEST_WINDOW_S - PONG_WAIT_S) * 1000 / TIME_DIVISOR
        assert -5 <= a["ping_written_ms"] - a["inv_handed_ms"] - cut_ms <= 1000, a
        self.stop_proxy()
        # The batched request runs alone at a smaller time divisor: the recipient decrypts about
        # 88 KB of NOTFOUND in Python before it reads the PING, which can outlast a 1 s PONG wait.
        self.start_proxy({"p.seed.": [batched]}, {batched: (lambda: BatchedParentRecipient(**ids), True)})
        proc = self.start_send(child["hex"] + "\n" + parent["hex"], "-seed=p.seed.", time_divisor=2)
        # The tool hangs up after the PONG; the job's later slots, which have no candidate, are then cut short.
        self.wait_until(lambda: batched in self.listeners and self.listeners[batched][0].message_count["ping"] == 1
                        and not self.listeners[batched][0].is_connected)
        self.interrupt(proc)
        rc, report = self.finish_send(proc)
        assert_equal(rc, 0)
        check_announced(report, batched)
        # The parent served once, NOTFOUND for exactly the entries that are not ours.
        listener = self.listeners[batched][0]
        assert_equal([t.txid_hex for t in listener.txs_received], [child["txid"], parent["txid"]])
        assert_equal(sorted(listener.notfound), listener.unknown)
        a = self.attempts_to(report, batched)[0]
        assert a["parent_tx_written_ms"] is not None
        self.stop_proxy()

    def test_package_node_recipient(self):
        self.log.info("A child with a low-fee parent: the node asks for the parent after the child and accepts both")
        node = self.nodes[0]
        node_addr = "3.3.3.4"
        self.start_proxy({"c.seed.": [node_addr]}, {}, node_endpoints=(node_addr,))
        # A zero-fee parent: below any relay floor on its own; the child pays for both.
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"))
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        alone = node.testmempoolaccept([parent["hex"]])[0]
        assert_equal(alone["allowed"], False)
        # testmempoolaccept does not apply the child's fee to the parent (no package feerates in test
        # accepts), so it cannot vouch for this package; the P2P 1p1c path below is the real check.
        # Child first on stdin: the tool orders the two by who spends whom. The node asks for the parent
        # 4 s after the child arrives (non-preferred plus txid-relay delay: the tool is a wtxid-relay
        # peer), which the parent hold must outlast; at TIME_DIVISOR the hold would be only 3 s.
        proc = self.start_send(child["hex"] + "\n" + parent["hex"], "-seed=c.seed.", time_divisor=5)
        # The node has both transactions and the tool has hung up (it does after the PONG), so the attempt
        # is over; the job's later slots, which have no candidate, are then cut short.
        self.wait_until(lambda: child["txid"] in node.getrawmempool() and parent["txid"] in node.getrawmempool()
                        and not node.getpeerinfo())
        self.interrupt(proc)
        rc, report = self.finish_send(proc)
        assert_equal(rc, 0)
        assert_equal(report["txid"], child["txid"])
        assert_equal(report["parent_txid"], parent["txid"])
        # The only candidate is dialled once (R4).
        assert_equal(report["summary"]["parents_served"], 1)
        for _, a in self.attempts(report):
            assert a["getdata_ms"] is not None and a["tx_written_ms"] is not None
            assert_greater_than_or_equal(a["parent_getdata_ms"], a["getdata_ms"])  # asked for only after the child was requested
            assert_equal(a["outcome"], "pong_received")
        self.stop_proxy()

    def test_package_existing_orphan(self):
        self.log.info("A node that already holds the child as an orphan asks the tool only for the parent")
        node = self.nodes[0]
        node_addr = "3.3.3.5"
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"))
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        # Another peer delivers the child first; the node orphans it and that peer cannot supply the parent.
        other = node.add_p2p_connection(ChildOnlyPeer())
        other.send_without_ping(msg_tx(tx_from_hex(child["hex"])))
        self.wait_until(lambda: child["txid"] in node.getorphantxs())
        other.wait_until(lambda: "getdata" in other.last_message)
        self.start_proxy({"e.seed.": [node_addr]}, {}, node_endpoints=(node_addr,))
        # Only a wtxid announcement adds the tool as an announcer of the existing orphan (BIP339). The
        # node then asks for the parent, never the child, 4 s later; see test_package_node_recipient.
        proc = self.start_send(child["hex"] + "\n" + parent["hex"], "-seed=e.seed.", time_divisor=5)
        # As above, but the other peer stays connected, so the tool's hanging up leaves one peer.
        self.wait_until(lambda: child["txid"] in node.getrawmempool() and parent["txid"] in node.getrawmempool()
                        and len(node.getpeerinfo()) == 1)
        self.interrupt(proc)
        rc, report = self.finish_send(proc)
        assert_equal(rc, 0)
        # The only candidate is dialled once (R4).
        assert_equal(report["summary"]["parents_served"], 1)
        for _, a in self.attempts(report):
            assert a["getdata_ms"] is None and a["tx_written_ms"] is None  # the child was never asked for
            assert_greater_than_or_equal(a["parent_getdata_ms"], a["inv_handed_ms"])
            assert_equal(a["outcome"], "pong_received")
        self.stop_proxy()
        node.disconnect_p2ps()


if __name__ == '__main__':
    ToolPrivbcastPackage(__file__).main()
