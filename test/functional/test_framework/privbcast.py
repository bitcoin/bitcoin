# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Shared code for the private broadcast functional tests (p2p_private_broadcast*.py).

A node with -privatebroadcast reaches the network only through its Tor SOCKS5 proxy. Here that
proxy is the test framework's SOCKS5 server: RESOLVE queries for the test seed names are answered
from a fixed script, and CONNECT requests are redirected to Python P2P listeners with chosen
behaviours, or to a bitcoind. Every wire-visible parameter is a constant of the release.
"""


from test_framework.crypto.ellswift import xswiftec
from test_framework.crypto.secp256k1 import FE
from test_framework.messages import (
    CInv,
    MSG_WTX,
    NODE_WITNESS,
    msg_getdata,
)
from test_framework.p2p import P2PInterface
from test_framework.socks5 import Command
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)
from test_framework.v2_p2p import EncryptedP2PState


# The Parameters of doc/design/private-broadcast.md. A release has no knobs, so the checks
# hardcode what they test against.
PRIVATE_VERSION = 70016
PRIVATE_USER_AGENT = "/pynode:0.0.1/"
REGTEST_PORT = 18444
SLOT_TABLE = [(0, "prompt"), (1, "prompt"), (2, "prompt"), (3, "mid"), (4, "late"), (5, "late")]  # (slot, stratum)
OPPORTUNITIES_PER_SLOT = 4
DISCOVERY_WINDOW_S = 12
HANDSHAKE_TIMEOUT_S = 45
BACKUP_MIN_S, BACKUP_MAX_S = 50, 60
MID_MIN_S, MID_MAX_S = 35, 180
LATE_MIN_S, LATE_MAX_S = 185, 240
PRIMARY_SEPARATION_S = 5
MAX_RECV_BYTES = 128 * 1024


def check_profile(listener, wtxid):
    """What a scripted recipient was sent: the job's VERSION (E1), at most one INV, naming wtxid alone
    (E3), and only messages the profile sends (E7)."""
    assert_greater_than_or_equal(1, listener.message_count["inv"])
    inv = listener.last_message.get("inv")
    if inv is not None:
        assert_equal([(i.type, i.hash) for i in inv.inv], [(MSG_WTX, int(wtxid, 16))])
    assert set(listener.last_message) <= {"version", "wtxidrelay", "verack", "inv", "tx", "ping"}, listener.last_message
    v = listener.last_message.get("version")
    if v is not None:  # a recipient the job could not talk to (a v1-only one) got none
        assert_equal((v.nVersion, v.nServices, v.strSubVer), (PRIVATE_VERSION, NODE_WITNESS, PRIVATE_USER_AGENT))
        assert_equal((v.nTime, v.nStartingHeight, v.relay), (0, 0, 0))
        assert_equal((v.addrTo.ip, v.addrTo.port, v.addrTo.nServices), ("0.0.0.0", 0, 0))
        assert_equal((v.addrFrom.ip, v.addrFrom.port, v.addrFrom.nServices), ("0.0.0.0", 0, NODE_WITNESS))


def ellswift_x(encoding):
    """The x coordinate a BIP324 public key encoding stands for: every encoding of one key gives the same."""
    return xswiftec(FE(int.from_bytes(encoding[:32], "big")), FE(int.from_bytes(encoding[32:], "big"))).to_bytes()


def check_fresh(listeners):
    """Nothing is shared between attempts but the transaction and the profile (A4): each recipient
    saw its own BIP324 key and VERSION nonce, and each PING carried its own nonce."""
    keys, version_nonces, ping_nonces = [], [], []
    for listener in listeners:
        state = getattr(listener, "v2_state", None)
        if state is not None and state.peer_ellswift is not None:
            keys.append(ellswift_x(state.peer_ellswift))
        if "version" in listener.last_message:
            version_nonces.append(listener.last_message["version"].nNonce)
        ping_nonces.extend(listener.ping_nonces)
    assert_greater_than(len(keys), 1)
    for values in (keys, version_nonces, ping_nonces):
        assert_equal(len(set(values)), len(values))


def check_schedule(report, divisor, tolerance_ms):
    """The slots and the shape of the schedule (Parameters, Interface/Report), every duration divided by divisor. A job
    rounds each scaled duration down to a millisecond, which tolerance_ms allows for."""
    assert_equal([(s["slot"], s["stratum"]) for s in report["slots"]], SLOT_TABLE)
    delivery_ms = DISCOVERY_WINDOW_S * 1000 / divisor
    late = []
    for s in report["slots"]:
        sched = s["scheduled_ms"]
        assert_equal(len(sched), OPPORTUNITIES_PER_SLOT)
        if s["stratum"] == "prompt":
            assert_greater_than_or_equal(tolerance_ms, abs(sched[0] - delivery_ms))
        else:
            lo, hi = (MID_MIN_S, MID_MAX_S) if s["stratum"] == "mid" else (LATE_MIN_S, LATE_MAX_S)
            assert_greater_than_or_equal(sched[0], delivery_ms + lo * 1000 / divisor - tolerance_ms)
            assert_greater_than_or_equal(delivery_ms + hi * 1000 / divisor + tolerance_ms, sched[0])
        if s["stratum"] == "late":
            late.append(sched[0])
        for k in range(1, len(sched)):
            interval = sched[k] - sched[k - 1]
            assert_greater_than_or_equal(interval, BACKUP_MIN_S * 1000 / divisor - tolerance_ms)
            assert_greater_than_or_equal(BACKUP_MAX_S * 1000 / divisor + tolerance_ms, interval)
    assert_greater_than_or_equal(abs(late[0] - late[1]), PRIMARY_SEPARATION_S * 1000 / divisor - tolerance_ms)


def check_summary(report):
    """The summary counts attempts (Interface, Report): those dialled, and those that got as far as each event."""
    attempts = [a for s in report["slots"] for a in s["attempts"]]
    summary = report["summary"]
    assert_equal(summary["connections"], len(attempts))
    counters = {"announcements_handed": "inv_handed_ms", "announcements_written": "inv_written_ms",
                "tx_written": "tx_written_ms", "pongs": "pong_ms"}
    for counter, event in counters.items():
        assert_equal(summary[counter], sum(a[event] is not None for a in attempts))


def query_sets(commands):
    """B2: every CONNECT has credentials of its own, and RESOLVEs share credentials only within a
    query set, which carries at most one query per seed name. Returns the query sets, each the seed
    names it asked for."""
    connects, sets = set(), {}
    for c in commands:
        assert c.username and c.password, c
        creds = (c.username, c.password)
        if c.cmd == Command.CONNECT:
            assert creds not in connects and creds not in sets, c
            connects.add(creds)
        else:
            assert creds not in connects, c
            names = sets.setdefault(creds, [])
            assert c.addr.decode() not in names, c
            names.append(c.addr.decode())
    return list(sets.values())


def drain_socks_commands(server):
    """The commands a SOCKS5 server has queued since the last call. An exception raised in one of its handlers is raised here."""
    commands = []
    while not server.queue.empty():
        item = server.queue.get()
        if isinstance(item, Exception):
            raise item
        commands.append(item)
    return commands


class Recipient(P2PInterface):
    """An honest recipient: negotiates wtxid relay (BIP339), as P2PInterface does by default,
    requests the announced transaction by wtxid and answers PING."""

    def __init__(self):
        super().__init__()
        self.txs_received = []
        self.ping_nonces = []
        self.notfound = []

    def request(self, hashes, inv_type=MSG_WTX):
        want = msg_getdata()
        for h in hashes:
            want.inv.append(CInv(inv_type, h))
        self.send_without_ping(want)

    def on_inv(self, message):
        self.request([i.hash for i in message.inv if i.type == MSG_WTX])

    def on_tx(self, message):
        self.txs_received.append(message.tx)

    def on_ping(self, message):
        self.ping_nonces.append(message.nonce)
        super().on_ping(message)

    def on_notfound(self, message):
        self.notfound.extend(i.hash for i in message.vec)


class RecordingV2State(EncryptedP2PState):
    """Keeps the initiator's ellswift key, so that a test can check each attempt used its own."""

    peer_ellswift = None

    def complete_handshake(self, response):
        start = response.tell()
        theirs = self.received_prefix + response.read(64 - len(self.received_prefix))
        response.seek(start)
        if len(theirs) == 64:
            self.peer_ellswift = theirs
        return super().complete_handshake(response)
