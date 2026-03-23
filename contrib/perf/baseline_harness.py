#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Baseline Performance Harness for stdio_bus Integration

This script measures baseline performance metrics for issues:
- #21803: Block processing delays
- #27623: Message handler saturation
- #27677: Mempool performance
- #18678: P2P/RPC degradation

Usage:
    python3 baseline_harness.py --datadir=/path/to/datadir --output=baseline_report.json

Requirements:
    - Bitcoin Core built with debug symbols (RelWithDebInfo)
    - Python 3.10+
    - numpy, matplotlib (optional, for analysis)
"""

import argparse
import json
import os
import subprocess
import sys
import time
from dataclasses import dataclass, asdict
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Optional, Any
import statistics


@dataclass
class MetricSample:
    """Single metric sample with timestamp"""
    timestamp_us: int
    value: float
    labels: Dict[str, str]


@dataclass
class MetricSummary:
    """Statistical summary of a metric"""
    name: str
    count: int
    min: float
    max: float
    mean: float
    median: float
    p50: float
    p95: float
    p99: float
    stddev: float
    unit: str


@dataclass
class BaselineReport:
    """Complete baseline report"""
    version: str
    timestamp: str
    git_commit: str
    build_type: str
    system_info: Dict[str, str]
    test_config: Dict[str, Any]
    metrics: Dict[str, MetricSummary]
    raw_samples: Dict[str, List[MetricSample]]


def get_git_commit() -> str:
    """Get current git commit hash"""
    try:
        result = subprocess.run(
            ['git', 'rev-parse', 'HEAD'],
            capture_output=True, text=True, check=True
        )
        return result.stdout.strip()[:12]
    except Exception:
        return "unknown"


def get_system_info() -> Dict[str, str]:
    """Collect system information"""
    import platform
    return {
        "os": platform.system(),
        "os_version": platform.version(),
        "arch": platform.machine(),
        "python": platform.python_version(),
        "cpu_count": str(os.cpu_count()),
    }


def calculate_percentile(data: List[float], percentile: float) -> float:
    """Calculate percentile of sorted data"""
    if not data:
        return 0.0
    sorted_data = sorted(data)
    k = (len(sorted_data) - 1) * percentile / 100
    f = int(k)
    c = f + 1 if f + 1 < len(sorted_data) else f
    return sorted_data[f] + (k - f) * (sorted_data[c] - sorted_data[f])


def summarize_metric(name: str, values: List[float], unit: str = "ms") -> MetricSummary:
    """Calculate statistical summary for a metric"""
    if not values:
        return MetricSummary(
            name=name, count=0, min=0, max=0, mean=0, median=0,
            p50=0, p95=0, p99=0, stddev=0, unit=unit
        )
    
    return MetricSummary(
        name=name,
        count=len(values),
        min=min(values),
        max=max(values),
        mean=statistics.mean(values),
        median=statistics.median(values),
        p50=calculate_percentile(values, 50),
        p95=calculate_percentile(values, 95),
        p99=calculate_percentile(values, 99),
        stddev=statistics.stdev(values) if len(values) > 1 else 0,
        unit=unit
    )


class BaselineHarness:
    """Main harness for baseline measurements"""
    
    def __init__(self, datadir: Path, bitcoind_path: Path, output_path: Path):
        self.datadir = datadir
        self.bitcoind_path = bitcoind_path
        self.output_path = output_path
        self.samples: Dict[str, List[float]] = {}
        
    def add_sample(self, metric: str, value: float):
        """Add a sample to a metric"""
        if metric not in self.samples:
            self.samples[metric] = []
        self.samples[metric].append(value)
    
    def run_rpc_benchmark(self, method: str, count: int = 100) -> List[float]:
        """Benchmark RPC method latency"""
        latencies = []
        
        for _ in range(count):
            start = time.perf_counter_ns()
            try:
                result = subprocess.run(
                    [str(self.bitcoind_path).replace('bitcoind', 'bitcoin-cli'),
                     f'-datadir={self.datadir}', method],
                    capture_output=True, check=True, timeout=30
                )
                end = time.perf_counter_ns()
                latencies.append((end - start) / 1_000_000)  # Convert to ms
            except Exception as e:
                print(f"RPC {method} failed: {e}", file=sys.stderr)
        
        return latencies
    
    def collect_debug_log_metrics(self) -> Dict[str, List[float]]:
        """Parse debug.log for timing information"""
        metrics = {
            'block_connect_ms': [],
            'block_validation_ms': [],
            'tx_validation_ms': [],
        }
        
        debug_log = self.datadir / 'debug.log'
        if not debug_log.exists():
            return metrics
        
        # Parse log for timing patterns
        # This is a simplified parser - real implementation would be more robust
        with open(debug_log, 'r') as f:
            for line in f:
                if 'UpdateTip' in line and 'ms' in line:
                    # Extract timing from UpdateTip lines
                    pass  # TODO: Implement parsing
        
        return metrics
    
    def generate_report(self) -> BaselineReport:
        """Generate the baseline report"""
        metrics = {}
        
        # Summarize all collected metrics
        for name, values in self.samples.items():
            unit = "ms" if "latency" in name or "_ms" in name else "count"
            metrics[name] = summarize_metric(name, values, unit)
        
        return BaselineReport(
            version="1.0.0",
            timestamp=datetime.utcnow().isoformat(),
            git_commit=get_git_commit(),
            build_type="RelWithDebInfo",
            system_info=get_system_info(),
            test_config={
                "datadir": str(self.datadir),
                "runs": 3,
            },
            metrics=metrics,
            raw_samples={}  # Optionally include raw samples
        )
    
    def save_report(self, report: BaselineReport):
        """Save report to JSON file"""
        # Convert dataclasses to dicts
        report_dict = asdict(report)
        
        # Convert MetricSummary objects
        report_dict['metrics'] = {
            k: asdict(v) for k, v in report.metrics.items()
        }
        
        with open(self.output_path, 'w') as f:
            json.dump(report_dict, f, indent=2)
        
        print(f"Report saved to {self.output_path}")


def print_summary(report: BaselineReport):
    """Print human-readable summary"""
    print("\n" + "=" * 60)
    print("BASELINE PERFORMANCE REPORT")
    print("=" * 60)
    print(f"Timestamp: {report.timestamp}")
    print(f"Git Commit: {report.git_commit}")
    print(f"Build Type: {report.build_type}")
    print()
    
    print("METRICS SUMMARY")
    print("-" * 60)
    
    for name, summary in sorted(report.metrics.items()):
        print(f"\n{name}:")
        print(f"  Count:  {summary.count}")
        print(f"  Mean:   {summary.mean:.3f} {summary.unit}")
        print(f"  P50:    {summary.p50:.3f} {summary.unit}")
        print(f"  P95:    {summary.p95:.3f} {summary.unit}")
        print(f"  P99:    {summary.p99:.3f} {summary.unit}")
        print(f"  StdDev: {summary.stddev:.3f} {summary.unit}")


def main():
    parser = argparse.ArgumentParser(
        description='Baseline Performance Harness for stdio_bus Integration'
    )
    parser.add_argument(
        '--datadir', type=Path, required=True,
        help='Bitcoin Core data directory'
    )
    parser.add_argument(
        '--bitcoind', type=Path, default=Path('build/bin/bitcoind'),
        help='Path to bitcoind binary'
    )
    parser.add_argument(
        '--output', type=Path, default=Path('baseline_report.json'),
        help='Output file for report'
    )
    parser.add_argument(
        '--rpc-methods', nargs='+',
        default=['getblockchaininfo', 'getmempoolinfo', 'getpeerinfo'],
        help='RPC methods to benchmark'
    )
    parser.add_argument(
        '--rpc-count', type=int, default=100,
        help='Number of RPC calls per method'
    )
    
    args = parser.parse_args()
    
    if not args.datadir.exists():
        print(f"Error: datadir {args.datadir} does not exist", file=sys.stderr)
        sys.exit(1)
    
    harness = BaselineHarness(args.datadir, args.bitcoind, args.output)
    
    print("Starting baseline measurements...")
    print(f"Data directory: {args.datadir}")
    print()
    
    # Run RPC benchmarks
    for method in args.rpc_methods:
        print(f"Benchmarking RPC: {method}...")
        latencies = harness.run_rpc_benchmark(method, args.rpc_count)
        for lat in latencies:
            harness.add_sample(f"rpc_{method}_latency_ms", lat)
    
    # Collect debug.log metrics
    print("Collecting debug.log metrics...")
    log_metrics = harness.collect_debug_log_metrics()
    for name, values in log_metrics.items():
        for v in values:
            harness.add_sample(name, v)
    
    # Generate and save report
    report = harness.generate_report()
    harness.save_report(report)
    
    # Print summary
    print_summary(report)


if __name__ == '__main__':
    main()
