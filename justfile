set shell := ["bash", "-uc"]

default:
    just --list

# ============================================================================
# Local benchmarking commands
# ============================================================================

# Test instrumented run using signet (includes report generation)
[group('local')]
test-instrumented datadir:
    nix develop --command python3 bench.py --profile quick experiment run \
        bench/experiments/test-signet.toml \
        --subject-name head \
        --profile-name 450-instrumented \
        --datadir {{ datadir }} \
        --binaries-dir ./binaries \
        --skip-existing
    nix develop --command python3 bench.py report bench-output/ bench-output/

# Test uninstrumented run using signet
[group('local')]
test-uninstrumented datadir:
    nix develop --command python3 bench.py --profile quick experiment run \
        bench/experiments/test-signet.toml \
        --subject-name head \
        --profile-name 450-uninstrumented \
        --datadir {{ datadir }} \
        --binaries-dir ./binaries \
        --skip-existing

# Full benchmark with instrumentation (flamegraphs + plots)
[group('local')]
instrumented datadir:
    python3 bench.py experiment run \
        bench/experiments/pr.toml \
        --subject-name head \
        --profile-name 450-instrumented \
        --datadir {{ datadir }} \
        --skip-existing

# Just build a binary (useful for incremental testing)
[group('local')]
build commit:
    python3 bench.py build {{ commit }}

# Run the default PR-style experiment
[group('local')]
run datadir:
    python3 bench.py experiment run bench/experiments/pr.toml --datadir {{ datadir }}

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

# Run experiment for CI
[group('ci')]
ci-run experiment datadir tmp_dir output_dir binaries_dir:
    python3 bench.py experiment run \
        {{ experiment }} \
        --datadir {{ datadir }} \
        --tmp-dir {{ tmp_dir }} \
        --output-dir {{ output_dir }} \
        --binaries-dir {{ binaries_dir }} \
        --skip-existing

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
