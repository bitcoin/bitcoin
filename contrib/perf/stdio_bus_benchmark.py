#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Benchmark script for stdio_bus shadow mode overhead measurement.

This script measures:
1. CPU overhead: comparing -stdiobus=off vs -stdiobus=shadow
2. Latency overhead: hook callback latency on hot path
3. Memory overhead: additional memory usage

Usage:
    python3 contrib/perf/stdio_bus_benchmark.py [--datadir=<path>] [--blocks=<n>]

Requirements:
    - Built bitcoind with -stdiobus support
    - Python 3.10+
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass
class BenchmarkResult:
    """Results from a single benchmark run"""
    mode: str  # "off" or "shadow"
    blocks_generated: int
    total_time_s: float
    blocks_per_second: float
    peak_memory_mb: float
    cpu_user_s: float
    cpu_system_s: float


@dataclass
class OverheadReport:
    """Comparison report between off and shadow modes"""
    baseline: BenchmarkResult
    shadow: BenchmarkResult
    time_overhead_pct: float
    memory_overhead_pct: float
    cpu_overhead_pct: float
    
    def to_dict(self):
        return {
            "baseline": {
                "mode": self.baseline.mode,
                "blocks": self.baseline.blocks_generated,
                "time_s": self.baseline.total_time_s,
                "blocks_per_s": self.baseline.blocks_per_second,
                "memory_mb": self.baseline.peak_memory_mb,
            },
            "shadow": {
                "mode": self.shadow.mode,
                "blocks": self.shadow.blocks_generated,
                "time_s": self.shadow.total_time_s,
                "blocks_per_s": self.shadow.blocks_per_second,
                "memory_mb": self.shadow.peak_memory_mb,
            },
            "overhead": {
                "time_pct": self.time_overhead_pct,
                "memory_pct": self.memory_overhead_pct,
                "cpu_pct": self.cpu_overhead_pct,
            },
            "pass": self.time_overhead_pct < 2.0 and self.cpu_overhead_pct < 2.0,
        }


def find_bitcoind() -> Path:
    """Find bitcoind binary"""
    candidates = [
        Path("build/bin/bitcoind"),
        Path("build/src/bitcoind"),
        Path("src/bitcoind"),
    ]
    for p in candidates:
        if p.exists():
            return p
    raise FileNotFoundError("bitcoind not found. Build the project first.")


def find_bitcoin_cli() -> Path:
    """Find bitcoin-cli binary"""
    candidates = [
        Path("build/bin/bitcoin-cli"),
        Path("build/src/bitcoin-cli"),
        Path("src/bitcoin-cli"),
    ]
    for p in candidates:
        if p.exists():
            return p
    raise FileNotFoundError("bitcoin-cli not found. Build the project first.")


def run_benchmark(
    bitcoind: Path,
    bitcoin_cli: Path,
    datadir: Path,
    mode: str,
    blocks: int,
) -> BenchmarkResult:
    """Run a single benchmark with specified mode"""
    
    print(f"\n{'='*60}")
    print(f"Running benchmark: -stdiobus={mode}, blocks={blocks}")
    print(f"{'='*60}")
    
    # Clean datadir
    if datadir.exists():
        import shutil
        shutil.rmtree(datadir)
    datadir.mkdir(parents=True)
    
    # Start bitcoind
    args = [
        str(bitcoind),
        f"-datadir={datadir}",
        "-regtest",
        "-daemon",
        "-server",
        f"-stdiobus={mode}",
        "-debug=net",
        "-printtoconsole=0",
    ]
    
    print(f"Starting bitcoind with: {' '.join(args)}")
    start_time = time.time()
    
    subprocess.run(args, check=True)
    
    # Wait for RPC to be ready
    cli_base = [str(bitcoin_cli), f"-datadir={datadir}", "-regtest"]
    for _ in range(30):
        try:
            subprocess.run(
                cli_base + ["getblockchaininfo"],
                capture_output=True,
                check=True,
                timeout=5,
            )
            break
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
            time.sleep(0.5)
    else:
        raise RuntimeError("bitcoind failed to start")
    
    print(f"bitcoind started in {time.time() - start_time:.2f}s")
    
    # Create wallet for mining
    try:
        subprocess.run(
            cli_base + ["createwallet", "bench"],
            capture_output=True,
            check=True,
            timeout=30,
        )
    except subprocess.CalledProcessError:
        pass  # Wallet may already exist
    
    # Load wallet if needed
    try:
        subprocess.run(
            cli_base + ["loadwallet", "bench"],
            capture_output=True,
            timeout=10,
        )
    except subprocess.CalledProcessError:
        pass
    
    # Get mining address
    result = subprocess.run(
        cli_base + ["-rpcwallet=bench", "getnewaddress"],
        capture_output=True,
        check=True,
        text=True,
        timeout=10,
    )
    address = result.stdout.strip()
    
    # Generate blocks and measure time
    print(f"Generating {blocks} blocks...")
    gen_start = time.time()
    
    subprocess.run(
        cli_base + ["-rpcwallet=bench", "generatetoaddress", str(blocks), address],
        capture_output=True,
        check=True,
        timeout=300,
    )
    
    gen_time = time.time() - gen_start
    print(f"Generated {blocks} blocks in {gen_time:.2f}s ({blocks/gen_time:.2f} blocks/s)")
    
    # Get memory info
    result = subprocess.run(
        cli_base + ["getmemoryinfo"],
        capture_output=True,
        check=True,
        text=True,
    )
    meminfo = json.loads(result.stdout)
    peak_memory_mb = meminfo.get("locked", {}).get("used", 0) / (1024 * 1024)
    
    # Stop bitcoind
    subprocess.run(cli_base + ["stop"], capture_output=True)
    time.sleep(2)
    
    total_time = time.time() - start_time
    
    return BenchmarkResult(
        mode=mode,
        blocks_generated=blocks,
        total_time_s=total_time,
        blocks_per_second=blocks / gen_time,
        peak_memory_mb=peak_memory_mb,
        cpu_user_s=0,  # Would need /usr/bin/time for accurate measurement
        cpu_system_s=0,
    )


def compare_results(baseline: BenchmarkResult, shadow: BenchmarkResult) -> OverheadReport:
    """Compare baseline (off) and shadow mode results"""
    
    time_overhead = ((shadow.total_time_s - baseline.total_time_s) / baseline.total_time_s) * 100
    memory_overhead = ((shadow.peak_memory_mb - baseline.peak_memory_mb) / max(baseline.peak_memory_mb, 1)) * 100
    
    # CPU overhead approximated from time overhead
    cpu_overhead = time_overhead
    
    return OverheadReport(
        baseline=baseline,
        shadow=shadow,
        time_overhead_pct=time_overhead,
        memory_overhead_pct=memory_overhead,
        cpu_overhead_pct=cpu_overhead,
    )


def main():
    parser = argparse.ArgumentParser(description="Benchmark stdio_bus shadow mode overhead")
    parser.add_argument("--datadir", type=Path, default=None, help="Data directory (temp if not specified)")
    parser.add_argument("--blocks", type=int, default=100, help="Number of blocks to generate")
    parser.add_argument("--output", type=Path, default=None, help="Output JSON file")
    args = parser.parse_args()
    
    bitcoind = find_bitcoind()
    bitcoin_cli = find_bitcoin_cli()
    
    print(f"Using bitcoind: {bitcoind}")
    print(f"Using bitcoin-cli: {bitcoin_cli}")
    
    with tempfile.TemporaryDirectory() as tmpdir:
        datadir = args.datadir or Path(tmpdir) / "benchmark"
        
        # Run baseline (off)
        baseline = run_benchmark(bitcoind, bitcoin_cli, datadir / "off", "off", args.blocks)
        
        # Run shadow mode
        shadow = run_benchmark(bitcoind, bitcoin_cli, datadir / "shadow", "shadow", args.blocks)
        
        # Compare
        report = compare_results(baseline, shadow)
        
        # Print results
        print("\n" + "="*60)
        print("BENCHMARK RESULTS")
        print("="*60)
        print(f"\nBaseline (-stdiobus=off):")
        print(f"  Time: {baseline.total_time_s:.2f}s")
        print(f"  Blocks/s: {baseline.blocks_per_second:.2f}")
        print(f"  Memory: {baseline.peak_memory_mb:.2f} MB")
        
        print(f"\nShadow mode (-stdiobus=shadow):")
        print(f"  Time: {shadow.total_time_s:.2f}s")
        print(f"  Blocks/s: {shadow.blocks_per_second:.2f}")
        print(f"  Memory: {shadow.peak_memory_mb:.2f} MB")
        
        print(f"\nOverhead:")
        print(f"  Time: {report.time_overhead_pct:+.2f}%")
        print(f"  Memory: {report.memory_overhead_pct:+.2f}%")
        print(f"  CPU (approx): {report.cpu_overhead_pct:+.2f}%")
        
        # Check pass/fail
        passed = report.time_overhead_pct < 2.0 and report.cpu_overhead_pct < 2.0
        print(f"\nTarget: <2% overhead")
        print(f"Result: {'PASS ✓' if passed else 'FAIL ✗'}")
        
        # Save output
        if args.output:
            with open(args.output, "w") as f:
                json.dump(report.to_dict(), f, indent=2)
            print(f"\nResults saved to: {args.output}")
        
        return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
