# Benchcoin

A CLI for benchmarking Bitcoin Core IBD.

## Quick Start

```bash
# Quick smoke test on signet (requires nix)
nix develop --command python3 bench.py --profile quick full \
    --chain signet --datadir /path/to/signet/datadir HEAD~1 HEAD

# Or use just (wraps nix develop)
just quick HEAD~1 HEAD /path/to/signet/datadir
```

## Requirements

- **Nix** with flakes enabled (provides hyperfine, flamegraph, etc.)
- A blockchain datadir snapshot to benchmark against
- Two git commits to compare

Optional (auto-detected, gracefully degrades without):
- `/run/wrappers/bin/drop-caches` (NixOS) - clears page cache between runs

## Commands

```
bench.py [GLOBAL_OPTIONS] COMMAND [OPTIONS] ARGS

Global Options:
  --profile {quick,full,ci}   Configuration profile
  --config PATH               Custom config file
  -v, --verbose               Verbose output
  --dry-run                   Show what would run

Commands:
  build     Build bitcoind at two commits
  run       Run benchmark (requires pre-built binaries)
  analyze   Generate plots from debug.log
  report    Generate HTML report
  full      Complete pipeline: build → run → analyze
```

### build

Build bitcoind binaries at two commits for comparison:

```bash
python3 bench.py build HEAD~1 HEAD
python3 bench.py build --binaries-dir /tmp/bins abc123 def456
python3 bench.py build --skip-existing HEAD~1 HEAD  # reuse existing
```

### run

Run hyperfine benchmark comparing two pre-built binaries:

```bash
python3 bench.py run --datadir /data/snapshot HEAD~1 HEAD
python3 bench.py run --instrumented --datadir /data/snapshot HEAD~1 HEAD
```

Options:
- `--datadir PATH` - Source blockchain snapshot (required)
- `--tmp-datadir PATH` - Working directory (default: ./bench-output/tmp-datadir)
- `--stop-height N` - Block height to sync to
- `--dbcache N` - Database cache in MB
- `--runs N` - Number of iterations (default: 3, forced to 1 if instrumented)
- `--instrumented` - Enable flamegraph profiling and debug logging
- `--connect ADDR` - P2P node to sync from (empty = public network)
- `--chain {main,signet,testnet,regtest}` - Which chain
- `--no-cache-drop` - Don't clear page cache between runs

### analyze

Generate plots from a debug.log file:

```bash
python3 bench.py analyze abc123 /path/to/debug.log --output-dir ./plots
```

Generates PNG plots for:
- Block height vs time
- Cache size vs height/time
- Transaction count vs height
- LevelDB compaction events
- CoinDB write batches

### report

Generate HTML report from benchmark results:

```bash
python3 bench.py report ./bench-output ./report
```

### full

Run complete pipeline (build + run + analyze if instrumented):

```bash
python3 bench.py --profile quick full --chain signet --datadir /tmp/signet HEAD~1 HEAD
python3 bench.py --profile full full --datadir /data/mainnet HEAD~1 HEAD
```

## Profiles

Profiles set sensible defaults for common scenarios:

| Profile | stop_height | runs | dbcache | connect |
|---------|-------------|------|---------|---------|
| quick   | 1,500       | 1    | 450     | (public network) |
| full    | 855,000     | 3    | 450     | (public network) |
| ci      | 855,000     | 3    | 450     | 148.251.128.115:33333 |

Override any profile setting with CLI flags:

```bash
python3 bench.py --profile quick full --stop-height 5000 --datadir ... HEAD~1 HEAD
```

## Configuration

Configuration is layered (lowest to highest priority):

1. Built-in defaults
2. `bench.toml` (in repo root)
3. Environment variables (`BENCH_DATADIR`, `BENCH_DBCACHE`, etc.)
4. CLI arguments

### bench.toml

```toml
[defaults]
chain = "main"
dbcache = 450
stop_height = 855000
runs = 3

[paths]
binaries_dir = "./binaries"
output_dir = "./bench-output"

[profiles.quick]
stop_height = 1500
runs = 1
dbcache = 450

[profiles.ci]
connect = "148.251.128.115:33333"
```

### Environment Variables

```bash
export BENCH_DATADIR=/data/snapshot
export BENCH_DBCACHE=1000
export BENCH_STOP_HEIGHT=100000
```

## Justfile Recipes

The justfile wraps common operations with `nix develop`:

```bash
just quick HEAD~1 HEAD /path/to/datadir     # Quick signet test
just full HEAD~1 HEAD /path/to/datadir      # Full mainnet benchmark
just instrumented HEAD~1 HEAD /path/to/datadir  # With flamegraphs
just build HEAD~1 HEAD                       # Build only
just run HEAD~1 HEAD /path/to/datadir       # Run only (binaries must exist)
```

## Architecture

```
bench.py              CLI entry point (argparse)
bench/
├── config.py         Layered configuration (TOML + env + CLI)
├── capabilities.py   System capability detection
├── build.py          Build phase (nix build)
├── benchmark.py      Benchmark phase (hyperfine)
├── analyze.py        Plot generation (matplotlib)
├── report.py         HTML report generation
└── utils.py          Git operations, datadir management
```

### Capability Detection

The tool auto-detects system capabilities and gracefully degrades:

```python
from bench.capabilities import detect_capabilities
caps = detect_capabilities()
# caps.has_hyperfine, caps.can_drop_caches, etc.
```

Missing optional features emit warnings but don't fail:

```
WARNING: drop-caches not available - cache won't be cleared between runs
```

Missing required features (hyperfine, flamegraph for instrumented) cause errors.

### Hyperfine Integration

The benchmark phase generates temporary shell scripts for hyperfine hooks:

- `setup` - Clean tmp datadir (once before all runs)
- `prepare` - Copy snapshot, drop caches, clean logs (before each run)
- `cleanup` - Clean tmp datadir (after all runs per command)
- `conclude` - Collect flamegraph/logs (instrumented only, after each run)

### Instrumented Mode

When `--instrumented` is set:

1. Wraps bitcoind in `flamegraph` for CPU profiling
2. Enables debug logging: `-debug=coindb -debug=leveldb -debug=bench -debug=validation`
3. Forces `runs=1` (profiling overhead makes multiple runs pointless)
4. Generates flamegraph SVGs and performance plots

## CI Integration

GitHub Actions workflows call bench.py directly (already in nix develop):

```yaml
- run: |
    nix develop --command python3 bench.py build \
      --binaries-dir ${{ runner.temp }}/binaries \
      $BASE_SHA $HEAD_SHA
```

CI-specific paths and the dedicated sync node are configured via `--profile ci`.
