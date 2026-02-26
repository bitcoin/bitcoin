"""Report phase - generate HTML reports from benchmark results.

Ported from the JavaScript logic in .github/workflows/publish-results.yml.
"""

from __future__ import annotations

import json
import logging
import shutil
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path
from typing import Any

from bench.analyze import HAS_MATPLOTLIB, LogParser, PlotGenerator
from bench.nightly import (
    NightlyHistory,
    series_color_index,
    series_key,
    series_label,
)
from bench.render import render_template

logger = logging.getLogger(__name__)


def format_config_display(
    dbcache: int,
    machine_id: str | None = None,
    instrumentation: str | None = None,
) -> str:
    """Format config for display.

    Args:
        dbcache: DB cache size in MB
        machine_id: Machine ID (e.g., "amd64", "arm64")
        instrumentation: Instrumentation mode (e.g., "instrumented", "uninstrumented")

    Returns:
        Display string like "dbcache=450MB (amd64, instrumented)"

    Examples:
        >>> format_config_display(450)
        'dbcache=450MB'
        >>> format_config_display(32000, "amd64")
        'dbcache=32GB (amd64)'
        >>> format_config_display(450, "arm64", "instrumented")
        'dbcache=450MB (arm64, instrumented)'
    """
    # Format dbcache with unit
    if dbcache >= 1000:
        cache_str = f"{dbcache // 1000}GB"
    else:
        cache_str = f"{dbcache}MB"

    # Build optional parts (exclude "uninstrumented" as it's the default)
    parts = []
    if machine_id:
        parts.append(machine_id)
    if instrumentation and instrumentation != "uninstrumented":
        parts.append(instrumentation)

    if parts:
        return f"dbcache={cache_str} ({', '.join(parts)})"
    return f"dbcache={cache_str}"


def parse_network_name(network: str) -> tuple[int, str]:
    """Parse a network/config name to extract dbcache and instrumentation.

    Args:
        network: Network name like "450-uninstrumented", "32000-instrumented", "450", "32000"

    Returns:
        Tuple of (dbcache_int, instrumentation_str)

    Examples:
        >>> parse_network_name("450-uninstrumented")
        (450, 'uninstrumented')
        >>> parse_network_name("32000-instrumented")
        (32000, 'instrumented')
        >>> parse_network_name("450")
        (450, 'uninstrumented')
    """
    parts = network.split("-")
    try:
        dbcache = int(parts[0])
    except ValueError:
        dbcache = 0

    instrumentation = parts[1] if len(parts) > 1 else "uninstrumented"
    return dbcache, instrumentation


@dataclass
class BenchmarkRun:
    """Parsed benchmark run data."""

    network: str
    command: str
    mean: float
    stddev: float | None
    user: float
    system: float
    parameters: dict[str, Any] = field(default_factory=dict)


@dataclass
class ReportResult:
    """Result of report generation."""

    output_dir: Path
    index_file: Path
    speedups: dict[str, float]


class ReportGenerator:
    """Generate HTML reports from benchmark results."""

    def __init__(
        self,
        repo_url: str = "https://github.com/bitcoin-dev-tools/benchcoin",
        nightly_history: NightlyHistory | None = None,
    ):
        self.repo_url = repo_url
        self.nightly_history = nightly_history

    def generate_multi_network(
        self,
        network_dirs: dict[str, Path],
        output_dir: Path,
        title: str = "Benchmark Results",
        pr_number: str | None = None,
        run_id: str | None = None,
        commit: str | None = None,
    ) -> ReportResult:
        """Generate HTML report from multiple network benchmark results.

        Args:
            network_dirs: Dict mapping network name to directory containing results.json
            output_dir: Where to write the HTML report
            title: Title for the report
            pr_number: PR number (for CI reports)
            run_id: Run ID (for CI reports)
            commit: Commit hash for PR (used in chart)

        Returns:
            ReportResult with paths and speedup data
        """
        output_dir.mkdir(parents=True, exist_ok=True)

        # Combine results from all networks
        all_runs: list[BenchmarkRun] = []
        for network, input_dir in network_dirs.items():
            results_file = input_dir / "results.json"
            if not results_file.exists():
                logger.warning(
                    f"results.json not found in {input_dir} for network {network}"
                )
                continue

            with open(results_file) as f:
                data = json.load(f)

            # Parse and add network to each run
            for result in data.get("results", []):
                all_runs.append(
                    BenchmarkRun(
                        network=network,
                        command=result.get("command", ""),
                        mean=result.get("mean", 0),
                        stddev=result.get("stddev"),
                        user=result.get("user", 0),
                        system=result.get("system", 0),
                        parameters=result.get("parameters", {}),
                    )
                )

            # Copy artifacts from this network
            self._copy_network_artifacts(network, input_dir, output_dir)

        if not all_runs:
            raise ValueError("No benchmark results found in any network directory")

        # Calculate nightly comparison (for uninstrumented configs only)
        nightly_comparison = self._calculate_nightly_comparison(all_runs, commit)

        # Build title with PR/run info if provided
        full_title = title
        if pr_number and run_id:
            full_title = f"PR #{pr_number} - Run {run_id}"

        # Generate HTML
        html = self._generate_html(
            all_runs, nightly_comparison, full_title, output_dir, output_dir, commit,
            run_id,
        )

        # Write report
        index_file = output_dir / "index.html"
        index_file.write_text(html)
        logger.info(f"Generated report: {index_file}")

        # Write combined results.json with nightly comparison
        combined_results: dict[str, Any] = {
            "results": [
                {
                    "network": run.network,
                    "command": run.command,
                    "mean": run.mean,
                    "stddev": run.stddev,
                    "user": run.user,
                    "system": run.system,
                }
                for run in all_runs
            ],
        }
        if nightly_comparison:
            combined_results["nightly_comparison"] = nightly_comparison

        results_file = output_dir / "results.json"
        results_file.write_text(json.dumps(combined_results, indent=2))

        # Return speedups derived from nightly comparison for backwards compatibility
        speedups = {
            config: data["speedup_percent"]
            for config, data in nightly_comparison.items()
            if data.get("speedup_percent") is not None
        }

        return ReportResult(
            output_dir=output_dir,
            index_file=index_file,
            speedups=speedups,
        )

    def generate(
        self,
        input_dir: Path,
        output_dir: Path,
        title: str = "Benchmark Results",
    ) -> ReportResult:
        """Generate HTML report from benchmark artifacts (single binary mode).

        Args:
            input_dir: Directory containing results.json and artifacts
            output_dir: Where to write the HTML report
            title: Title for the report

        Returns:
            ReportResult with paths
        """
        output_dir.mkdir(parents=True, exist_ok=True)

        # Load results.json
        results_file = input_dir / "results.json"
        if not results_file.exists():
            raise FileNotFoundError(f"results.json not found in {input_dir}")

        with open(results_file) as f:
            data = json.load(f)

        # Parse results
        runs = self._parse_results(data)

        # Copy artifacts first so plots are available for template rendering
        self._copy_artifacts(input_dir, output_dir)

        # Generate HTML (no nightly comparison in single-directory mode)
        html = self._generate_html(runs, {}, title, input_dir, output_dir)

        # Write report
        index_file = output_dir / "index.html"
        index_file.write_text(html)
        logger.info(f"Generated report: {index_file}")

        return ReportResult(
            output_dir=output_dir,
            index_file=index_file,
            speedups={},
        )

    def generate_index(
        self,
        results_dir: Path,
        output_file: Path,
    ) -> None:
        """Generate main index.html listing all available results.

        Args:
            results_dir: Directory containing pr-* subdirectories
            output_file: Where to write index.html
        """
        results = []

        if results_dir.exists():
            for pr_dir in sorted(
                results_dir.iterdir(),
                key=lambda d: (0, int(d.name.replace("pr-", "")))
                if d.name.startswith("pr-") and d.name.replace("pr-", "").isdigit()
                else (1, d.name),
            ):
                if pr_dir.is_dir() and pr_dir.name.startswith("pr-"):
                    pr_num = pr_dir.name.replace("pr-", "")
                    pr_runs = []
                    for run_dir in sorted(pr_dir.iterdir()):
                        if run_dir.is_dir():
                            pr_runs.append(run_dir.name)
                    if pr_runs:
                        results.append((pr_num, pr_runs))

        html = render_template("results-index.html", results=results)
        output_file.write_text(html)
        logger.info(f"Generated index: {output_file}")

    def _parse_results(self, data: dict) -> list[BenchmarkRun]:
        """Parse results from hyperfine JSON output."""
        runs = []

        # Handle both direct hyperfine output and combined results format
        results = data.get("results", [])

        for result in results:
            runs.append(
                BenchmarkRun(
                    network=result.get("network", "default"),
                    command=result.get("command", ""),
                    mean=result.get("mean", 0),
                    stddev=result.get("stddev"),
                    user=result.get("user", 0),
                    system=result.get("system", 0),
                    parameters=result.get("parameters", {}),
                )
            )

        return runs

    def _calculate_nightly_comparison(
        self, runs: list[BenchmarkRun], commit: str | None = None
    ) -> dict[str, dict[str, Any]]:
        """Calculate comparison against nightly baseline.

        Compares PR results against the most recent nightly results for each config.
        Only considers uninstrumented configs (those without '-true' suffix).

        Args:
            runs: List of benchmark runs
            commit: PR commit hash

        Returns:
            Dict mapping config to comparison data:
            {
                "450": {
                    "pr_mean": 14500.0,
                    "pr_stddev": 100.0,
                    "nightly_mean": 14800.0,
                    "nightly_date": "2026-01-05",
                    "nightly_commit": "abc123...",
                    "speedup_percent": 2.0
                }
            }
        """
        comparison: dict[str, dict[str, Any]] = {}

        if not self.nightly_history:
            logger.warning("No nightly history available for comparison")
            return comparison

        # Group runs by network/config, only uninstrumented
        for run in runs:
            network = run.network

            # Skip instrumented configs
            if network.endswith("-true") or network.endswith("-instrumented"):
                continue

            # Extract base config name (e.g., "450-false" -> "450", "450-uninstrumented" -> "450")
            config = network.replace("-false", "").replace("-uninstrumented", "")

            # Get PR result mean
            pr_mean = run.mean
            pr_stddev = run.stddev

            # Get latest nightly for this config
            nightly = self.nightly_history.get_latest(config)

            if nightly:
                speedup = None
                if nightly.mean > 0:
                    speedup = round(((nightly.mean - pr_mean) / nightly.mean) * 100, 1)

                comparison[config] = {
                    "pr_mean": pr_mean,
                    "pr_stddev": pr_stddev,
                    "pr_commit": commit,
                    "nightly_mean": nightly.mean,
                    "nightly_stddev": nightly.stddev,
                    "nightly_date": nightly.date,
                    "nightly_commit": nightly.commit,
                    "speedup_percent": speedup,
                    "series_key": series_key(nightly),
                    "series_label": series_label(nightly),
                }
            else:
                # No nightly data, just record PR result
                comparison[config] = {
                    "pr_mean": pr_mean,
                    "pr_stddev": pr_stddev,
                    "pr_commit": commit,
                    "nightly_mean": None,
                    "nightly_date": None,
                    "nightly_commit": None,
                    "speedup_percent": None,
                }

        return comparison

    def _copy_network_artifacts(
        self, network: str, input_dir: Path, output_dir: Path
    ) -> None:
        """Copy artifacts from a network directory with network prefix."""
        # Copy flamegraphs with network prefix
        for svg in input_dir.glob("*-flamegraph.svg"):
            dest = output_dir / f"{network}-{svg.name}"
            shutil.copy2(svg, dest)
            logger.debug(f"Copied {svg.name} as {dest.name}")

        # Generate plots from debug logs (logs themselves are available as CI artifacts)
        if HAS_MATPLOTLIB:
            for log in input_dir.glob("*-debug.log"):
                name = log.name.removesuffix("-debug.log")
                prefix = f"{network}-{name}"
                plots_dir = output_dir / "plots"
                plots_dir.mkdir(parents=True, exist_ok=True)
                try:
                    data = LogParser().parse_file(log)
                    plots = PlotGenerator(prefix, plots_dir).generate_all(data)
                    logger.info(f"Generated {len(plots)} plots for {prefix}")
                except Exception:
                    logger.warning(
                        f"Failed to generate plots for {prefix}", exc_info=True
                    )

    def _generate_html(
        self,
        runs: list[BenchmarkRun],
        nightly_comparison: dict[str, dict[str, Any]],
        title: str,
        input_dir: Path,
        output_dir: Path,
        commit: str | None = None,
        run_id: str | None = None,
    ) -> str:
        """Generate the HTML report."""
        sorted_runs = sorted(runs, key=lambda r: r.network)

        runs_data = []
        for run in sorted_runs:
            dbcache, instrumentation = parse_network_name(run.network)
            config_display = format_config_display(
                dbcache, instrumentation=instrumentation
            )
            runs_data.append(
                {
                    "config_display": config_display,
                    "mean": run.mean,
                    "stddev": run.stddev,
                    "user": run.user,
                    "system": run.system,
                }
            )

        nightly_data, pr_chart_data = self._prepare_nightly_data(
            nightly_comparison, commit
        )

        graphs = self._prepare_graphs_data(runs, input_dir, output_dir)

        nightly_chart_data = None
        if pr_chart_data and self.nightly_history:
            nightly_chart_data = self.nightly_history.get_chart_data()

        ci_run_url = f"{self.repo_url}/actions/runs/{run_id}" if run_id else None

        return render_template(
            "pr-report.html",
            title=title,
            runs=runs_data,
            nightly_comparison=nightly_data,
            pr_chart_data=pr_chart_data,
            nightly_chart_data=nightly_chart_data,
            graphs=graphs,
            repo_url=self.repo_url,
            ci_run_url=ci_run_url,
        )

    def _prepare_nightly_data(
        self,
        nightly_comparison: dict[str, dict[str, Any]],
        commit: str | None = None,
    ) -> tuple[dict[str, dict[str, Any]], list[dict]]:
        """Prepare nightly comparison data for template rendering.

        Returns:
            Tuple of (nightly_comparison_with_display, pr_chart_data)
        """
        if not nightly_comparison:
            return {}, []

        result = {}
        pr_chart_data = []

        for config, data in sorted(nightly_comparison.items()):
            try:
                dbcache = int(config)
            except ValueError:
                dbcache = 0

            result[config] = {
                **data,
                "config_display": format_config_display(dbcache),
            }

            if data.get("nightly_mean"):
                key = data.get("series_key", f"unknown|db{config}|0-0")
                pr_chart_data.append(
                    {
                        "config": config,
                        "mean": data["pr_mean"],
                        "stddev": data.get("pr_stddev") or 0,
                        "commit": commit or "unknown",
                        "date": date.today().isoformat(),
                        "series_key": key,
                        "series_label": data.get("series_label", f"{config} dbcache"),
                        "color_index": series_color_index(key),
                    }
                )

        return result, pr_chart_data

    def _prepare_graphs_data(
        self,
        runs: list[BenchmarkRun],
        input_dir: Path,
        output_dir: Path,
    ) -> list[dict]:
        """Prepare flamegraphs and debug logs data for template rendering."""
        graphs = []

        for run in runs:
            name = run.command
            network = run.network

            flamegraph_name = None
            network_prefixed = f"{network}-{name}-flamegraph.svg"
            non_prefixed = f"{name}-flamegraph.svg"

            if (output_dir / network_prefixed).exists():
                flamegraph_name = network_prefixed
            elif (input_dir / non_prefixed).exists():
                flamegraph_name = non_prefixed

            plots = []
            plots_dir = output_dir / "plots"
            if plots_dir.exists():
                for prefix in [f"{network}-{name}", name]:
                    plot_files = sorted(plots_dir.glob(f"{prefix}-*.png"))
                    if plot_files:
                        plots = [f"plots/{p.name}" for p in plot_files]
                        break

            if not flamegraph_name and not plots:
                continue

            display_label = f"{network} - {name}" if network != "default" else name

            graphs.append(
                {
                    "label": display_label,
                    "flamegraph": flamegraph_name,
                    "plots": plots,
                }
            )

        return graphs

    def _copy_artifacts(self, input_dir: Path, output_dir: Path) -> None:
        """Copy flamegraphs and generate plots from debug logs."""
        same_dir = input_dir.resolve() == output_dir.resolve()

        if not same_dir:
            for svg in input_dir.glob("*-flamegraph.svg"):
                dest = output_dir / svg.name
                shutil.copy2(svg, dest)
                logger.debug(f"Copied {svg.name}")

        if HAS_MATPLOTLIB:
            for log in input_dir.glob("*-debug.log"):
                name = log.name.removesuffix("-debug.log")
                plots_dir = output_dir / "plots"
                plots_dir.mkdir(parents=True, exist_ok=True)
                try:
                    data = LogParser().parse_file(log)
                    plots = PlotGenerator(name, plots_dir).generate_all(data)
                    logger.info(f"Generated {len(plots)} plots for {name}")
                except Exception:
                    logger.warning(
                        f"Failed to generate plots for {name}", exc_info=True
                    )


class ReportPhase:
    """Generate reports from benchmark results."""

    def __init__(
        self,
        repo_url: str = "https://github.com/bitcoin-dev-tools/benchcoin",
        nightly_history_file: Path | None = None,
    ):
        nightly_history: NightlyHistory | None = None
        if nightly_history_file and nightly_history_file.exists():
            nightly_history = NightlyHistory(nightly_history_file)
        self.generator = ReportGenerator(repo_url, nightly_history)

    def run(
        self,
        input_dir: Path,
        output_dir: Path,
        title: str = "Benchmark Results",
    ) -> ReportResult:
        """Generate report from benchmark artifacts.

        Args:
            input_dir: Directory containing results.json and artifacts
            output_dir: Where to write the HTML report
            title: Title for the report

        Returns:
            ReportResult with paths and speedup data
        """
        return self.generator.generate(input_dir, output_dir, title)

    def run_multi_network(
        self,
        network_dirs: dict[str, Path],
        output_dir: Path,
        title: str = "Benchmark Results",
        pr_number: str | None = None,
        run_id: str | None = None,
        commit: str | None = None,
    ) -> ReportResult:
        """Generate report from multiple network benchmark results.

        Args:
            network_dirs: Dict mapping network name to directory containing results.json
            output_dir: Where to write the HTML report
            title: Title for the report
            pr_number: PR number (for CI reports)
            run_id: Run ID (for CI reports)
            commit: Commit hash for PR

        Returns:
            ReportResult with paths and speedup data
        """
        return self.generator.generate_multi_network(
            network_dirs, output_dir, title, pr_number, run_id, commit
        )

    def update_index(self, results_dir: Path, output_file: Path) -> None:
        """Update the main index.html listing all results.

        Args:
            results_dir: Directory containing pr-* subdirectories
            output_file: Where to write index.html
        """
        self.generator.generate_index(results_dir, output_file)
