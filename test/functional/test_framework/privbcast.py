# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Shared code for the private broadcast functional tests (p2p_private_broadcast*.py).

A node with -privatebroadcast reaches the network only through its Tor SOCKS5 proxy. Here that
proxy is the test framework's SOCKS5 server: RESOLVE queries for the test seed names are answered
from a fixed script, and CONNECT requests are redirected to Python P2P listeners with chosen
behaviours, or to a bitcoind. Every wire-visible parameter is a constant of the release.
"""


import json
import threading

from test_framework.crypto.ellswift import xswiftec
from test_framework.crypto.secp256k1 import FE
from test_framework.messages import (
    CInv,
    MSG_WTX,
    NODE_WITNESS,
    msg_getdata,
)
from test_framework.netutil import format_addr_port
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    start_p2p_listener,
)
from test_framework.socks5 import (
    Command,
    start_socks5_server,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)
from test_framework.v2_p2p import EncryptedP2PState
from test_framework.wallet import MiniWallet


# The job tests run in real time: setmocktime moves a job's schedule but not the steady clock that
# times its proxy exchanges and connections, so the regtest -privatebroadcasttimedivisor shortens
# every duration of the plan instead. The margins in the tests are sized for the slowest CI host,
# not for precision, because the tests check the node end to end. p2p_private_broadcast.py runs
# jobs on a mocked clock instead.
TIME_DIVISOR = 10  # scaled budgets (4.5 s handshake, 7.5 s request window, 1 s PONG) comfortably cover a real bitcoind recipient


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


class PrivbcastJobTest(BitcoinTestFramework):
    """Runs private broadcast jobs in nodes[0] through the framework's SOCKS5 server, against scripted
    recipients and against nodes[1], a bitcoind."""
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        # A job speaks v2 (BIP324) and never falls back to v1, so the bitcoind recipient must accept v2.
        self.extra_args = [["-v2transport=1"], ["-v2transport=1"]]

    def run_test(self):
        raise NotImplementedError

    def setup_wallet(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 120)  # >100 for COINBASE_MATURITY, plus a margin of spendable UTXOs for every scenario
        # nodes[1] has the chain, and from here on hears of a transaction only from a job.
        self.disconnect_nodes(0, 1)

    def start_proxy(self, resolve_script, behaviours, proxy_authenticates=True, tor=True, time_divisor=TIME_DIVISOR):
        """Start a SOCKS5 server, and restart nodes[0] to run its jobs through it, on the seeds of resolve_script.
        resolve_script: seed name -> list of answers, cycled per query.
        behaviours: endpoint address string -> (listener class, supports_v2).
        proxy_authenticates: if False the proxy lacks username and password authentication, the only method
        a job offers, and answers "no acceptable method" (0xFF).
        tor: if False the proxy is an ordinary SOCKS5 proxy: no RESOLVE, no .onion.
        time_divisor: nodes[0]'s -privatebroadcasttimedivisor."""
        self.listeners = {}
        self.listeners_lock = threading.Lock()
        self.connects = {}
        self.resolve_counts = {}

        def resolve_factory(name):
            with self.listeners_lock:
                n = self.resolve_counts.get(name, 0)
                self.resolve_counts[name] = n + 1
            answers = resolve_script.get(name)
            if not answers:
                return None
            return answers[n % len(answers)]

        def destinations_factory(requested_to_addr, requested_to_port, proxy_client):
            with self.listeners_lock:
                self.connects[requested_to_addr] = self.connects.get(requested_to_addr, 0) + 1
                if requested_to_addr not in behaviours:
                    self.log.debug(f"unexpected connect to {format_addr_port(requested_to_addr, requested_to_port)}")
                    return None
                if requested_to_addr not in self.listeners:
                    cls, v2 = behaviours[requested_to_addr]
                    listener = cls()
                    listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
                    listener.peer_connect_send_version(services=P2P_SERVICES)
                    if v2:
                        listener.v2_state = RecordingV2State(initiating=False, net=self.chain)
                    else:
                        # A v1-only peer closes on the v2 handshake bytes; mark it so the framework does
                        # not log that expected close as an error. A job never comes back over v1.
                        listener.reconnect = True
                    addr, port = start_p2p_listener(self.network_thread, listener)
                    self.listeners[requested_to_addr] = (listener, addr, port)
                _, addr, port = self.listeners[requested_to_addr]
                return {"actual_to_addr": addr, "actual_to_port": port}

        self.socks5_server = start_socks5_server(destinations_factory, resolve_factory, auth=proxy_authenticates, tor=tor)
        self.restart_node(0, extra_args=[
            "-privatebroadcast",
            f"-onion=127.0.0.1:{self.socks5_server.conf.addr[1]}",
            *[f"-privatebroadcastseed={name}" for name in resolve_script],
            f"-privatebroadcasttimedivisor={time_divisor}",
            "-v2transport=1",
            "-debug=privatebroadcast",
        ])

    def stop_proxy(self):
        self.socks5_server.stop()

    def jobs(self):
        """nodes[0]'s jobs, by wtxid."""
        return {j["wtxid"]: j for j in self.nodes[0].getprivatebroadcastinfo()["jobs"]}

    def start_job(self, tx):
        """Submit a MiniWallet transaction to nodes[0], whose queue is empty: the job starts at once.
        Returns its wtxid."""
        assert_equal(self.nodes[0].sendrawtransaction(tx["hex"]), tx["txid"])
        return tx["wtxid"]

    def finish_job(self, wtxid):
        """Wait for a job to end; returns the job and its report."""
        self.wait_until(lambda: self.jobs()[wtxid]["state"] in ("done", "aborted"), timeout=240)
        job = self.jobs()[wtxid]
        self.log.debug(json.dumps(job, indent=1))
        check_summary(job["report"])
        return job, job["report"]

    def run_job(self, tx):
        """Run a job to its end; returns the job and its report."""
        return self.finish_job(self.start_job(tx))

    @staticmethod
    def attempts(report):
        for s in report["slots"]:
            for a in s["attempts"]:
                yield s, a

    def attempts_to(self, report, endpoint):
        return [a for _, a in self.attempts(report) if a["endpoint"].startswith(endpoint)]
