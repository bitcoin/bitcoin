# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Shared code for the bitcoin-privbcast functional tests (tool_privbcast*.py, p2p_private_broadcast.py).

The tool reaches the network only through a Tor SOCKS5 listener. Here that listener is the
test framework's SOCKS5 server: RESOLVE queries for the test seed names are answered from a
fixed script, and CONNECT requests are redirected to Python P2P listeners with chosen
behaviours, or to a bitcoind. Every wire-visible parameter is a constant in the tool;
regtest-only flags supply the seeds and a time divisor.
"""


import json
import platform
import signal
import subprocess
import tempfile
import threading

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
    assert_greater_than_or_equal,
    p2p_port,
)
from test_framework.v2_p2p import EncryptedP2PState
from test_framework.wallet import MiniWallet


# These tests run in real time. The tool is a separate process, so a test cannot move its clock; the
# regtest -timedivisor shortens every duration of the plan instead. The margins in the tests are
# sized for the slowest CI host, not for precision, because the tests check the process end to end.
# The node's jobs run on a mocked clock, in p2p_private_broadcast.py.
TIME_DIVISOR = 10  # scaled budgets (4.5 s handshake, 7.5 s request window, 1 s PONG) comfortably cover a real bitcoind recipient

# The Parameters of doc/design/private-broadcast-tool.md. The tool has no knobs, so the checks
# hardcode what they test against.
PRIVATE_VERSION = 70017
PRIVATE_USER_AGENT = "/pynode:0.0.1/"
REGTEST_PORT = 18444
SLOTS = 6
SLOT_TABLE = [(0, "prompt"), (1, "prompt"), (2, "prompt"), (3, "mid"), (4, "late"), (5, "late")]  # (slot, stratum)
OPPORTUNITIES_PER_SLOT = 4
DISCOVERY_WINDOW_S = 12
START_GRACE_S = 5
HANDSHAKE_TIMEOUT_S = 45
REQUEST_WINDOW_S = 75
PONG_WAIT_S = 10
BACKUP_MIN_S, BACKUP_MAX_S = 50, 60
MID_MIN_S, MID_MAX_S = 35, 180
LATE_MIN_S, LATE_MAX_S = 185, 240
PRIMARY_SEPARATION_S = 5
MAX_RECV_BYTES = 128 * 1024
MAX_STDIN_BYTES = 8_004_096


def check_profile(listener, wtxid):
    """What a scripted recipient was sent: the job's VERSION (E1), at most one INV, naming wtxid alone
    (E3), and only messages the profile sends (E7)."""
    assert_greater_than_or_equal(1, listener.message_count["inv"])
    inv = listener.last_message.get("inv")
    if inv is not None:
        assert_equal([(i.type, i.hash) for i in inv.inv], [(MSG_WTX, int(wtxid, 16))])
    assert set(listener.last_message) <= {"version", "wtxidrelay", "verack", "inv", "tx", "ping"}, listener.last_message
    v = listener.last_message.get("version")
    if v is not None:  # a recipient the tool could not talk to (a v1-only one) got none
        assert_equal((v.nVersion, v.nServices, v.strSubVer), (PRIVATE_VERSION, NODE_WITNESS, PRIVATE_USER_AGENT))
        assert_equal((v.nTime, v.nStartingHeight, v.relay), (0, 0, 0))
        assert_equal((v.addrTo.ip, v.addrTo.port, v.addrTo.nServices), ("0.0.0.0", 0, 0))
        assert_equal((v.addrFrom.ip, v.addrFrom.port, v.addrFrom.nServices), ("0.0.0.0", 0, NODE_WITNESS))


def check_schedule(report, divisor, tolerance_ms):
    """The slots and the shape of the schedule (Parameters, Interface/Report), every duration divided by divisor. The tool
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
            assert delivery_ms + lo * 1000 / divisor - tolerance_ms <= sched[0] <= delivery_ms + hi * 1000 / divisor + tolerance_ms, sched
        if s["stratum"] == "late":
            late.append(sched[0])
        for k in range(1, len(sched)):
            assert BACKUP_MIN_S * 1000 / divisor - tolerance_ms <= sched[k] - sched[k - 1] <= BACKUP_MAX_S * 1000 / divisor + tolerance_ms, sched
    assert_greater_than_or_equal(abs(late[0] - late[1]), PRIMARY_SEPARATION_S * 1000 / divisor - tolerance_ms)


def check_summary(report):
    """The summary counts attempts (Interface, Report): those dialled, and those that got as far as each event."""
    attempts = [a for s in report["slots"] for a in s["attempts"]]
    summary = report["summary"]
    assert_equal(summary["connections"], len(attempts))
    counters = {"announcements_handed": "inv_handed_ms", "announcements_written": "inv_written_ms",
                "tx_written": "tx_written_ms", "pongs": "pong_ms"}
    if "parent_txid" in report:
        counters["parents_served"] = "parent_tx_written_ms"
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


class PrivbcastToolTest(BitcoinTestFramework):
    """Runs bitcoin-privbcast through the framework's SOCKS5 server, against scripted recipients."""
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # The tool speaks v2 (BIP324) and never falls back to v1, so a node that
        # receives the transaction as an exit-path recipient must accept v2 (the mainnet default).
        self.extra_args = [["-v2transport=1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_bitcoin_privbcast()

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        raise NotImplementedError

    def setup_wallet(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 120)  # >100 for COINBASE_MATURITY, plus a margin of spendable UTXOs for every scenario

    def start_proxy(self, resolve_script, behaviours, node_endpoints=(), connect_delay=0, resolve_delay=0, proxy_authenticates=True, tor=True):
        """resolve_script: seed name -> list of answers, cycled per query.
        behaviours: endpoint address string -> (listener class, supports_v2).
        node_endpoints: endpoint address strings redirected to nodes[0].
        connect_delay: seconds the proxy stalls before answering any CONNECT (Tor building a circuit).
        resolve_delay: seconds the proxy stalls before answering any RESOLVE (a slow resolver), or a dict of
        seed name -> seconds, to stall only the RESOLVEs of those names.
        proxy_authenticates: if False the proxy lacks username and password authentication, the only method
        the tool offers, and answers "no acceptable method" (0xFF).
        tor: if False the proxy is an ordinary SOCKS5 proxy: no RESOLVE, no .onion."""
        self.listeners = {}
        self.listeners_lock = threading.Lock()
        self.connects = {}
        self.resolve_counts = {}

        def resolve_factory(name):
            with self.listeners_lock:
                n = self.resolve_counts.get(name, 0)
                self.resolve_counts[name] = n + 1
            # Stopping the server ends a stall, as it ends the server's own.
            if isinstance(resolve_delay, dict) and self.socks5_server.stopping.wait(resolve_delay.get(name, 0)):
                return None
            answers = resolve_script.get(name)
            if not answers:
                return None
            return answers[n % len(answers)]

        def destinations_factory(requested_to_addr, requested_to_port, proxy_client):
            with self.listeners_lock:
                self.connects[requested_to_addr] = self.connects.get(requested_to_addr, 0) + 1
                if requested_to_addr in node_endpoints:
                    return {"actual_to_addr": "127.0.0.1", "actual_to_port": p2p_port(0)}
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
                        # not log that expected close as an error. The tool never comes back over v1.
                        listener.reconnect = True
                    addr, port = start_p2p_listener(self.network_thread, listener)
                    self.listeners[requested_to_addr] = (listener, addr, port)
                _, addr, port = self.listeners[requested_to_addr]
                return {"actual_to_addr": addr, "actual_to_port": port}

        self.socks5_server = start_socks5_server(destinations_factory, resolve_factory, connect_reply_delay=connect_delay,
                                                 resolve_reply_delay=0 if isinstance(resolve_delay, dict) else resolve_delay,
                                                 auth=proxy_authenticates, tor=tor)

    def stop_proxy(self):
        self.socks5_server.stop()

    def tool_argv(self, *extra, chain="-regtest"):
        argv = self.get_binaries().privbcast_argv() + [chain, f"-tor=127.0.0.1:{self.socks5_server.conf.addr[1]}"]
        return argv + list(extra)

    def run_tool(self, *extra, stdin="", expected_rc=0, chain="-regtest", timeout=240):
        argv = self.tool_argv(*extra, chain=chain)
        self.log.debug(f"running {argv}")
        timeout *= self.options.timeout_factor
        try:
            proc = subprocess.run(argv, input=stdin, capture_output=True, text=True, timeout=timeout)
        except subprocess.TimeoutExpired:
            raise AssertionError(f"{argv} did not exit within {timeout} s")
        for line in proc.stderr.splitlines():
            self.log.debug(f"tool stderr: {line}")
        assert_equal(proc.returncode, expected_rc)
        return proc

    def run_send(self, tx_hex, *extra, expected_rc=0, time_divisor=TIME_DIVISOR):
        proc = self.run_tool(f"-timedivisor={time_divisor}", *extra, "send", stdin=tx_hex, expected_rc=expected_rc)
        report = json.loads(proc.stdout)
        self.log.debug(json.dumps(report, indent=1))
        check_summary(report)
        return report, proc

    def start_send(self, tx_hex, *extra, time_divisor=TIME_DIVISOR):
        """Start a send job in the background (transaction from a file, so the process owns no pipe)."""
        # Its own process group on Windows, so that interrupt() can target it alone with Ctrl-Break.
        creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if platform.system() == "Windows" else 0
        with tempfile.TemporaryFile("w+", encoding="utf8", dir=self.options.tmpdir) as stdin:
            stdin.write(tx_hex)
            stdin.seek(0)
            return subprocess.Popen(self.tool_argv(f"-timedivisor={time_divisor}", *extra, "send"),
                                    stdin=stdin, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                    creationflags=creationflags)

    def finish_send(self, proc):
        """Wait for a background send job; returns (returncode, report)."""
        out, err = proc.communicate(timeout=120 * self.options.timeout_factor)
        for line in err.splitlines():
            self.log.debug(f"tool stderr: {line}")
        report = json.loads(out)
        self.log.debug(json.dumps(report, indent=1))
        check_summary(report)
        return proc.returncode, report

    @staticmethod
    def interrupt(proc):
        """Interrupt a background job as a user would: SIGINT, or on Windows Ctrl-Break, which the tool handles the same way."""
        proc.send_signal(signal.CTRL_BREAK_EVENT if platform.system() == "Windows" else signal.SIGINT)

    @staticmethod
    def attempts(report):
        for s in report["slots"]:
            for a in s["attempts"]:
                yield s, a

    def attempts_to(self, report, endpoint):
        return [a for _, a in self.attempts(report) if a["endpoint"].startswith(endpoint)]
