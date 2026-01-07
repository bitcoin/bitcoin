set shell := ["bash", "-uc"]

default:
    just --list

# ============================================================================
# Local benchmarking commands
# ============================================================================

# Test instrumented run using signet (includes report generation)
[group('local')]
test-instrumented commit datadir:
    nix develop --command python3 bench.py build --skip-existing {{ commit }}:pr
    nix develop --command python3 bench.py --profile quick run \
        --benchmark-config bench/configs/pr.toml \
        --matrix-entry 450-true \
        --datadir {{ datadir }} \
        pr:./binaries/pr/bitcoind
    nix develop --command python3 bench.py report bench-output/ bench-output/

# Test uninstrumented run using signet
[group('local')]
test-uninstrumented commit datadir:
    nix develop --command python3 bench.py build --skip-existing {{ commit }}:pr
    nix develop --command python3 bench.py --profile quick run \
        --benchmark-config bench/configs/pr.toml \
        --matrix-entry 450-false \
        --datadir {{ datadir }} \
        pr:./binaries/pr/bitcoind

# Full benchmark with instrumentation (flamegraphs + plots)
[group('local')]
instrumented commit datadir:
    python3 bench.py build {{ commit }}:pr
    python3 bench.py run \
        --benchmark-config bench/configs/pr.toml \
        --matrix-entry 450-true \
        --datadir {{ datadir }} \
        pr:./binaries/pr/bitcoind

# Just build a binary (useful for incremental testing)
[group('local')]
build commit:
    python3 bench.py build {{ commit }}

# Run benchmark with pre-built binary
[group('local')]
run datadir binary:
    python3 bench.py run \
        --benchmark-config bench/configs/pr.toml \
        --matrix-entry 450-false \
        --datadir {{ datadir }} \
        {{ binary }}

# Generate plots from a debug.log file
[group('local')]
analyze commit logfile output_dir="./plots":
    python3 bench.py analyze {{ commit }} {{ logfile }} --output-dir {{ output_dir }}

# Generate HTML report from benchmark results
[group('local')]
report input_dir output_dir nightly_history="":
    #!/usr/bin/env bash
    set -euo pipefail
    if [ -n "{{ nightly_history }}" ]; then
        python3 bench.py report {{ input_dir }} {{ output_dir }} --nightly-history {{ nightly_history }}
    else
        python3 bench.py report {{ input_dir }} {{ output_dir }}
    fi

# ============================================================================
# CI commands (called by GitHub Actions)
# ============================================================================

# Build binary for CI
[group('ci')]
ci-build commit binaries_dir:
    python3 bench.py build -o {{ binaries_dir }} {{ commit }}:pr

# Run benchmark for CI
[group('ci')]
ci-run benchmark_config matrix_entry datadir tmp_datadir output_dir binaries_dir:
    python3 bench.py run \
        --benchmark-config {{ benchmark_config }} \
        --matrix-entry {{ matrix_entry }} \
        --datadir {{ datadir }} \
        --tmp-datadir {{ tmp_datadir }} \
        --output-dir {{ output_dir }} \
        pr:{{ binaries_dir }}/pr/bitcoind

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
