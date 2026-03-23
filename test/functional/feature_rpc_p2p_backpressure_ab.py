#!/usr/bin/env python3
# Copyright (c) 2026
# Distributed under the MIT software license.

"""
A/B test for Phase 5 backpressure policy (#18678).

Run A: -experimental-rpc-priority=0
Run B: -experimental-rpc-priority=1

Workload:
- High-rate low-priority P2P tx INV flood
- Concurrent RPC calls (getblockcount/getbestblockhash/getmempoolinfo)

Outputs:
- JSON metrics files under <tmpdir>/phase5_ab_metrics/
"""

import json
import math
import os
import random
import threading
import time
from dataclasses import asdict, dataclass
from typing import List

from test_framework.messages import (
    CInv,
    MSG_TX,
    msg_inv,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    get_rpc_proxy,
)


@dataclass
class ScenarioMetrics:
    mode: str
    duration_s: int
    rpc_threads: int
    rpc_calls_total: int
    rpc_calls_ok: int
    rpc_calls_err: int
    rpc_calls_per_sec: float
    rpc_error_rate: float
    rpc_p50_ms: float
    rpc_p95_ms: float
    rpc_p99_ms: float
    p2p_inv_msgs_sent: int
    p2p_inv_entries_sent: int


def percentile(values: List[float], p: float) -> float:
    if not values:
        return 0.0
    s = sorted(values)
    k = max(0, min(len(s) - 1, math.ceil((p / 100.0) * len(s)) - 1))
    return float(s[k])


class LowPriorityInvFloodPeer(P2PInterface):
    def send_inv_batch(self, rng: random.Random, entries: int) -> None:
        invs = [CInv(MSG_TX, rng.getrandbits(256)) for _ in range(entries)]
        self.send_without_ping(msg_inv(invs))


class RpcP2PBackpressureABTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.base_args = [
            "-rpcthreads=4",
            "-rpcworkqueue=64",
            "-debug=0",
            "-listen=1",
        ]
        self.extra_args = [self.base_args + ["-experimental-rpc-priority=0"]]
        self.supports_cli = False

        # Workload tuning
        self.test_duration_s = 45
        self.rpc_threads = 12
        self.inv_entries_per_msg = 32
        self.inv_msgs_per_tick = 2
        self.tick_sleep_s = 0.02

    def skip_test_if_missing_module(self):
        pass

    def run_test(self):
        baseline_args = self.base_args + ["-experimental-rpc-priority=0"]
        policy_args = self.base_args + ["-experimental-rpc-priority=1"]

        self.log.info("Run A: baseline (-experimental-rpc-priority=0)")
        self.restart_node(0, extra_args=baseline_args)
        baseline = self.run_scenario(mode="baseline")

        self.log.info("Run B: policy (-experimental-rpc-priority=1)")
        self.restart_node(0, extra_args=policy_args)
        policy = self.run_scenario(mode="policy")

        self.log.info("Baseline metrics: %s", asdict(baseline))
        self.log.info("Policy metrics:   %s", asdict(policy))

        # Save artifacts
        out_dir = os.path.join(self.options.tmpdir, "phase5_ab_metrics")
        os.makedirs(out_dir, exist_ok=True)
        with open(os.path.join(out_dir, "baseline.json"), "w", encoding="utf-8") as f:
            json.dump(asdict(baseline), f, indent=2, sort_keys=True)
        with open(os.path.join(out_dir, "policy.json"), "w", encoding="utf-8") as f:
            json.dump(asdict(policy), f, indent=2, sort_keys=True)

        summary = {
            "p95_improvement_pct": self.improvement_pct(baseline.rpc_p95_ms, policy.rpc_p95_ms),
            "p99_improvement_pct": self.improvement_pct(baseline.rpc_p99_ms, policy.rpc_p99_ms),
            "error_rate_delta_pct_points": round(policy.rpc_error_rate - baseline.rpc_error_rate, 4),
            "out_dir": out_dir,
        }
        with open(os.path.join(out_dir, "summary.json"), "w", encoding="utf-8") as f:
            json.dump(summary, f, indent=2, sort_keys=True)

        self.log.info("Summary: %s", summary)
        self.log.info("Artifacts written to: %s", out_dir)

        # Optional strict gating
        enforce = os.getenv("PHASE5_ENFORCE", "0") == "1"
        if enforce:
            assert policy.rpc_p95_ms <= baseline.rpc_p95_ms * 0.80
            assert policy.rpc_p99_ms <= baseline.rpc_p99_ms * 0.80

    @staticmethod
    def improvement_pct(base: float, new: float) -> float:
        if base <= 0:
            return 0.0
        return round((base - new) * 100.0 / base, 2)

    def run_scenario(self, mode: str) -> ScenarioMetrics:
        node = self.nodes[0]
        assert_equal(node.getblockcount(), 0)

        peer = node.add_p2p_connection(LowPriorityInvFloodPeer())
        peer.sync_with_ping()

        stop_event = threading.Event()
        lock = threading.Lock()

        rpc_latencies_ms: List[float] = []
        rpc_ok = 0
        rpc_err = 0

        rpc_methods = ("getblockcount", "getbestblockhash", "getmempoolinfo")

        def rpc_worker(worker_id: int):
            nonlocal rpc_ok, rpc_err
            proxy = get_rpc_proxy(node.url, node.index, timeout=120, coveragedir=None)
            rng = random.Random(1000 + worker_id)

            while not stop_event.is_set():
                method = rpc_methods[rng.randrange(len(rpc_methods))]
                t0 = time.perf_counter_ns()
                ok = True
                try:
                    if method == "getblockcount":
                        proxy.getblockcount()
                    elif method == "getbestblockhash":
                        proxy.getbestblockhash()
                    else:
                        proxy.getmempoolinfo()
                except Exception:
                    ok = False
                dt_ms = (time.perf_counter_ns() - t0) / 1_000_000.0
                with lock:
                    rpc_latencies_ms.append(dt_ms)
                    if ok:
                        rpc_ok += 1
                    else:
                        rpc_err += 1

        threads = []
        for i in range(self.rpc_threads):
            t = threading.Thread(target=rpc_worker, args=(i,), daemon=True)
            t.start()
            threads.append(t)

        # Main thread runs P2P flood
        p2p_inv_msgs_sent = 0
        p2p_inv_entries_sent = 0
        p2p_errors = 0
        flood_rng = random.Random(42)

        start = time.time()
        end = start + self.test_duration_s
        while time.time() < end:
            for _ in range(self.inv_msgs_per_tick):
                try:
                    peer.send_inv_batch(flood_rng, self.inv_entries_per_msg)
                    p2p_inv_msgs_sent += 1
                    p2p_inv_entries_sent += self.inv_entries_per_msg
                except Exception as e:
                    p2p_errors += 1
                    # Don't break - continue trying to send
                    if p2p_errors == 1:
                        self.log.warning(f"P2P send error (first): {e}")
            time.sleep(self.tick_sleep_s)
        
        if p2p_inv_msgs_sent == 0:
            self.log.warning(f"No P2P INV messages sent! Errors: {p2p_errors}")

        stop_event.set()
        for t in threads:
            t.join(timeout=5)

        try:
            node.disconnect_p2ps()
        except Exception:
            pass

        total = rpc_ok + rpc_err
        duration = max(1e-9, time.time() - start)

        metrics = ScenarioMetrics(
            mode=mode,
            duration_s=self.test_duration_s,
            rpc_threads=self.rpc_threads,
            rpc_calls_total=total,
            rpc_calls_ok=rpc_ok,
            rpc_calls_err=rpc_err,
            rpc_calls_per_sec=round(rpc_ok / duration, 2),
            rpc_error_rate=round((rpc_err * 100.0 / total) if total else 0.0, 4),
            rpc_p50_ms=round(percentile(rpc_latencies_ms, 50), 3),
            rpc_p95_ms=round(percentile(rpc_latencies_ms, 95), 3),
            rpc_p99_ms=round(percentile(rpc_latencies_ms, 99), 3),
            p2p_inv_msgs_sent=p2p_inv_msgs_sent,
            p2p_inv_entries_sent=p2p_inv_entries_sent,
        )
        return metrics


if __name__ == '__main__':
    RpcP2PBackpressureABTest(__file__).main()
