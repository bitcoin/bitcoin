# Benchcoin

A CLI for benchmarking Bitcoin Core IBD (Initial Block Download).

## Quick Start

```bash
# Quick smoke test on signet (requires nix)
nix develop --command python3 bench.py build HEAD:test
nix develop --command python3 bench.py run \
    --benchmark-config bench/configs/test-signet.toml \
    --matrix-entry 450 \
    --datadir /path/to/signet-datadir \
    --output-dir ./output \
    test:./binaries/test/bitcoind

# Declarative PR-style experiment
nix develop --command python3 bench.py experiment run \
    bench/experiments/pr.toml \
    --datadir /data/pruned-840k \
    --output-dir ./output

# Or use just
just test-uninstrumented HEAD /path/to/signet-datadir
```

## Requirements

- **Nix** with flakes enabled (provides hyperfine, flamegraph, etc.)
- A blockchain datadir snapshot to benchmark against

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
  experiment Run a declarative experiment manifest
  build     Build bitcoind at a commit
  run       Run benchmark (requires pre-built binary + TOML config)
  analyze   Generate plots from debug.log
  report    Generate HTML report
  nightly   Manage nightly history + generate chart
```

### experiment

Run a complete benchmark experiment from a TOML manifest:

```bash
python3 bench.py experiment run bench/experiments/pr.toml \
    --datadir /data/pruned-840k \
    --output-dir ./output \
    --binaries-dir ./binaries
```

An experiment is the high-level interface for normal use. It defines:

- `subjects`: commits or existing binaries to benchmark
- `profiles`: runtime settings such as dbcache and instrumentation
- `comparisons`: derived outputs such as differential flamegraphs

Example with two named commits and two instrumented profiles:

```toml
[experiment]
name = "diff"
outputs = ["results", "differential-flamegraph"]

[[subjects]]
name = "before"
commit = "abc123"

[[subjects]]
name = "after"
commit = "def456"

[[profiles]]
name = "450"
dbcache = 450
instrumentation = true

[[profiles]]
name = "32000"
dbcache = 32000
instrumentation = true

[[comparisons]]
name = "before-vs-after"
before = "before"
after = "after"
outputs = ["differential-flamegraph"]
```

Defaults are intentionally useful: if a manifest omits subjects, benchcoin
benchmarks `HEAD`; if it omits profiles, benchcoin uses a `450` MB
uninstrumented profile.

Schedulers can shard a manifest without changing its benchmark definition by
using `--subject-name NAME` or `--profile-name NAME`.

### build

Build a bitcoind binary at a commit:

```bash
python3 bench.py build HEAD:pr
python3 bench.py build -o /tmp/bins abc123:test
python3 bench.py build --skip-existing HEAD:pr
```

### run

Run a benchmark using a TOML config and matrix entry:

```bash
python3 bench.py run \
    --benchmark-config bench/configs/pr.toml \
    --matrix-entry 450-uninstrumented \
    --datadir /data/pruned-840k \
    --output-dir ./output \
    pr:./binaries/pr/bitcoind
```

Options:
- `--benchmark-config PATH` - TOML config file (required)
- `--matrix-entry NAME` - Matrix entry to run (required)
- `--datadir PATH` - Blockchain datadir snapshot to copy for each run
- `--tmp-datadir PATH` - Working directory for benchmark runs
- `-o, --output-dir PATH` - Output directory for results
- `--no-cache-drop` - Skip clearing page cache between runs

### analyze

Generate plots from a debug.log file:

```bash
python3 bench.py analyze abc123 /path/to/debug.log --output-dir ./plots
```

Generates PNG plots for block processing rate, cache usage, transaction counts, LevelDB compaction, and CoinDB write batches.

### report

Generate an HTML report from benchmark results:

```bash
# Single directory
python3 bench.py report ./bench-output ./report

# Multi-network (CI mode)
python3 bench.py report \
    --network 450-uninstrumented:./results/450 \
    --network 32000-uninstrumented:./results/32000 \
    --nightly-history ./nightly-history.json \
    --pr-number 123 --run-id abc \
    ./output
```

### nightly

Manage nightly benchmark history:

```bash
# Append a result
python3 bench.py nightly --history-file history.json append \
    results.json COMMIT 450 \
    --benchmark-config bench/configs/nightly.toml \
    --machine-specs machine-specs.json

# Generate chart
python3 bench.py nightly --history-file history.json chart index.html
```

## Experiment Manifests

Experiments are driven by TOML files in `bench/experiments/`:

| File | Subjects | Profiles | Use Case |
|------|----------|----------|----------|
| `pr.toml` | `HEAD` | 450/32000 x uninstrumented/instrumented | PR-style benchmark |
| `differential-flamegraph.toml` | before/after commits | 450/32000 instrumented | Differential flamegraphs |

## Low-Level Benchmark Configs

Benchmarks are driven by TOML config files in `bench/configs/`:

| File | Chain | Matrix Entries | Use Case |
|------|-------|----------------|----------|
| `pr.toml` | mainnet | 450/32000 x uninstrumented/instrumented | PR comparison |
| `nightly.toml` | mainnet | 450, 32000 | Nightly baseline |
| `test-signet.toml` | signet | 450 | Quick local smoke test |

Configs use `start_height = 840000` (resuming from a pruned snapshot) with `runs = 2` (except signet which starts from 0 with `runs = 1`).

### Matrix Expansion

The `[bitcoind.matrix]` section defines parameter axes. Their cartesian product generates named entries:

```toml
[bitcoind.matrix]
dbcache = [450, 32000]
instrumentation = ["uninstrumented", "instrumented"]
# Produces: 450-uninstrumented, 450-instrumented, 32000-uninstrumented, 32000-instrumented
```

Select one with `--matrix-entry`.

## Justfile Recipes

```bash
just test-instrumented HEAD /path/to/datadir    # Signet smoke test with flamegraphs
just test-uninstrumented HEAD /path/to/datadir  # Signet smoke test without profiling
just instrumented HEAD /path/to/datadir         # Full instrumented benchmark
just build HEAD:pr                              # Build only
just run /path/to/datadir pr:./binaries/pr/bitcoind  # Run with pre-built binary
just analyze COMMIT debug.log ./plots
just report ./input ./output --nightly-history ./nightly-history.json
```

## Architecture

```
bench.py              CLI entry point (argparse)
bench/
├── experiment.py     Experiment manifests, planning, execution, derivations
├── config.py         Layered configuration (TOML + env + CLI)
├── benchmark_config.py  TOML config loader + matrix expansion
├── capabilities.py   System capability detection
├── build.py          Build phase (nix build)
├── benchmark.py      Benchmark phase (hyperfine)
├── flamegraph.py     Differential flamegraph derivation
├── analyze.py        Plot generation (matplotlib)
├── report.py         HTML report generation
├── nightly.py        Nightly history + chart generation
└── utils.py          Git operations, datadir management
```

### Experiment Model

The high-level flow is:

```
experiment manifest -> subjects x profiles -> run artifacts -> derivations
```

Subjects describe what to benchmark (`commit` or `binary`). Profiles describe
how to benchmark it (`dbcache`, `instrumentation`, `runs`, and optional
bitcoind flags). Comparisons describe outputs derived from multiple runs, such
as differential flamegraphs.

### Hyperfine Integration

The benchmark phase generates shell scripts for hyperfine hooks:

- `setup` - Clean tmp datadir (once before all runs)
- `prepare` - Copy snapshot, drop caches, clean logs (before each run)
- `cleanup` - Collect artifacts and clean tmp datadir (after each run)

### Instrumented Mode

When `instrumentation = "instrumented"` in the matrix:

1. Runs bitcoind under `perf record`
2. Enables debug logging: `coindb`, `leveldb`, `bench`, `validation`
3. Forces `runs=1` (profiling overhead makes multiple runs pointless)
4. Generates raw `perf.data`, folded stacks, flamegraph SVGs, and optional plots

## CI Integration

GitHub Actions workflows call bench.py directly:

```yaml
- run: |
    nix develop --command python3 bench.py experiment run \
      bench/experiments/pr.toml \
      --datadir $ORIGINAL_DATADIR \
      --tmp-dir ${{ runner.temp }}/datadirs \
      --binaries-dir ${{ runner.temp }}/binaries \
      --output-dir ${{ runner.temp }}/experiment-output
```
