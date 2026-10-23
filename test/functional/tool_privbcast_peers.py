#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-privbcast against unsuitable and deviating recipients, the assignment of
candidates to slots, and a bitcoind as the recipient.

The harness and the shared scripted recipients are in test_framework/privbcast.py.
"""


from test_framework.messages import (
    CInv,
    MSG_TX,
    MSG_WITNESS_TX,
    MSG_WTX,
    NODE_WITNESS,
    msg_inv,
    msg_ping,
    msg_pong,
    msg_wtxidrelay,
)
from test_framework.p2p import (
    NetworkThread,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)


from test_framework.privbcast import (
    TIME_DIVISOR,
    SLOTS,
    OPPORTUNITIES_PER_SLOT,
    HANDSHAKE_TIMEOUT_S,
    PONG_WAIT_S,
    MAX_RECV_BYTES,
    Recipient,
    PrivbcastToolTest,
)


class NoWtxidRecipient(Recipient):
    """Does not send WTXIDRELAY: the tool must leave it before announcing anything."""

    def __init__(self):
        super().__init__()
        self.wtxidrelay = False


class LateWtxidRecipient(NoWtxidRecipient):
    """Sends WTXIDRELAY only after its VERACK, too late for BIP339: the tool must leave it unannounced."""

    def on_version(self, message):
        super().on_version(message)
        self.send_without_ping(msg_wtxidrelay())


class WrongNoncePongRecipient(Recipient):
    """Requests and receives X, then answers the PING with another nonce, which answers nothing."""

    def on_ping(self, message):
        self.ping_nonces.append(message.nonce)
        self.send_without_ping(msg_pong(message.nonce ^ 1))


class EarlyRequestRecipient(Recipient):
    """Asks for X by wtxid right after its VERSION, before any announcement, and never again: the early
    request must be ignored, not remembered."""

    def __init__(self, wtxid):
        super().__init__()
        self.wtxid = wtxid

    def on_version(self, message):
        self.send_version()
        self.request([self.wtxid])
        super().on_version(message)

    def on_inv(self, message):
        pass


class PingingRecipient(Recipient):
    """Sends a PING of its own after its VERACK: the tool answers nothing its profile does not send."""

    def on_version(self, message):
        super().on_version(message)
        self.send_without_ping(msg_ping(nonce=1))


class WrongFormRecipient(Recipient):
    """Asks for the announced transaction only in forms E5 does not answer: by its wtxid as MSG_WITNESS_TX
    and as MSG_TX, twice in one request, and as MSG_WTX naming another hash. It must never be served."""

    def on_inv(self, message):
        hashes = [i.hash for i in message.inv if i.type == MSG_WTX]
        self.request(hashes, inv_type=MSG_WITNESS_TX)
        self.request(hashes, inv_type=MSG_TX)
        self.request(hashes * 2)
        self.request([h ^ 1 for h in hashes])


class OldVersionRecipient(Recipient):
    """Announces protocol 70015, below the 70016 that BIP339 needs: the tool must leave it unannounced."""

    def peer_connect_send_version(self, services):
        super().peer_connect_send_version(services)
        self.on_connection_send_msg.nVersion = 70015


class NoWitnessRecipient(Recipient):
    """Does not offer NODE_WITNESS: the tool must leave it unannounced."""

    def peer_connect_send_version(self, services):
        super().peer_connect_send_version(services & ~NODE_WITNESS)


class FloodingRecipient(Recipient):
    """Floods the connection with announcements once the handshake is done, well past the receive cap,
    and never asks for the transaction, so only the cap can end its attempt early. One message per
    turn of the event loop: encrypting one takes long enough in Python that a burst would hold up the
    other scripted peers' handshakes."""
    MESSAGES = 10  # about 36 kB each

    def on_inv(self, message):
        pass

    def on_verack(self, message):
        super().on_verack(message)
        junk = msg_inv([CInv(MSG_TX, i) for i in range(1, 1001)])

        def flood(left):
            if left and self.is_connected:
                self.send_without_ping(junk)
                NetworkThread.network_event_loop.call_soon(flood, left - 1)
        flood(self.MESSAGES)


class ToolPrivbcastPeers(PrivbcastToolTest):
    def set_test_params(self):
        super().set_test_params()

    def run_test(self):
        self.setup_wallet()
        self.test_unsuitable_peers()
        self.test_peer_deviations()
        self.test_assignment()
        self.test_node_recipient()

    def test_unsuitable_peers(self):
        self.log.info("Peers below protocol 70016, without NODE_WITNESS or without WTXIDRELAY are left unannounced; "
                      "requests in the wrong form go unanswered; a flooding peer is cut at the receive cap")
        old, no_witness, no_wtxid, flooder, wrong_form = "9.3.0.1", "9.3.0.2", "9.3.0.3", "9.3.0.4", "9.3.0.5"
        self.start_proxy({"v.seed.": [old, no_witness], "w.seed.": [no_wtxid, flooder], "x.seed.": [wrong_form]},
                         {old: (OldVersionRecipient, True), no_witness: (NoWitnessRecipient, True),
                          no_wtxid: (NoWtxidRecipient, True), flooder: (FloodingRecipient, True),
                          wrong_form: (WrongFormRecipient, True)})
        tx = self.wallet.create_self_transfer()
        # The exit status is 0 because the wrong-form recipient is announced to (H2). Whether the flooder
        # was sent the INV before the cap cut it is a race, and only changes announcements_written.
        report, _ = self.run_send(tx["hex"], "-seed=v.seed.", "-seed=w.seed.", "-seed=x.seed.")
        for endpoint in (old, no_witness, no_wtxid):
            assert_equal([a["outcome"] for a in self.attempts_to(report, endpoint)], ["not_announced"])
            listener = self.listeners[endpoint][0]
            assert_equal(listener.message_count["inv"], 0)
            assert not listener.txs_received
        assert_equal(self.attempts_to(report, old)[0]["peer_version"], 70015)
        asked = self.attempts_to(report, wrong_form)
        assert_equal([a["outcome"] for a in asked], ["announced_not_requested"])
        assert_equal(asked[0]["extra_requests"], 4)  # each of its four requests got no reply
        assert not self.listeners[wrong_form][0].txs_received
        flooded = self.attempts_to(report, flooder)
        assert_equal(len(flooded), 1)
        a = flooded[0]
        assert a["outcome"] in ("not_announced", "post_announcement_failure"), a
        # Past the cap, then cut: well short of the announcements it was sent, and early.
        assert_greater_than_or_equal(a["bytes_recv"], MAX_RECV_BYTES)
        assert_greater_than(FloodingRecipient.MESSAGES * 36_000, a["bytes_recv"])
        assert_greater_than(HANDSHAKE_TIMEOUT_S * 1000 / TIME_DIVISOR, a["ended_ms"] - a["scheduled_start_ms"])
        self.stop_proxy()

    def test_peer_deviations(self):
        self.log.info("A PONG with another nonce, WTXIDRELAY after VERACK, a request before the announcement, a PING from the peer")
        tx = self.wallet.create_self_transfer()
        wrong_nonce, late_wtxid, early, pinging = "9.3.1.1", "9.3.1.2", "9.3.1.3", "9.3.1.4"
        wtxid = int(tx["wtxid"], 16)
        self.start_proxy({"y.seed.": [wrong_nonce, late_wtxid], "z.seed.": [early, pinging]},
                         {wrong_nonce: (WrongNoncePongRecipient, True), late_wtxid: (LateWtxidRecipient, True),
                          early: (lambda: EarlyRequestRecipient(wtxid), True), pinging: (PingingRecipient, True)})
        report, _ = self.run_send(tx["hex"], "-seed=y.seed.", "-seed=z.seed.")
        # A PONG without the PING's nonce answers nothing: the attempt ends when the wait runs out.
        a = self.attempts_to(report, wrong_nonce)
        assert_equal([x["outcome"] for x in a], ["tx_written_no_pong"])
        assert -5 <= a[0]["ended_ms"] - a[0]["ping_written_ms"] - PONG_WAIT_S * 1000 / TIME_DIVISOR <= 1000, a[0]
        # WTXIDRELAY after VERACK is too late.
        assert_equal([x["outcome"] for x in self.attempts_to(report, late_wtxid)], ["not_announced"])
        assert_equal(self.listeners[late_wtxid][0].message_count["inv"], 0)
        # A request before the announcement point is ignored, not remembered.
        assert_equal([x["outcome"] for x in self.attempts_to(report, early)], ["announced_not_requested"])
        assert_equal(self.listeners[early][0].message_count["inv"], 1)
        assert_equal(self.listeners[early][0].txs_received, [])
        # The peer's own PING gets no PONG, and the attempt goes on as with any other peer.
        assert_equal([x["outcome"] for x in self.attempts_to(report, pinging)], ["pong_received"])
        assert "pong" not in self.listeners[pinging][0].last_message
        self.stop_proxy()

    def test_assignment(self):
        self.log.info("Assignment: seeds spread over slots")
        # No endpoint is reachable, so every assigned opportunity is dialled and fails before announcing.
        # Six seeds of four fill every opportunity: each slot's opportunities come from distinct seeds,
        # and the six first attempts from six seeds (R5b).
        tx = self.wallet.create_self_transfer()
        seeds = {f"q{n}.seed.": [f"9.8.{n}.{i}" for i in range(1, 5)] for n in range(1, 7)}
        self.start_proxy(seeds, {})
        report, _ = self.run_send(tx["hex"], *[f"-seed={name}" for name in seeds], expected_rc=2)
        accepted = {st["name"]: st["accepted"] for st in report["discovery"]["seeds"]}
        if accepted != {name: 4 for name in seeds}:
            self.log.warning(f"host lost discovery answers ({accepted}): spread checks not applicable")
        else:
            first = []
            for s in report["slots"]:
                provenance = [a["provenance"] for a in s["attempts"]]
                assert_equal(len(provenance), OPPORTUNITIES_PER_SLOT)
                assert_equal(len(set(provenance)), len(provenance))
                first.append(provenance[0])
            assert_equal(len(set(first)), SLOTS)
        for _, a in self.attempts(report):
            assert a["provenance"] in seeds, a
        self.stop_proxy()

    def test_node_recipient(self):
        self.log.info("A bitcoind as the only recipient receives the transaction and accepts it")
        node = self.nodes[0]
        node_addr = "3.3.3.3"
        self.start_proxy({"c.seed.": [node_addr]}, {}, node_endpoints=(node_addr,))
        tx = self.wallet.create_self_transfer()
        # E8: selecting the user's own node makes it an ordinary first-hop relayer: it receives the
        # transaction over v2 and accepts it.
        proc = self.start_send(tx["hex"], "-seed=c.seed.")
        # The node has the transaction and the tool has hung up (it does after the PONG), so the
        # attempt is over; the job's later slots, which have no candidate, are then cut short.
        self.wait_until(lambda: tx["txid"] in node.getrawmempool() and not node.getpeerinfo())
        self.interrupt(proc)
        rc, report = self.finish_send(proc)
        assert_equal(rc, 0)
        # The only candidate is dialled once (R4).
        assert_equal([a["outcome"] for a in self.attempts_to(report, node_addr)], ["pong_received"])
        assert_equal(report["summary"]["announcements_written"], 1)
        self.stop_proxy()


if __name__ == '__main__':
    ToolPrivbcastPeers(__file__).main()
