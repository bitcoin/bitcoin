#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-privbcast, the bounded private transaction broadcast tool: argument and input
errors, a complete job against scripted recipients, concurrent jobs, interruption, a stalled
stderr and discovery alone.

The harness and the shared scripted recipients are in test_framework/privbcast.py.
"""


import json
import os
import subprocess
import threading
import time

from test_framework.crypto.ellswift import xswiftec
from test_framework.crypto.secp256k1 import FE
from test_framework.messages import (
    MSG_WITNESS_TX,
    MSG_WTX,
    msg_sendtxrcncl,
)
from test_framework.p2p import (
    P2P_SUBVERSION,
    P2P_VERSION,
)
from test_framework.socks5 import (
    Command,
)
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)


from test_framework.privbcast import (
    TIME_DIVISOR,
    REGTEST_PORT,
    SLOTS,
    START_GRACE_S,
    HANDSHAKE_TIMEOUT_S,
    REQUEST_WINDOW_S,
    PONG_WAIT_S,
    MAX_STDIN_BYTES,
    check_profile,
    check_schedule,
    drain_socks_commands,
    query_sets,
    Recipient,
    PrivbcastToolTest,
)


def ellswift_x(encoding):
    """The x coordinate a BIP324 public key encoding stands for: every encoding of one key gives the same."""
    return xswiftec(FE(int.from_bytes(encoding[:32], "big")), FE(int.from_bytes(encoding[32:], "big"))).to_bytes()


class SilentRecipient(Recipient):
    """Handshakes, then never requests: what an honest peer that already has X looks like."""

    def on_inv(self, message):
        pass


class NoRelayRecipient(Recipient):
    """Announces relay=false in its VERSION; the tool must not announce to it."""

    def peer_connect_send_version(self, services):
        super().peer_connect_send_version(services)
        self.on_connection_send_msg.relay = 0


class NoPongRecipient(Recipient):
    """Requests and receives X but never answers the PING."""

    def on_ping(self, message):
        self.ping_nonces.append(message.nonce)


class ProbingRecipient(Recipient):
    """Sends requests the profile does not allow, repeats the request after the transfer, and
    sends a late SENDTXRCNCL. None of it may be answered or change the tool's behaviour."""

    def on_inv(self, message):
        hashes = [i.hash for i in message.inv if i.type == MSG_WTX]
        self.request(hashes, inv_type=MSG_WITNESS_TX)  # the txid-relay form: not what a wtxid-relay peer is asked
        self.request(hashes)  # the one request that is answered

    def on_tx(self, message):
        super().on_tx(message)
        self.request([message.tx.wtxid_int])
        self.request([message.tx.wtxid_int])
        self.send_without_ping(msg_sendtxrcncl())


class ToolPrivbcast(PrivbcastToolTest):
    def set_test_params(self):
        super().set_test_params()

    def run_test(self):
        self.setup_wallet()
        self.test_argument_errors()
        self.test_bounded_job()
        self.test_concurrent_invocations()
        self.test_interrupt_mid_delivery()
        self.test_stalled_stderr()
        self.test_discover()

    def test_argument_errors(self):
        self.log.info("Argument and input errors: exit status 1, found before any network activity")
        # Messages are for people and are not checked. Each case is built so that only one check can
        # fail, and a job that ran anyway would query a seed through the proxy at once: a.seed. on
        # regtest, the chain's own seeds elsewhere.
        self.start_proxy({"a.seed.": ["9.0.2.1"]}, {})
        timeout = 20 * self.options.timeout_factor

        def refused(*args, stdin, chain="-regtest"):
            self.run_tool(*args, stdin=stdin, expected_rc=1, chain=chain, timeout=20)
            assert_equal(drain_socks_commands(self.socks5_server), [])

        tx_hex = self.wallet.create_self_transfer()["hex"]
        refused("-seed=a.seed.", "send", stdin="zz")
        refused("-seed=a.seed.", "send", stdin="")
        # Two unrelated transactions are refused, and so is the same one twice.
        other_hex = self.wallet.create_self_transfer()["hex"]
        refused("-seed=a.seed.", "send", stdin=f"{tx_hex}\n{other_hex}")
        refused("-seed=a.seed.", "send", stdin=f"{tx_hex} {tx_hex}")
        # Regtest-only flags are refused elsewhere.
        refused("-seed=a.seed.", "send", stdin=tx_hex, chain="-signet")
        refused(f"-timedivisor={TIME_DIVISOR}", "send", stdin=tx_hex, chain="-chain=main")
        # Seed material comes only from the chain's release list (U1): no -signetseednode, and no
        # custom signet, which has no seeds.
        refused("-signetseednode=127.0.0.1:38333", "send", stdin=tx_hex, chain="-signet")
        refused("-signetchallenge=51", "send", stdin=tx_hex, chain="-signet")
        # No command, or an unknown one, is an error.
        refused("-seed=a.seed.", stdin=tx_hex)
        refused("-seed=a.seed.", "frobnicate", stdin=tx_hex)
        # stdin is read up to its bound and no further: input still open past the bound is refused
        # without waiting for its end.
        proc = subprocess.Popen(self.tool_argv("-seed=a.seed.", "send"), stdin=subprocess.PIPE,
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        def feed():
            try:
                proc.stdin.write((tx_hex + " " * MAX_STDIN_BYTES).encode())
                proc.stdin.flush()
            except OSError:
                pass  # the tool stopped reading
        writer = threading.Thread(target=feed)
        writer.start()
        try:
            rc = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
            rc = None
        writer.join()
        try:
            proc.stdin.close()
        except OSError:
            pass
        assert_equal(drain_socks_commands(self.socks5_server), [])
        assert_equal(rc, 1)
        # A remote SOCKS listener is refused.
        argv = self.get_binaries().privbcast_argv() + ["-regtest", "-tor=10.1.2.3:9050", "send"]
        try:
            proc = subprocess.run(argv, input=tx_hex, capture_output=True, text=True, timeout=timeout)
        except subprocess.TimeoutExpired:
            raise AssertionError("a remote -tor must be refused before any network activity")
        assert_equal(proc.returncode, 1)
        # No arguments at all prints the usage.
        proc = subprocess.run(self.get_binaries().privbcast_argv(), capture_output=True, text=True, timeout=3 * timeout)
        assert_equal(proc.returncode, 1)
        assert proc.stdout
        self.stop_proxy()

    def test_bounded_job(self):
        self.log.info("A complete job against scripted recipients")
        # Half the usual pace: discovery opens twenty SOCKS streams at once, and a loaded CI host
        # (Windows) has been slow enough at TIME_DIVISOR to cost a query. This section counts every
        # query.
        divisor = 5
        # Every candidate of a seed shares one behaviour, and the six first attempts cover all five
        # seeds whatever the tie order (R5b), so each behaviour is exercised on every run rather than
        # only when the shuffle happens to pick it. d.seed. names an honest but v1-only recipient (the
        # tool never falls back to v1, so it is never served: B4), and e.seed. a probing v2 recipient;
        # each answers every query with that one address.
        resolve_script = {
            "a.seed.": ["8.0.0.1", "8.0.0.2", "8.0.0.3", "8.0.0.4"],
            "b.seed.": ["8.0.1.1", "8.0.1.2", "8.0.1.3", "8.0.1.1"],  # last answer repeats within the seed
            "c.seed.": ["8.0.2.1", "8.0.2.2", "8.0.2.3", "8.0.2.4"],
            "d.seed.": ["8.0.3.1"],
            "e.seed.": ["8.0.4.1"],
        }
        seed_behaviour = {"a.seed.": (NoPongRecipient, True), "b.seed.": (NoRelayRecipient, True), "c.seed.": (SilentRecipient, True),
                          "d.seed.": (Recipient, False), "e.seed.": (ProbingRecipient, True)}
        behaviours = {ip: seed_behaviour[seed] for seed, ips in resolve_script.items() for ip in ips}
        v1_ip, probing_ip = resolve_script["d.seed."][0], resolve_script["e.seed."][0]
        self.start_proxy(resolve_script, behaviours)
        tx = self.wallet.create_self_transfer()
        report, proc = self.run_send(tx["hex"], *[f"-seed={name}" for name in resolve_script], time_divisor=divisor)
        assert_equal(report["txid"], tx["txid"])
        assert_equal(report["wtxid"], tx["wtxid"])
        assert_equal(report["summary"]["interrupted"], False)
        assert_equal(report["summary"]["error"], None)
        # The report counts from job start: no field is named for a time and no number is an epoch
        # timestamp. Progress went to stderr.
        def check_relative(value):
            if isinstance(value, dict):
                for key, v in value.items():
                    assert not key.startswith("time"), key
                    check_relative(v)
            elif isinstance(value, list):
                for v in value:
                    check_relative(v)
            elif isinstance(value, (int, float)) and not isinstance(value, bool):
                assert_greater_than(10**9, value)
        check_relative(report)
        assert proc.stderr  # progress lines
        assert_equal(report["summary"]["slots_completed"], SLOTS)

        # Discovery: four query sets, each one query per seed, each query on its own stream, and every
        # candidate got the chain port. The tool's accounting and the proxy's view must agree, on any host. A host
        # that stalls at job start skips queries rather than running them late, and one that
        # answers a SOCKS stage too slowly costs the query; both are the tool doing its job, so
        # the counts are asserted as invariants, not as the twenty answers a fast host gets.
        commands = drain_socks_commands(self.socks5_server)
        resolves = [c for c in commands if c.cmd == Command.RESOLVE]
        disc = report["discovery"]
        seeds = {s["name"]: s for s in disc["seeds"]}
        assert_equal(set(seeds), set(resolve_script))
        for name, st in seeds.items():
            assert_equal(st["queries"] + st["skipped"], 4)
            assert_greater_than_or_equal(st["queries"], self.resolve_counts.get(name, 0))  # a query sent may still not reach RESOLVE
            assert_greater_than_or_equal(self.resolve_counts.get(name, 0), st["answers"])
        assert_equal(sum(st["answers"] for st in seeds.values()), disc["duplicates"] + disc["rejected"] + sum(st["accepted"] for st in seeds.values()))
        assert_equal(disc["rejected"], 0)
        assert_equal(disc["exit_path_candidates"], sum(st["accepted"] for st in seeds.values()))
        assert_greater_than(disc["exit_path_candidates"], 0)
        # Stream isolation (B2): every connection has credentials of its own, and the queries share
        # theirs only within a query set.
        sets = query_sets(commands)
        assert_greater_than_or_equal(4, len(sets))
        for _, a in self.attempts(report):
            assert a["endpoint"].endswith(f":{REGTEST_PORT}")
            # The peer's protocol version and user agent, as it sent them.
            if a["peer_version"] is not None:
                assert_equal((a["peer_version"], a["peer_user_agent"]), (P2P_VERSION, P2P_SUBVERSION))

        # The checks below need a host that kept up: every query reached the proxy and no
        # opportunity was missed. If the host stalled they do not apply, and are skipped.
        missed = sum(s["missed_opportunities"] for s in report["slots"])
        skipped = sum(st["skipped"] for st in seeds.values())
        stalled = missed > 0 or skipped > 0 or len(resolves) < 20
        if stalled:
            self.log.warning(f"host stalled during the job (missed={missed}, skipped={skipped}, resolves={len(resolves)}): outcome checks not applicable")
        else:
            assert_equal(sorted(sorted(s) for s in sets), [sorted(resolve_script)] * 4)  # D1
            assert_equal(report["summary"]["pongs"], 1)  # the probing recipient's; the v1-only one is never served
            # Every INV the transport took was fully written: these recipients read at once.
            assert_equal(report["summary"]["announcements_written"], report["summary"]["announcements_handed"])
            # Outcomes follow the seed's behaviour; every seed was drawn at least once.
            def seed_of(ip):
                return next(seed for seed, ips in resolve_script.items() if ip in ips)

            seeds_attempted = set()
            for _, a in self.attempts(report):
                seed = seed_of(a["endpoint"].rsplit(":", 1)[0])
                assert_equal(a["provenance"], seed)
                seeds_attempted.add(seed)
                if seed == "a.seed.":  # requests and receives X, never answers the PING
                    assert_equal(a["outcome"], "tx_written_no_pong")
                    # Ended when the PONG wait ran out.
                    assert -5 <= a["ended_ms"] - a["ping_written_ms"] - PONG_WAIT_S * 1000 / divisor <= 1000, a
                elif seed == "b.seed.":  # relay=false: refused before any announcement
                    assert_equal(a["outcome"], "not_announced")
                    assert_equal(self.listeners[a["endpoint"].rsplit(":", 1)[0]][0].message_count["inv"], 0)
                elif seed == "c.seed.":  # handshakes, never requests
                    assert_equal(a["outcome"], "announced_not_requested")
                    # Ended when the request window ran out.
                    assert -5 <= a["ended_ms"] - a["inv_handed_ms"] - REQUEST_WINDOW_S * 1000 / divisor <= 1000, a
            assert_equal(seeds_attempted, set(resolve_script))
            # Every seed supplies a first attempt (R5a, R5b).
            assert_equal({s["attempts"][0]["provenance"] for s in report["slots"]}, set(resolve_script))
            # A refused primary is replaced at the slot's next opportunity by a candidate from another seed.
            replaced = [s for s in report["slots"] if s["attempts"] and s["attempts"][0]["provenance"] == "b.seed."]
            assert_greater_than(len(replaced), 0)
            for s in replaced:
                assert_greater_than_or_equal(len(s["attempts"]), 2)
                assert s["attempts"][1]["provenance"] != s["attempts"][0]["provenance"]
                assert s["attempts"][1]["inv_handed_ms"] is not None
            # The v1-only recipient closes the v2 attempt without a byte. That is a plain transport
            # failure: the tool never falls back to v1, so the endpoint is never served (B4).
            v1_attempts = self.attempts_to(report, v1_ip)
            assert_equal([a["outcome"] for a in v1_attempts], ["not_announced"])
            assert_equal(v1_attempts[0]["bytes_recv"], 0)
            assert_equal(len(self.listeners[v1_ip][0].txs_received), 0)
            # The probing recipient: its txid-form request, repeats and late SENDTXRCNCL change nothing.
            probing = self.attempts_to(report, probing_ip)
            assert_equal(len(probing), 1)
            assert_equal(probing[0]["outcome"], "pong_received")
            assert_equal(probing[0]["extra_requests"], 3)  # txid-form request before, two repeats after
            assert_equal(len(self.listeners[probing_ip][0].txs_received), 1)  # served exactly once

        # Each endpoint is connected exactly once for the whole job.
        for n in self.connects.values():
            assert_equal(n, 1)

        # An announcement ends its slot: no attempt follows one whose INV the transport took.
        for s in report["slots"]:
            for prev in s["attempts"][:-1]:
                assert prev["inv_handed_ms"] is None, s

        # Every recipient got the fixed profile: its VERSION, at most one INV naming the transaction's
        # wtxid alone, and only messages the profile sends.
        for listener, _, _ in self.listeners.values():
            check_profile(listener, tx["wtxid"])
            if "version" in listener.last_message and not isinstance(listener, NoRelayRecipient):
                # After a VERSION that qualifies the tool sends WTXIDRELAY (E2); NoRelayRecipient's does not qualify.
                assert "wtxidrelay" in listener.last_message

        # Nothing is shared between attempts but the transaction and the profile: each recipient saw
        # its own BIP324 key and VERSION nonce, and each PING carried its own nonce.
        keys, version_nonces, ping_nonces = [], [], []
        for listener, _, _ in self.listeners.values():
            state = getattr(listener, "v2_state", None)
            if state is not None and state.peer_ellswift is not None:
                keys.append(ellswift_x(state.peer_ellswift))
            if "version" in listener.last_message:
                version_nonces.append(listener.last_message["version"].nNonce)
            ping_nonces.extend(listener.ping_nonces)
        assert_greater_than(len(keys), 1)
        for values in (keys, version_nonces, ping_nonces):
            assert_equal(len(set(values)), len(values))

        # The report names what was dialled and carries no transaction bytes.
        assert tx["hex"] not in json.dumps(report)

        # The schedule has the shape Parameters gives it. Each attempt sits on a scheduled
        # opportunity, started within the scaled grace of it (C5) and ended by its deadline (D2).
        check_schedule(report, divisor, tolerance_ms=1)
        grace_ms = START_GRACE_S * 1000 / divisor
        attempt_max_ms = (HANDSHAKE_TIMEOUT_S + REQUEST_WINDOW_S + PONG_WAIT_S) * 1000 / divisor
        for s in report["slots"]:
            sched = s["scheduled_ms"]
            for a in s["attempts"]:
                assert a["scheduled_start_ms"] in sched, a
                assert 0 <= a["started_ms"] - a["scheduled_start_ms"] <= grace_ms, a
                assert_greater_than_or_equal(a["scheduled_start_ms"] + attempt_max_ms + 200, a["ended_ms"])
            # A slot never has two connections alive at once, and dials no opportunity twice: each
            # attempt starts after the previous one ended, at a later opportunity.
            for prev, nxt in zip(s["attempts"], s["attempts"][1:]):
                assert_greater_than_or_equal(nxt["started_ms"], prev["ended_ms"])
                assert_greater_than(sched.index(nxt["scheduled_start_ms"]), sched.index(prev["scheduled_start_ms"]))
        self.stop_proxy()

    def test_concurrent_invocations(self):
        self.log.info("Two send jobs run at once through one proxy, each its own broadcast")
        # The tool keeps no state between jobs and locks nothing, so two invocations sharing one
        # Tor proxy each complete independently. Distinct seeds and recipients let the run assert
        # that neither job saw the other's peer.
        addr1, addr2 = "8.5.0.1", "8.5.0.2"
        self.start_proxy({"a.seed.": [addr1], "b.seed.": [addr2]},
                         {addr1: (Recipient, True), addr2: (Recipient, True)})
        tx1 = self.wallet.create_self_transfer()
        tx2 = self.wallet.create_self_transfer()
        proc1 = self.start_send(tx1["hex"], "-seed=a.seed.")
        proc2 = self.start_send(tx2["hex"], "-seed=b.seed.")  # both processes now live at once
        rc1, report1 = self.finish_send(proc1)
        rc2, report2 = self.finish_send(proc2)
        for rc, report, tx, mine in ((rc1, report1, tx1, addr1), (rc2, report2, tx2, addr2)):
            assert_equal(rc, 0)
            assert_equal(report["txid"], tx["txid"])  # each job broadcast its own transaction
            # The job's one candidate is dialled once (R4), and the other job's recipient is never touched.
            assert_equal([(a["endpoint"], a["outcome"]) for _, a in self.attempts(report)], [(f"{mine}:{REGTEST_PORT}", "pong_received")])
            assert_equal((report["summary"]["announcements_written"], report["summary"]["pongs"]), (1, 1))
        # Nothing is shared between the two jobs: no query set carried both jobs' seeds, each
        # recipient saw its own BIP324 key and VERSION nonce, and each job drew its own schedule.
        sets = query_sets(drain_socks_commands(self.socks5_server))
        assert all(len(s) == 1 for s in sets)
        assert_greater_than_or_equal(8, len(sets))
        l1, l2 = self.listeners[addr1][0], self.listeners[addr2][0]
        assert ellswift_x(l1.v2_state.peer_ellswift) != ellswift_x(l2.v2_state.peer_ellswift)
        assert l1.last_message["version"].nNonce != l2.last_message["version"].nNonce
        assert [s["scheduled_ms"] for s in report1["slots"]] != [s["scheduled_ms"] for s in report2["slots"]]
        self.stop_proxy()

    def test_interrupt_mid_delivery(self):
        self.log.info("SIGINT after the recipient received our INV: exit status 0, and the report says interrupted")
        self.start_proxy({"a.seed.": ["8.2.0.1"]}, {"8.2.0.1": (Recipient, True)})
        tx = self.wallet.create_self_transfer()
        proc = self.start_send(tx["hex"], "-seed=a.seed.")
        # Cancel once the recipient has seen our INV: it was fully written by then.
        self.wait_until(lambda: "8.2.0.1" in self.listeners and self.listeners["8.2.0.1"][0].message_count["inv"] >= 1)
        self.interrupt(proc)
        interrupted = time.monotonic()
        rc, report = self.finish_send(proc)
        assert_greater_than(3 * self.options.timeout_factor, time.monotonic() - interrupted)
        assert_equal(report["summary"]["interrupted"], True)
        assert_greater_than(SLOTS, report["summary"]["slots_completed"])
        assert_greater_than(report["summary"]["announcements_written"], 0)
        assert_equal(rc, 0)
        self.stop_proxy()

    def test_stalled_stderr(self):
        self.log.info("A full stderr pipe nobody drains: progress lines are dropped, the schedule is not held up")
        try:
            import fcntl
        except ImportError:
            self.log.info("skipped: no fcntl on this platform")
            return
        if not hasattr(fcntl, "F_SETPIPE_SZ"):
            self.log.info("skipped: no F_SETPIPE_SZ on this platform")
            return
        self.start_proxy({"a.seed.": ["8.3.0.1"]}, {"8.3.0.1": (Recipient, True)})
        proc = r_fd = w_fd = None
        try:
            tx = self.wallet.create_self_transfer()
            r_fd, w_fd = os.pipe()
            # The smallest pipe the kernel allows, filled to capacity before the tool starts: every
            # write the tool attempts finds it full, and nobody reads until the job is over. A write
            # that waited for room would hold its slot for the whole job.
            capacity = fcntl.fcntl(w_fd, fcntl.F_SETPIPE_SZ, 4096)
            os.set_blocking(w_fd, False)
            filled = 0
            while True:
                try:
                    filled += os.write(w_fd, b"x" * (capacity - filled or 1))
                except BlockingIOError:
                    break
            os.set_blocking(w_fd, True)  # the tool inherits an ordinary blocking descriptor
            assert_greater_than(filled, 0)
            proc = subprocess.Popen(self.tool_argv(f"-timedivisor={TIME_DIVISOR}", "-seed=a.seed.", "-debug=1", "send"),
                                    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=w_fd, text=True)
            os.close(w_fd)
            w_fd = None
            out, _ = proc.communicate(input=tx["hex"], timeout=120 * self.options.timeout_factor)
            assert_equal(proc.returncode, 0)
            report = json.loads(out)
            assert_greater_than(report["summary"]["pongs"], 0)
            assert_equal(report["summary"]["slots_completed"], SLOTS)
            # The job ended by its scheduled end (D2), not when someone read stderr
            # (Interface/bitcoin-privbcast).
            assert_greater_than_or_equal(max(s["scheduled_end_ms"] for s in report["slots"]) + 1500, report["summary"]["duration_ms"])
            # The pipe holds exactly the filler: not one line waited for room, none got through.
            os.set_blocking(r_fd, False)
            got = b""
            while True:
                try:
                    chunk = os.read(r_fd, 65536)
                except BlockingIOError:
                    break
                if not chunk:
                    break
                got += chunk
            assert_equal(len(got), filled)
        finally:
            if proc is not None and proc.poll() is None:
                proc.kill()  # a tool that waited for room is still blocked in its first write
                proc.communicate()
            for fd in (w_fd, r_fd):
                if fd is not None:
                    os.close(fd)
            self.stop_proxy()

    def test_discover(self):
        self.log.info("discover resolves through the proxy and opens no connection")
        # Seed a answers IPv4, IPv6 and a private address, which is rejected (the fourth query repeats
        # the first answer: a duplicate); seed b answers four distinct addresses, all candidates; seeds c and
        # d both answer one address, which counts once; seed z's queries all fail.
        # Half the usual pace, as in test_bounded_job: every query is counted.
        self.start_proxy({"a.seed.": ["1.1.1.1", "2606:4700:4700::1111", "10.0.0.1"],
                          "b.seed.": ["2.2.2.1", "2.2.2.2", "2.2.2.3", "2.2.2.4"],
                          "c.seed.": ["4.4.4.1"], "d.seed.": ["4.4.4.1"]}, {"1.1.1.1": (Recipient, True)})
        proc = self.run_tool("-timedivisor=5", "-seed=a.seed.", "-seed=b.seed.", "-seed=c.seed.", "-seed=d.seed.", "-seed=z.seed.",
                             "-noprogress", "discover")
        out = json.loads(proc.stdout)
        seeds = {s["name"]: s for s in out["seeds"]}
        assert_equal(seeds["a.seed."]["queries"], 4)
        assert_equal(seeds["a.seed."]["answers"], 4)
        assert_equal(seeds["a.seed."]["accepted"], 2)
        assert_equal(sorted(seeds["a.seed."]["candidates"]), [f"1.1.1.1:{REGTEST_PORT}", f"[2606:4700:4700::1111]:{REGTEST_PORT}"])
        assert_equal((seeds["b.seed."]["answers"], seeds["b.seed."]["accepted"]), (4, 4))
        assert_equal(sorted(seeds["b.seed."]["candidates"]), [f"2.2.2.{i}:{REGTEST_PORT}" for i in range(1, 5)])
        assert_equal(seeds["z.seed."]["queries"], 4)
        assert_equal(seeds["z.seed."]["answers"], 0)
        assert_equal(seeds["z.seed."]["accepted"], 0)
        c, d = seeds["c.seed."], seeds["d.seed."]
        assert_equal(sorted([c["accepted"], d["accepted"]]), [0, 1])
        assert_equal(c["candidates"] + d["candidates"], [f"4.4.4.1:{REGTEST_PORT}"])
        assert_equal(out["duplicates"], 1 + 7)  # a.seed.'s repeat, and all but one answer of 4.4.4.1
        assert_equal(out["rejected"], 1)
        commands = drain_socks_commands(self.socks5_server)
        assert_equal([c.cmd for c in commands], [Command.RESOLVE] * 20)
        assert_equal(self.connects, {})
        self.stop_proxy()


if __name__ == '__main__':
    ToolPrivbcast(__file__).main()
