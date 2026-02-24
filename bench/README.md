# Benchcoin

A CLI for benchmarking Bitcoin Core IBD (Initial Block Download).

## Quick Start

```bash
# Quick smoke test on signet (requires nix)
nix develop --command python3 bench.py build HEAD:test
nix develop --command python3 bench.py run \
    --benchmark-config bench/configs/test-signet.toml \
    --matrix-entry 450 \
    --output-dir ./output \
    test:./binaries/test/bitcoind

# Or use just
just test-uninstrumented HEAD
```

## Requirements

- **Nix** with flakes enabled (provides hyperfine, flamegraph, etc.)

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
  build     Build bitcoind at a commit
  run       Run benchmark (requires pre-built binary + TOML config)
  analyze   Generate plots from debug.log
  report    Generate HTML report
  nightly   Manage nightly history + generate chart
```

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
    --output-dir ./output \
    pr:./binaries/pr/bitcoind
```

Options:
- `--benchmark-config PATH` - TOML config file (required)
- `--matrix-entry NAME` - Matrix entry to run (required)
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

## Benchmark Configs

Benchmarks are driven by TOML config files in `bench/configs/`:

| File | Chain | Matrix Entries | Use Case |
|------|-------|----------------|----------|
| `pr.toml` | mainnet | 450/32000 x uninstrumented/instrumented | PR comparison |
| `nightly.toml` | mainnet | 450, 32000 | Nightly baseline |
| `test-signet.toml` | signet | 450 | Quick local smoke test |

All configs use `full_ibd = true` (sync from genesis) with `runs = 1`.

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
just test-instrumented HEAD          # Signet smoke test with flamegraphs
just test-uninstrumented HEAD        # Signet smoke test without profiling
just instrumented HEAD               # Full instrumented benchmark
just build HEAD:pr                   # Build only
just run pr:./binaries/pr/bitcoind   # Run with pre-built binary
just analyze COMMIT debug.log ./plots
just report ./input ./output --nightly-history ./nightly-history.json
```

## Architecture

```
bench.py              CLI entry point (argparse)
bench/
├── config.py         Layered configuration (TOML + env + CLI)
├── benchmark_config.py  TOML config loader + matrix expansion
├── capabilities.py   System capability detection
├── build.py          Build phase (nix build)
├── benchmark.py      Benchmark phase (hyperfine)
├── analyze.py        Plot generation (matplotlib)
├── report.py         HTML report generation
├── nightly.py        Nightly history + chart generation
└── utils.py          Git operations, datadir management
```

### Hyperfine Integration

The benchmark phase generates shell scripts for hyperfine hooks:

- `setup` - Create tmp datadir (once before all runs)
- `prepare` - Create fresh datadir, drop caches, clean logs (before each run)
- `cleanup` - Clean tmp datadir (after all runs)
- `conclude` - Collect flamegraph/logs (instrumented only)

### Instrumented Mode

When `instrumentation = "instrumented"` in the matrix:

1. Wraps bitcoind in `flamegraph` for CPU profiling
2. Enables debug logging: `coindb`, `leveldb`, `bench`, `validation`
3. Forces `runs=1` (profiling overhead makes multiple runs pointless)
4. Generates flamegraph SVGs and performance plots

## CI Integration

GitHub Actions workflows call bench.py directly:

```yaml
- run: |
    nix develop --command python3 bench.py run \
      --benchmark-config bench/configs/pr.toml \
      --matrix-entry ${{ matrix.name }} \
      --tmp-datadir ${{ runner.temp }}/datadir \
      --output-dir ${{ runner.temp }}/output \
      pr:${{ runner.temp }}/binaries/pr/bitcoind
```
