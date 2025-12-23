"""Report phase - generate HTML reports from benchmark results.

Ported from the JavaScript logic in .github/workflows/publish-results.yml.
"""

from __future__ import annotations

import json
import logging
import re
import shutil
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

logger = logging.getLogger(__name__)

# HTML template for individual run report
RUN_REPORT_TEMPLATE = """<!DOCTYPE html>
<html>
  <head>
    <title>Benchmark Results</title>
    <link href="https://cdn.jsdelivr.net/npm/tailwindcss@2.2.19/dist/tailwind.min.css" rel="stylesheet">
  </head>
  <body class="bg-gray-100 p-8">
    <div class="w-9/10 mx-auto">
      <h1 class="text-3xl font-bold mb-8">Benchmark Results</h1>
      <div class="bg-white rounded-lg shadow p-6 mb-8">
        <h2 class="text-xl font-semibold mb-4">{title}</h2>

        <!-- Run Data Table -->
        <h3 class="text-lg font-semibold mb-4">Run Data</h3>
        <div class="overflow-x-auto mb-8">
          <table class="min-w-full table-auto">
            <thead>
              <tr class="bg-gray-50">
                <th class="px-4 py-2 text-left">Network</th>
                <th class="px-4 py-2">Command</th>
                <th class="px-4 py-2">Mean (s)</th>
                <th class="px-4 py-2">Std Dev</th>
                <th class="px-4 py-2">User (s)</th>
                <th class="px-4 py-2">System (s)</th>
              </tr>
            </thead>
            <tbody>
              {run_data_rows}
            </tbody>
          </table>
        </div>

        <!-- Speedup Summary Table -->
        <h3 class="text-lg font-semibold mb-4">Speedup Summary</h3>
        <div class="overflow-x-auto mb-8">
          <table class="min-w-full table-auto">
            <thead>
              <tr class="bg-gray-50">
                <th class="px-4 py-2 text-left">Network</th>
                <th class="px-4 py-2">Speedup (%)</th>
              </tr>
            </thead>
            <tbody>
              {speedup_rows}
            </tbody>
          </table>
        </div>

        <!-- Flamegraphs and Plots -->
        {graphs_section}
      </div>
    </div>
  </body>
</html>"""

# HTML template for main index
INDEX_TEMPLATE = """<!DOCTYPE html>
<html>
  <head>
    <title>Bitcoin Benchmark Results</title>
    <link href="https://cdn.jsdelivr.net/npm/tailwindcss@2.2.19/dist/tailwind.min.css" rel="stylesheet">
  </head>
  <body class="bg-gray-100 p-8">
    <div class="w-9/10 mx-auto">
      <h1 class="text-3xl font-bold mb-8">Bitcoin Benchmark Results</h1>
      <div class="bg-white rounded-lg shadow p-6">
        <h2 class="text-xl font-semibold mb-4">Available Results</h2>
        <ul class="space-y-2">
          {run_list}
        </ul>
      </div>
    </div>
  </body>
</html>"""


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
        self, repo_url: str = "https://github.com/bitcoin-dev-tools/benchcoin"
    ):
        self.repo_url = repo_url

    def generate_multi_network(
        self,
        network_dirs: dict[str, Path],
        output_dir: Path,
        title: str = "Benchmark Results",
        pr_number: str | None = None,
        run_id: str | None = None,
    ) -> ReportResult:
        """Generate HTML report from multiple network benchmark results.

        Args:
            network_dirs: Dict mapping network name to directory containing results.json
            output_dir: Where to write the HTML report
            title: Title for the report
            pr_number: PR number (for CI reports)
            run_id: Run ID (for CI reports)

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

        # Calculate speedups per network
        speedups = self._calculate_speedups_per_network(all_runs)

        # Build title with PR/run info if provided
        full_title = title
        if pr_number and run_id:
            full_title = f"PR #{pr_number} - Run {run_id}"

        # Generate HTML
        html = self._generate_html(
            all_runs, speedups, full_title, output_dir, output_dir
        )

        # Write report
        index_file = output_dir / "index.html"
        index_file.write_text(html)
        logger.info(f"Generated report: {index_file}")

        # Write combined results.json
        combined_results = {
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
            "speedups": speedups,
        }
        results_file = output_dir / "results.json"
        results_file.write_text(json.dumps(combined_results, indent=2))

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
        """Generate HTML report from benchmark artifacts.

        Args:
            input_dir: Directory containing results.json and artifacts
            output_dir: Where to write the HTML report
            title: Title for the report

        Returns:
            ReportResult with paths and speedup data
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

        # Calculate speedups
        speedups = self._calculate_speedups(runs)

        # Generate HTML
        html = self._generate_html(runs, speedups, title, input_dir, output_dir)

        # Write report
        index_file = output_dir / "index.html"
        index_file.write_text(html)
        logger.info(f"Generated report: {index_file}")

        # Copy artifacts (flamegraphs, plots)
        self._copy_artifacts(input_dir, output_dir)

        return ReportResult(
            output_dir=output_dir,
            index_file=index_file,
            speedups=speedups,
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
        runs = []

        if results_dir.exists():
            for pr_dir in sorted(results_dir.iterdir()):
                if pr_dir.is_dir() and pr_dir.name.startswith("pr-"):
                    pr_num = pr_dir.name.replace("pr-", "")
                    pr_runs = []
                    for run_dir in sorted(pr_dir.iterdir()):
                        if run_dir.is_dir():
                            pr_runs.append(run_dir.name)
                    if pr_runs:
                        runs.append((pr_num, pr_runs))

        run_list_html = ""
        for pr_num, pr_runs in runs:
            run_links = "\n".join(
                f'<li><a href="results/pr-{pr_num}/{run}/index.html" '
                f'class="text-blue-600 hover:underline">Run {run}</a></li>'
                for run in pr_runs
            )
            run_list_html += f"""
            <li class="font-semibold">PR #{pr_num}
              <ul class="ml-8 space-y-1">
                {run_links}
              </ul>
            </li>
            """

        html = INDEX_TEMPLATE.format(run_list=run_list_html)
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

    def _calculate_speedups(self, runs: list[BenchmarkRun]) -> dict[str, float]:
        """Calculate speedup percentages.

        Uses the first entry as baseline and compares all others against it.
        Returns a dict mapping command name to speedup percentage.
        """
        speedups = {}

        if len(runs) < 2:
            return speedups

        # Use first run as baseline
        baseline = runs[0]
        baseline_mean = baseline.mean

        if baseline_mean <= 0:
            return speedups

        # Calculate speedup for each other run
        for run in runs[1:]:
            speedup = ((baseline_mean - run.mean) / baseline_mean) * 100
            # Use command name as key, extracting just the name part
            name = run.command
            speedups[name] = round(speedup, 1)

        return speedups

    def _calculate_speedups_per_network(
        self, runs: list[BenchmarkRun]
    ) -> dict[str, float]:
        """Calculate speedup percentages per network.

        For each network, uses 'base' as baseline and calculates speedup for 'head'.
        Returns a dict mapping network name to speedup percentage.
        """
        speedups = {}

        # Group runs by network
        networks: dict[str, list[BenchmarkRun]] = {}
        for run in runs:
            if run.network not in networks:
                networks[run.network] = []
            networks[run.network].append(run)

        # Calculate speedup for each network
        for network, network_runs in networks.items():
            base_mean = None
            head_mean = None

            for run in network_runs:
                if run.command == "base":
                    base_mean = run.mean
                elif run.command == "head":
                    head_mean = run.mean

            if base_mean and head_mean and base_mean > 0:
                speedup = ((base_mean - head_mean) / base_mean) * 100
                speedups[network] = round(speedup, 1)

        return speedups

    def _copy_network_artifacts(
        self, network: str, input_dir: Path, output_dir: Path
    ) -> None:
        """Copy artifacts from a network directory with network prefix."""
        # Copy flamegraphs with network prefix
        for svg in input_dir.glob("*-flamegraph.svg"):
            dest = output_dir / f"{network}-{svg.name}"
            shutil.copy2(svg, dest)
            logger.debug(f"Copied {svg.name} as {dest.name}")

        # Copy plots directory with network prefix
        plots_dir = input_dir / "plots"
        if plots_dir.exists():
            dest_plots = output_dir / f"{network}-plots"
            if dest_plots.exists():
                shutil.rmtree(dest_plots)
            shutil.copytree(plots_dir, dest_plots)
            logger.debug(f"Copied plots to {dest_plots}")

    def _generate_html(
        self,
        runs: list[BenchmarkRun],
        speedups: dict[str, float],
        title: str,
        input_dir: Path,
        output_dir: Path,
    ) -> str:
        """Generate the HTML report."""
        # Sort runs by network then by command (base first)
        sorted_runs = sorted(
            runs,
            key=lambda r: (r.network, 0 if "base" in r.command.lower() else 1),
        )

        # Generate run data rows
        run_data_rows = ""
        for run in sorted_runs:
            # Create commit link if there's a commit hash in the command
            command_html = self._linkify_commit(run.command)

            stddev_str = f"{run.stddev:.3f}" if run.stddev else "N/A"

            run_data_rows += f"""
              <tr class="border-t">
                <td class="px-4 py-2 font-mono text-sm">{run.network}</td>
                <td class="px-4 py-2 font-mono text-sm text-center">{command_html}</td>
                <td class="px-4 py-2 text-center">{run.mean:.3f}</td>
                <td class="px-4 py-2 text-center">{stddev_str}</td>
                <td class="px-4 py-2 text-center">{run.user:.3f}</td>
                <td class="px-4 py-2 text-center">{run.system:.3f}</td>
              </tr>
            """

        # Generate speedup rows
        speedup_rows = ""
        for name, speedup in speedups.items():
            # Skip instrumented runs in speedup summary
            if name.lower().endswith("-instrumented"):
                continue

            color_class = ""
            if speedup > 0:
                color_class = "text-green-600"
            elif speedup < 0:
                color_class = "text-red-600"

            sign = "+" if speedup > 0 else ""
            speedup_rows += f"""
              <tr class="border-t">
                <td class="px-4 py-2 font-mono text-sm">{name}</td>
                <td class="px-4 py-2 text-center {color_class}">{sign}{speedup}%</td>
              </tr>
            """

        # Generate graphs section
        graphs_section = self._generate_graphs_section(runs, input_dir, output_dir)

        return RUN_REPORT_TEMPLATE.format(
            title=title,
            run_data_rows=run_data_rows,
            speedup_rows=speedup_rows,
            graphs_section=graphs_section,
        )

    def _linkify_commit(self, command: str) -> str:
        """Convert commit hashes in command to links."""

        def replace_commit(match):
            commit = match.group(1)
            short_commit = commit[:8] if len(commit) > 8 else commit
            return f'(<a href="{self.repo_url}/commit/{commit}" target="_blank" class="text-blue-600 hover:underline">{short_commit}</a>)'

        return re.sub(r"\(([a-f0-9]{7,40})\)", replace_commit, command)

    def _generate_graphs_section(
        self,
        runs: list[BenchmarkRun],
        input_dir: Path,
        output_dir: Path,
    ) -> str:
        """Generate the flamegraphs and plots section."""
        graphs_html = ""

        for run in runs:
            # Use the command/name directly (e.g., "base", "head")
            name = run.command
            network = run.network

            # Check for flamegraph - try both with and without network prefix
            # Network-prefixed: {network}-{name}-flamegraph.svg (for multi-network reports)
            # Non-prefixed: {name}-flamegraph.svg (for single-network reports)
            flamegraph_name = None
            flamegraph_path = None

            network_prefixed = f"{network}-{name}-flamegraph.svg"
            non_prefixed = f"{name}-flamegraph.svg"

            if (output_dir / network_prefixed).exists():
                flamegraph_name = network_prefixed
                flamegraph_path = output_dir / network_prefixed
            elif (input_dir / non_prefixed).exists():
                flamegraph_name = non_prefixed
                flamegraph_path = input_dir / non_prefixed

            # Check for plots - try both network-prefixed and non-prefixed directories
            plot_files = []
            plots_dir = None

            network_plots_dir = output_dir / f"{network}-plots"
            regular_plots_dir = input_dir / "plots"

            if network_plots_dir.exists():
                plots_dir = network_plots_dir
                plot_files = [
                    p.name
                    for p in plots_dir.iterdir()
                    if p.name.startswith(f"{name}-") and p.suffix == ".png"
                ]
            elif regular_plots_dir.exists():
                plots_dir = regular_plots_dir
                plot_files = [
                    p.name
                    for p in plots_dir.iterdir()
                    if p.name.startswith(f"{name}-") and p.suffix == ".png"
                ]

            if not flamegraph_path and not plot_files:
                continue

            # Build display label
            display_label = f"{network} - {name}" if network != "default" else name

            graphs_html += f"""
            <div class="mb-8">
              <h4 class="text-md font-medium mb-2">{display_label}</h4>
            """

            if flamegraph_path:
                graphs_html += f"""
                <object data="{flamegraph_name}" type="image/svg+xml" width="100%" class="mb-4"></object>
                """

            if plot_files and plots_dir:
                # Determine the relative path for plots
                plots_rel_path = plots_dir.name
                for plot in sorted(plot_files):
                    graphs_html += f"""
                <a href="{plots_rel_path}/{plot}" target="_blank">
                  <img src="{plots_rel_path}/{plot}" alt="{plot}" class="mb-4 max-w-full h-auto">
                </a>
                """

            graphs_html += "</div>"

        if graphs_html:
            return f"""
            <h3 class="text-lg font-semibold mb-4">Flamegraphs and Plots</h3>
            {graphs_html}
            """

        return ""

    def _copy_artifacts(self, input_dir: Path, output_dir: Path) -> None:
        """Copy flamegraphs and plots to output directory."""
        # Skip if input and output are the same directory
        if input_dir.resolve() == output_dir.resolve():
            logger.debug("Input and output are the same directory, skipping copy")
            return

        # Copy flamegraphs
        for svg in input_dir.glob("*-flamegraph.svg"):
            dest = output_dir / svg.name
            shutil.copy2(svg, dest)
            logger.debug(f"Copied {svg.name}")

        # Copy plots directory
        plots_dir = input_dir / "plots"
        if plots_dir.exists():
            dest_plots = output_dir / "plots"
            if dest_plots.exists():
                shutil.rmtree(dest_plots)
            shutil.copytree(plots_dir, dest_plots)
            logger.debug("Copied plots directory")


class ReportPhase:
    """Generate reports from benchmark results."""

    def __init__(
        self, repo_url: str = "https://github.com/bitcoin-dev-tools/benchcoin"
    ):
        self.generator = ReportGenerator(repo_url)

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
    ) -> ReportResult:
        """Generate report from multiple network benchmark results.

        Args:
            network_dirs: Dict mapping network name to directory containing results.json
            output_dir: Where to write the HTML report
            title: Title for the report
            pr_number: PR number (for CI reports)
            run_id: Run ID (for CI reports)

        Returns:
            ReportResult with paths and speedup data
        """
        return self.generator.generate_multi_network(
            network_dirs, output_dir, title, pr_number, run_id
        )

    def update_index(self, results_dir: Path, output_file: Path) -> None:
        """Update the main index.html listing all results.

        Args:
            results_dir: Directory containing pr-* subdirectories
            output_file: Where to write index.html
        """
        self.generator.generate_index(results_dir, output_file)
