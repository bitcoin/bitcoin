#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-privbcast against misbehaving proxies: one that stalls every CONNECT, one that
offers no authentication, one that is not Tor, a slow resolver, and interruption while blocked
in RESOLVE.

The harness and the shared scripted recipients are in test_framework/privbcast.py.
"""


import json
import time

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
    DISCOVERY_WINDOW_S,
    START_GRACE_S,
    HANDSHAKE_TIMEOUT_S,
    drain_socks_commands,
    Recipient,
    PrivbcastToolTest,
)


class ToolPrivbcastProxy(PrivbcastToolTest):
    def set_test_params(self):
        super().set_test_params()

    def run_test(self):
        self.setup_wallet()
        self.test_stalled_proxy()
        self.test_socks_auth_required()
        self.test_not_tor_proxy()
        self.test_slow_resolve()
        self.test_interrupt_blocked_resolve()

    def test_stalled_proxy(self):
        self.log.info("A proxy that stalls every CONNECT: attempts fail within the handshake budget, later opportunities start on time")
        # Half the usual pace, since nine proxy handlers stall at once. The stall ends inside the scaled
        # handshake budget (45 s / 5 = 9 s) with a failure, and a slot's next opportunity is at least 10 s
        # later, so a backup still starts on time.
        # Three seeds of three: nine candidates make six primaries and three backups.
        divisor = 5
        script = {"s.seed.": ["8.1.0.1", "8.1.0.2", "8.1.0.3"], "t.seed.": ["8.1.1.1", "8.1.1.2", "8.1.1.3"],
                  "u.seed.": ["8.1.2.1", "8.1.2.2", "8.1.2.3"]}
        self.start_proxy(script, {}, connect_delay=8.0)
        tx = self.wallet.create_self_transfer()
        report, _ = self.run_send(tx["hex"], "-seed=s.seed.", "-seed=t.seed.", "-seed=u.seed.", expected_rc=2, time_divisor=divisor)
        assert_equal(report["summary"]["interrupted"], False)
        assert_equal(report["summary"]["slots_completed"], SLOTS)
        # Every attempt went to a seed's answer.
        assert {a["endpoint"] for _, a in self.attempts(report)} <= {f"{ip}:{REGTEST_PORT}" for ips in script.values() for ip in ips}
        # Six primaries and three backups, all stalled and all on time; the rest is empty.
        assert_equal(report["summary"]["connections"], 9)
        handshake_ms = HANDSHAKE_TIMEOUT_S * 1000 / divisor
        grace_ms = START_GRACE_S * 1000 / divisor
        for s in report["slots"]:
            assert_equal(s["missed_opportunities"], 0)
            for a in s["attempts"]:
                assert_equal(a["outcome"], "not_announced")
                assert 0 <= a["started_ms"] - a["scheduled_start_ms"] <= grace_ms, a
                # Failed within the handshake budget, so the slot's next opportunity is not held up.
                assert_greater_than_or_equal(handshake_ms + 200, a["ended_ms"] - a["scheduled_start_ms"])
        # Scarce candidates go to first attempts: every slot dialled its first opportunity, and the three
        # left over went to second ones, of slots 0 to 2, which draw first in each layer.
        for s in report["slots"]:
            assert_equal(s["attempts"][0]["scheduled_start_ms"], s["scheduled_ms"][0])
            for a in s["attempts"][1:]:
                assert_equal(a["scheduled_start_ms"], s["scheduled_ms"][1])
                assert_greater_than(3, s["slot"])
        # The job ends by its scheduled end: nothing stretched.
        assert_greater_than_or_equal(max(s["scheduled_end_ms"] for s in report["slots"]) + 1500, report["summary"]["duration_ms"])
        self.stop_proxy()

    def test_socks_auth_required(self):
        self.log.info("A proxy offering no authentication: the tool resolves nothing and sends nothing")
        self.start_proxy({"a.seed.": ["9.0.0.1"]}, {}, proxy_authenticates=False)
        # The tool offers only username and password, which this proxy lacks, so the proxy answers
        # "no acceptable method": no RESOLVE, so no candidate and no CONNECT.
        tx = self.wallet.create_self_transfer()
        # Nothing here depends on time, so a faster clock.
        report, _ = self.run_send(tx["hex"], "-seed=a.seed.", expected_rc=2, time_divisor=50)
        assert_equal(report["discovery"]["exit_path_candidates"], 0)
        assert_equal(report["summary"]["connections"], 0)
        assert_equal(drain_socks_commands(self.socks5_server), [])
        self.stop_proxy()

    def test_not_tor_proxy(self):
        self.log.info("An authenticating proxy that is not Tor: no candidates, nothing dialled")
        self.start_proxy({"a.seed.": ["9.0.1.1"]}, {}, tor=False)
        # Every RESOLVE is refused as an unsupported command, so there is no candidate to CONNECT to.
        tx = self.wallet.create_self_transfer()
        report, _ = self.run_send(tx["hex"], "-seed=a.seed.", expected_rc=2, time_divisor=50)
        assert_equal(report["discovery"]["exit_path_candidates"], 0)
        assert_equal(report["summary"]["connections"], 0)
        commands = drain_socks_commands(self.socks5_server)
        assert_equal([c.cmd for c in commands], [Command.RESOLVE] * 4)
        assert_equal(self.connects, {})
        self.stop_proxy()

    def test_slow_resolve(self):
        self.log.info("A resolver that answers after the discovery window: no candidates, discovery still ends on time")
        window_s = DISCOVERY_WINDOW_S / TIME_DIVISOR
        self.start_proxy({"a.seed.": ["9.1.0.1"]}, {}, resolve_delay=window_s * 2)
        started = time.monotonic()
        proc = self.run_tool(f"-timedivisor={TIME_DIVISOR}", "-seed=a.seed.", "-noprogress", "discover")
        elapsed = time.monotonic() - started
        out = json.loads(proc.stdout)
        assert_equal(out["seeds"][0]["queries"], 4)  # all four started at once
        assert_equal(out["seeds"][0]["accepted"], 0)  # every answer arrived after the window
        # discover returns at the window, not after the resolver's much longer stall.
        assert_greater_than(window_s * 2, elapsed)
        assert_equal(self.connects, {})
        self.stop_proxy()

        self.log.info("With one seed's resolver that slow, a send job still starts delivery at the window")
        self.start_proxy({"a.seed.": ["9.1.0.1"], "b.seed.": ["9.1.0.2"]}, {"9.1.0.2": (Recipient, True)},
                         resolve_delay={"a.seed.": window_s * 2})
        tx = self.wallet.create_self_transfer()
        proc = self.start_send(tx["hex"], "-seed=a.seed.", "-seed=b.seed.")
        self.wait_until(lambda: "9.1.0.2" in self.listeners and self.listeners["9.1.0.2"][0].txs_received)
        self.interrupt(proc)
        _, report = self.finish_send(proc)
        assert_equal(report["discovery"]["exit_path_candidates"], 1)
        prompt = report["slots"][0]
        assert_equal(prompt["missed_opportunities"], 0)
        started_ms = prompt["attempts"][0]["started_ms"]
        assert 0 <= started_ms - DISCOVERY_WINDOW_S * 1000 / TIME_DIVISOR <= START_GRACE_S * 1000 / TIME_DIVISOR, started_ms
        self.stop_proxy()

    def test_interrupt_blocked_resolve(self):
        self.log.info("SIGINT while workers are blocked in RESOLVE ends the job promptly")
        self.start_proxy({"a.seed.": ["9.2.0.1"]}, {}, resolve_delay=10)
        # Unscaled, so that the exchange's own timeouts are seconds away: only abandoning it ends the
        # job this soon.
        tx = self.wallet.create_self_transfer()
        proc = self.start_send(tx["hex"], "-seed=a.seed.", time_divisor=1)
        # Wait until the proxy holds a RESOLVE reply, so a query is blocked.
        self.wait_until(lambda: self.resolve_counts.get("a.seed.", 0) >= 1)
        self.interrupt(proc)
        interrupted = time.monotonic()
        rc, report = self.finish_send(proc)
        # The blocked exchanges were abandoned, not waited out.
        assert_greater_than(3 * self.options.timeout_factor, time.monotonic() - interrupted)
        assert_equal(rc, 2)
        assert_equal(report["summary"]["interrupted"], True)
        assert_equal(report["summary"]["connections"], 0)
        assert_equal(report["summary"]["slots_completed"], 0)
        assert_equal(self.connects, {})
        self.stop_proxy()


if __name__ == '__main__':
    ToolPrivbcastProxy(__file__).main()
