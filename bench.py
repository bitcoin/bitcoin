#!/usr/bin/env python3
"""Benchcoin - Bitcoin Core benchmarking toolkit.

A CLI for building, benchmarking, analyzing, and reporting on Bitcoin Core
performance.

Usage:
    bench.py build COMMIT[:NAME]...   Build bitcoind at one or more commits
    bench.py run NAME:BINARY...       Benchmark one or more binaries
    bench.py analyze COMMIT LOGFILE   Generate plots from debug.log
    bench.py compare RESULTS...       Compare benchmark results
    bench.py report INPUT OUTPUT      Generate HTML report
    bench.py nightly append ...       Append result to nightly history
    bench.py nightly chart ...        Generate nightly chart HTML

Examples:
    # Build two commits
    bench.py build HEAD~1:before HEAD:after

    # Benchmark built binaries
    bench.py run before:./binaries/before/bitcoind after:./binaries/after/bitcoind --datadir /data

    # Compare results
    bench.py compare ./bench-output/results.json

    # Generate HTML report
    bench.py report ./bench-output ./report

    # Append nightly result and regenerate chart
    bench.py nightly append results.json abc123 default 450
    bench.py nightly chart ./index.html
"""

from __future__ import annotations

import argparse
import logging
import sys
from pathlib import Path

from bench.capabilities import detect_capabilities
from bench.config import build_config

logging.basicConfig(
    level=logging.INFO,
    format="%(levelname)s: %(message)s",
)
logger = logging.getLogger(__name__)


def cmd_build(args: argparse.Namespace) -> int:
    """Build bitcoind at one or more commits."""
    from bench.build import BuildPhase

    capabilities = detect_capabilities()
    config = build_config(
        cli_args={
            "binaries_dir": args.output_dir,
            "skip_existing": args.skip_existing,
            "dry_run": args.dry_run,
            "verbose": args.verbose,
        },
        config_file=Path(args.config) if args.config else None,
        profile=args.profile,
    )

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    phase = BuildPhase(config, capabilities)

    try:
        result = phase.run(
            args.commits,
            output_dir=Path(args.output_dir) if args.output_dir else None,
        )
        logger.info(f"Built {len(result.binaries)} binary(ies):")
        for binary in result.binaries:
            logger.info(f"  {binary.name}: {binary.path}")
        return 0
    except Exception as e:
        logger.error(f"Build failed: {e}")
        return 1


def cmd_run(args: argparse.Namespace) -> int:
    """Run benchmark on one or more binaries."""
    from bench.benchmark import BenchmarkPhase, parse_binary_spec
    from bench.benchmark_config import BenchmarkConfig

    capabilities = detect_capabilities()

    # Load benchmark config
    benchmark_config = BenchmarkConfig.from_toml(Path(args.benchmark_config))

    # Validate benchmark config
    errors = benchmark_config.validate()
    if errors:
        for error in errors:
            logger.error(f"Config error: {error}")
        return 1

    # Get matrix entry
    matrix_entry = benchmark_config.get_matrix_entry(args.matrix_entry)
    if not matrix_entry:
        available = benchmark_config.get_matrix_names()
        logger.error(
            f"Matrix entry '{args.matrix_entry}' not found. "
            f"Available: {', '.join(available)}"
        )
        return 1
    logger.info(f"Using matrix entry: {matrix_entry}")

    # Build config with CLI args and benchmark config values
    cli_args: dict = {
        "datadir": args.datadir,
        "tmp_datadir": args.tmp_datadir,
        "output_dir": args.output_dir,
        "no_cache_drop": args.no_cache_drop,
        "dry_run": args.dry_run,
        "verbose": args.verbose,
        "runs": benchmark_config.runs,
    }

    # Apply matrix entry values
    if "dbcache" in matrix_entry:
        cli_args["dbcache"] = matrix_entry["dbcache"]
    if "instrumented" in matrix_entry:
        cli_args["instrumented"] = matrix_entry["instrumented"]

    config = build_config(
        cli_args=cli_args,
        config_file=Path(args.config) if args.config else None,
        profile=args.profile,
    )

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    errors = config.validate()
    if errors:
        for error in errors:
            logger.error(error)
        return 1

    # Parse binary specs
    try:
        binaries = [parse_binary_spec(spec) for spec in args.binaries]
    except ValueError as e:
        logger.error(str(e))
        return 1

    # Validate binaries exist
    for name, path in binaries:
        if not path.exists():
            logger.error(f"Binary not found: {path} ({name})")
            return 1

    phase = BenchmarkPhase(config, capabilities, benchmark_config)
    output_dir = Path(config.output_dir)

    try:
        result = phase.run(
            binaries=binaries,
            datadir=Path(config.datadir) if config.datadir else None,
            output_dir=output_dir,
        )
        logger.info(f"Results saved to: {result.results_file}")

        # For instrumented runs, also generate plots
        if config.instrumented:
            from bench.analyze import AnalyzePhase

            analyze_phase = AnalyzePhase()

            for binary_result in result.binaries:
                if binary_result.debug_log:
                    try:
                        analyze_phase.run(
                            commit=binary_result.name,
                            log_file=binary_result.debug_log,
                            output_dir=output_dir / "plots",
                        )
                    except Exception as e:
                        logger.warning(f"Analysis for {binary_result.name} failed: {e}")

        return 0
    except Exception as e:
        logger.error(f"Benchmark failed: {e}")
        if args.verbose:
            import traceback

            traceback.print_exc()
        return 1


def cmd_compare(args: argparse.Namespace) -> int:
    """Compare benchmark results from multiple files."""
    from bench.compare import ComparePhase

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    results_files = [Path(f) for f in args.results_files]

    # Validate files exist
    for f in results_files:
        if not f.exists():
            logger.error(f"Results file not found: {f}")
            return 1

    phase = ComparePhase()

    try:
        result = phase.run(results_files, baseline=args.baseline)

        # Output results
        output_json = phase.to_json(result)

        if args.output:
            output_path = Path(args.output)
            output_path.write_text(output_json)
            logger.info(f"Comparison saved to: {output_path}")
        else:
            print(output_json)

        return 0
    except Exception as e:
        logger.error(f"Comparison failed: {e}")
        if args.verbose:
            import traceback

            traceback.print_exc()
        return 1


def cmd_analyze(args: argparse.Namespace) -> int:
    """Generate plots from debug.log."""
    from bench.analyze import AnalyzePhase

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    log_file = Path(args.log_file)
    output_dir = Path(args.output_dir)

    if not log_file.exists():
        logger.error(f"Log file not found: {log_file}")
        return 1

    phase = AnalyzePhase()

    try:
        result = phase.run(
            commit=args.commit,
            log_file=log_file,
            output_dir=output_dir,
        )
        logger.info(f"Generated {len(result.plots)} plots in {result.output_dir}")
        return 0
    except Exception as e:
        logger.error(f"Analysis failed: {e}")
        if args.verbose:
            import traceback

            traceback.print_exc()
        return 1


def cmd_report(args: argparse.Namespace) -> int:
    """Generate HTML report from benchmark results."""
    from bench.report import ReportPhase

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    output_dir = Path(args.output_dir)
    nightly_history_file = Path(args.nightly_history) if args.nightly_history else None
    phase = ReportPhase(nightly_history_file=nightly_history_file)

    try:
        # CI multi-network mode
        if args.networks:
            network_dirs = {}
            for spec in args.networks:
                if ":" not in spec:
                    logger.error(f"Invalid network spec '{spec}': must be NETWORK:PATH")
                    return 1
                network, path = spec.split(":", 1)
                network_dirs[network] = Path(path)

            # Validate directories exist
            for network, path in network_dirs.items():
                if not path.exists():
                    logger.error(f"Network directory not found: {path} ({network})")
                    return 1

            result = phase.run_multi_network(
                network_dirs=network_dirs,
                output_dir=output_dir,
                title=args.title or "Benchmark Results",
                pr_number=args.pr_number,
                run_id=args.run_id,
                commit=args.commit,
            )

            # Update results index if we have a results directory
            # Note: This writes to results/index.html, not the main index.html
            # The main index.html is generated by the nightly benchmark chart
            if args.update_index:
                results_base = output_dir.parent.parent  # Go up from pr-N/run-id
                if results_base.exists():
                    phase.update_index(results_base, results_base / "index.html")
        else:
            # Standard single-directory mode
            input_dir = Path(args.input_dir)

            if not input_dir.exists():
                logger.error(f"Input directory not found: {input_dir}")
                return 1

            result = phase.run(
                input_dir=input_dir,
                output_dir=output_dir,
                title=args.title or "Benchmark Results",
            )

        # Print nightly comparison (speedups vs nightly)
        if result.speedups:
            logger.info("Comparison to nightly:")
            for config, speedup in result.speedups.items():
                sign = "+" if speedup > 0 else ""
                logger.info(f"  {config}: {sign}{speedup}%")

        return 0
    except Exception as e:
        logger.error(f"Report generation failed: {e}")
        if args.verbose:
            import traceback

            traceback.print_exc()
        return 1


def cmd_nightly(args: argparse.Namespace) -> int:
    """Manage nightly benchmark history and charts."""
    from bench.nightly import NightlyPhase

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    if not args.nightly_command:
        logger.error("No nightly subcommand specified. Use 'append' or 'chart'.")
        return 1

    history_file = Path(args.history_file)
    phase = NightlyPhase(history_file)

    try:
        if args.nightly_command == "append":
            benchmark_config_file = (
                Path(args.benchmark_config) if args.benchmark_config else None
            )
            phase.append(
                results_file=Path(args.results_file),
                commit=args.commit,
                config=args.config,
                dbcache=args.dbcache,
                date_str=args.date,
                capture_machine=args.capture_machine,
                benchmark_config_file=benchmark_config_file,
            )
            logger.info(f"Appended result to {history_file}")
        elif args.nightly_command == "chart":
            phase.chart(output_file=Path(args.output_file))
            logger.info(f"Generated chart at {args.output_file}")
        return 0
    except Exception as e:
        logger.error(f"Nightly operation failed: {e}")
        if args.verbose:
            import traceback

            traceback.print_exc()
        return 1


def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Benchcoin - Bitcoin Core benchmarking toolkit",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument(
        "--config",
        metavar="PATH",
        help="Config file (default: bench.toml)",
    )
    parser.add_argument(
        "--profile",
        choices=["quick", "full", "ci"],
        default="full",
        help="Configuration profile (default: full)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Verbose output",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be done without executing",
    )

    subparsers = parser.add_subparsers(dest="command", help="Commands")

    # Build command
    build_parser = subparsers.add_parser(
        "build",
        help="Build bitcoind at one or more commits",
        description="Build bitcoind binaries from git commits. "
        "Each commit can optionally have a name suffix: COMMIT:NAME",
    )
    build_parser.add_argument(
        "commits",
        nargs="+",
        metavar="COMMIT[:NAME]",
        help="Commit(s) to build. Format: COMMIT or COMMIT:NAME (e.g., HEAD:latest, abc123:v27)",
    )
    build_parser.add_argument(
        "-o",
        "--output-dir",
        metavar="PATH",
        help="Where to store binaries (default: ./binaries)",
    )
    build_parser.add_argument(
        "--skip-existing",
        action="store_true",
        help="Skip build if binary already exists",
    )
    build_parser.set_defaults(func=cmd_build)

    # Run command
    run_parser = subparsers.add_parser(
        "run",
        help="Run benchmark on one or more binaries",
        description="Benchmark bitcoind binaries using hyperfine. "
        "Each binary must have a name and path: NAME:PATH",
    )
    run_parser.add_argument(
        "binaries",
        nargs="+",
        metavar="NAME:PATH",
        help="Binary(ies) to benchmark. Format: NAME:PATH (e.g., v27:./binaries/v27/bitcoind)",
    )
    run_parser.add_argument(
        "--datadir",
        metavar="PATH",
        help="Source datadir with blockchain snapshot (omit for fresh sync)",
    )
    run_parser.add_argument(
        "--tmp-datadir",
        metavar="PATH",
        help="Temp datadir for benchmark runs",
    )
    run_parser.add_argument(
        "-o",
        "--output-dir",
        metavar="PATH",
        help="Output directory for results (default: ./bench-output)",
    )
    run_parser.add_argument(
        "--no-cache-drop",
        action="store_true",
        help="Skip cache dropping between runs",
    )
    run_parser.add_argument(
        "--benchmark-config",
        required=True,
        metavar="PATH",
        help="Benchmark config TOML file (e.g., bench/configs/pr.toml)",
    )
    run_parser.add_argument(
        "--matrix-entry",
        required=True,
        metavar="NAME",
        help="Matrix entry to run (e.g., 'default', 'large-instrumented')",
    )
    run_parser.set_defaults(func=cmd_run)

    # Analyze command
    analyze_parser = subparsers.add_parser(
        "analyze", help="Generate plots from debug.log"
    )
    analyze_parser.add_argument("commit", help="Commit hash (for naming)")
    analyze_parser.add_argument("log_file", help="Path to debug.log")
    analyze_parser.add_argument(
        "--output-dir",
        default="./plots",
        metavar="PATH",
        help="Output directory for plots",
    )
    analyze_parser.set_defaults(func=cmd_analyze)

    # Compare command
    compare_parser = subparsers.add_parser(
        "compare",
        help="Compare benchmark results from multiple files",
        description="Load and compare results from one or more results.json files. "
        "Calculates speedup percentages relative to a baseline.",
    )
    compare_parser.add_argument(
        "results_files",
        nargs="+",
        metavar="RESULTS_FILE",
        help="results.json file(s) to compare",
    )
    compare_parser.add_argument(
        "--baseline",
        metavar="NAME",
        help="Name of the baseline entry (default: first entry)",
    )
    compare_parser.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        help="Output file for comparison JSON (default: stdout)",
    )
    compare_parser.set_defaults(func=cmd_compare)

    # Report command
    report_parser = subparsers.add_parser(
        "report",
        help="Generate HTML report",
        description="Generate HTML report from benchmark results. "
        "Use --network for multi-network CI reports.",
    )
    report_parser.add_argument(
        "input_dir",
        nargs="?",
        help="Directory with results.json (for single-network mode)",
    )
    report_parser.add_argument("output_dir", help="Output directory for report")
    report_parser.add_argument(
        "--title",
        help="Report title",
    )
    # CI multi-network options
    report_parser.add_argument(
        "--network",
        dest="networks",
        action="append",
        metavar="NAME:PATH",
        help="Network results directory (repeatable, e.g., --network mainnet:./mainnet-results)",
    )
    report_parser.add_argument(
        "--pr-number",
        metavar="N",
        help="PR number (for CI reports)",
    )
    report_parser.add_argument(
        "--run-id",
        metavar="ID",
        help="Run ID (for CI reports)",
    )
    report_parser.add_argument(
        "--update-index",
        action="store_true",
        help="Update main index.html (for CI reports)",
    )
    report_parser.add_argument(
        "--nightly-history",
        metavar="PATH",
        help="Path to nightly-history.json for comparison against nightly baseline",
    )
    report_parser.add_argument(
        "--commit",
        metavar="SHA",
        help="PR commit hash (for chart display)",
    )
    report_parser.set_defaults(func=cmd_report)

    # Nightly command
    nightly_parser = subparsers.add_parser(
        "nightly",
        help="Manage nightly benchmark history and charts",
        description="Commands for managing nightly benchmark results history "
        "and generating the historical trend chart.",
    )
    nightly_parser.add_argument(
        "--history-file",
        default="nightly-history.json",
        metavar="PATH",
        help="Path to nightly history JSON file (default: nightly-history.json)",
    )
    nightly_subparsers = nightly_parser.add_subparsers(
        dest="nightly_command", help="Nightly commands"
    )

    # nightly append
    nightly_append = nightly_subparsers.add_parser(
        "append",
        help="Append a result to the nightly history",
        description="Parse a hyperfine results.json file and append the result "
        "to the nightly history JSON file.",
    )
    nightly_append.add_argument(
        "results_file",
        help="Path to hyperfine results.json file",
    )
    nightly_append.add_argument(
        "commit",
        help="Git commit hash",
    )
    nightly_append.add_argument(
        "config",
        help="Configuration name (e.g., '450', '32000' from matrix entry)",
    )
    nightly_append.add_argument(
        "dbcache",
        type=int,
        help="DB cache size in MB (450 or 32000)",
    )
    nightly_append.add_argument(
        "--date",
        metavar="YYYY-MM-DD",
        help="Date for this result (default: today)",
    )
    nightly_append.add_argument(
        "--capture-machine",
        action="store_true",
        help="Detect and store machine specs (CPU, architecture, disk type)",
    )
    nightly_append.add_argument(
        "--benchmark-config",
        metavar="PATH",
        help="Benchmark config TOML file to store with results",
    )

    # nightly chart
    nightly_chart = nightly_subparsers.add_parser(
        "chart",
        help="Generate the nightly trend chart HTML",
        description="Generate an HTML page with an interactive Plotly chart "
        "showing nightly benchmark results over time.",
    )
    nightly_chart.add_argument(
        "output_file",
        help="Path to write the chart HTML (typically index.html)",
    )

    nightly_parser.set_defaults(func=cmd_nightly)

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
