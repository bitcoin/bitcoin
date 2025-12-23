set shell := ["bash", "-uc"]

default:
    just --list

# ============================================================================
# Local benchmarking commands
# ============================================================================

# Test instrumented run using signet (includes report generation)
[group('local')]
test-instrumented base head datadir:
    nix develop --command python3 bench.py build --skip-existing {{ base }}:base {{ head }}:head
    nix develop --command python3 bench.py --profile quick run \
        --chain signet \
        --instrumented \
        --datadir {{ datadir }} \
        base:./binaries/base/bitcoind \
        head:./binaries/head/bitcoind
    nix develop --command python3 bench.py report bench-output/ bench-output/

# Test uninstrumented run using signet
[group('local')]
test-uninstrumented base head datadir:
    nix develop --command python3 bench.py build --skip-existing {{ base }}:base {{ head }}:head
    nix develop --command python3 bench.py --profile quick run \
        --chain signet \
        --datadir {{ datadir }} \
        base:./binaries/base/bitcoind \
        head:./binaries/head/bitcoind

# Full benchmark with instrumentation (flamegraphs + plots)
[group('local')]
instrumented base head datadir:
    python3 bench.py build {{ base }}:base {{ head }}:head
    python3 bench.py --profile quick run \
        --instrumented \
        --datadir {{ datadir }} \
        base:./binaries/base/bitcoind \
        head:./binaries/head/bitcoind

# Just build binaries (useful for incremental testing)
[group('local')]
build *commits:
    python3 bench.py build {{ commits }}

# Run benchmark with pre-built binaries
[group('local')]
run datadir *binaries:
    python3 bench.py run --datadir {{ datadir }} {{ binaries }}

# Generate plots from a debug.log file
[group('local')]
analyze commit logfile output_dir="./plots":
    python3 bench.py analyze {{ commit }} {{ logfile }} --output-dir {{ output_dir }}

# Compare benchmark results
[group('local')]
compare *results_files:
    python3 bench.py compare {{ results_files }}

# Generate HTML report from benchmark results
[group('local')]
report input_dir output_dir:
    python3 bench.py report {{ input_dir }} {{ output_dir }}

# ============================================================================
# CI commands (called by GitHub Actions)
# ============================================================================

# Build binaries for CI
[group('ci')]
ci-build base_commit head_commit binaries_dir:
    python3 bench.py build -o {{ binaries_dir }} {{ base_commit }}:base {{ head_commit }}:head

# Run uninstrumented benchmarks for CI
[group('ci')]
ci-run datadir tmp_datadir output_dir dbcache binaries_dir:
    python3 bench.py --profile ci run \
        --datadir {{ datadir }} \
        --tmp-datadir {{ tmp_datadir }} \
        --output-dir {{ output_dir }} \
        --dbcache {{ dbcache }} \
        base:{{ binaries_dir }}/base/bitcoind \
        head:{{ binaries_dir }}/head/bitcoind

# Run instrumented benchmarks for CI
[group('ci')]
ci-run-instrumented datadir tmp_datadir output_dir dbcache binaries_dir:
    python3 bench.py --profile ci run \
        --instrumented \
        --datadir {{ datadir }} \
        --tmp-datadir {{ tmp_datadir }} \
        --output-dir {{ output_dir }} \
        --dbcache {{ dbcache }} \
        base:{{ binaries_dir }}/base/bitcoind \
        head:{{ binaries_dir }}/head/bitcoind

# ============================================================================
# Git helpers
# ============================================================================

# Cherry-pick commits from a Bitcoin Core PR onto this branch
[group('git')]
pick-pr pr_number:
    #!/usr/bin/env bash
    set -euxo pipefail

    if ! git remote get-url upstream 2>/dev/null | grep -q "bitcoin/bitcoin"; then
        echo "Error: 'upstream' remote not found or doesn't point to bitcoin/bitcoin"
        echo "Please add it with: git remote add upstream https://github.com/bitcoin/bitcoin.git"
        exit 1
    fi

    git fetch upstream pull/{{ pr_number }}/head:bench-{{ pr_number }} && git cherry-pick $(git rev-list --reverse bench-{{ pr_number }} --not upstream/master)
