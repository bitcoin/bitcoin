"""Compare phase - compare benchmark results from multiple runs."""

from __future__ import annotations

import json
import logging
from dataclasses import dataclass
from pathlib import Path

logger = logging.getLogger(__name__)


@dataclass
class BenchmarkEntry:
    """A single benchmark entry from results.json."""

    command: str
    mean: float
    stddev: float | None
    user: float
    system: float
    min: float
    max: float
    times: list[float]


@dataclass
class Comparison:
    """Comparison of one entry against the baseline."""

    name: str
    mean: float
    baseline_mean: float
    speedup_percent: float
    stddev: float | None


@dataclass
class CompareResult:
    """Result of comparison."""

    baseline: str
    comparisons: list[Comparison]


class ComparePhase:
    """Compare benchmark results from multiple results.json files."""

    def run(
        self,
        results_files: list[Path],
        baseline: str | None = None,
    ) -> CompareResult:
        """Compare benchmark results.

        Args:
            results_files: List of results.json files to compare
            baseline: Name of the baseline entry (default: first entry)

        Returns:
            CompareResult with comparison data
        """
        if not results_files:
            raise ValueError("At least one results file is required")

        # Load all entries from all files
        all_entries: list[BenchmarkEntry] = []
        for results_file in results_files:
            if not results_file.exists():
                raise FileNotFoundError(f"Results file not found: {results_file}")

            logger.info(f"Loading results from: {results_file}")
            with open(results_file) as f:
                data = json.load(f)

            entries = self._parse_results(data)
            logger.info(f"  Found {len(entries)} entries")
            all_entries.extend(entries)

        if not all_entries:
            raise ValueError("No benchmark entries found in results files")

        # Determine baseline
        if baseline is None:
            baseline = all_entries[0].command
        logger.info(f"Using baseline: {baseline}")

        # Find baseline entry
        baseline_entry = None
        for entry in all_entries:
            if entry.command == baseline:
                baseline_entry = entry
                break

        if baseline_entry is None:
            available = [e.command for e in all_entries]
            raise ValueError(
                f"Baseline '{baseline}' not found. Available: {', '.join(available)}"
            )

        # Calculate comparisons
        comparisons: list[Comparison] = []
        for entry in all_entries:
            if entry.command == baseline:
                continue

            speedup = self._calculate_speedup(baseline_entry.mean, entry.mean)
            comparisons.append(
                Comparison(
                    name=entry.command,
                    mean=entry.mean,
                    baseline_mean=baseline_entry.mean,
                    speedup_percent=speedup,
                    stddev=entry.stddev,
                )
            )

        # Log results
        logger.info("Comparison results:")
        logger.info(f"  Baseline ({baseline}): {baseline_entry.mean:.3f}s")
        for comp in comparisons:
            sign = "+" if comp.speedup_percent > 0 else ""
            logger.info(
                f"  {comp.name}: {comp.mean:.3f}s ({sign}{comp.speedup_percent:.1f}%)"
            )

        return CompareResult(
            baseline=baseline,
            comparisons=comparisons,
        )

    def _parse_results(self, data: dict) -> list[BenchmarkEntry]:
        """Parse results from hyperfine JSON output."""
        entries = []

        results = data.get("results", [])
        for result in results:
            entries.append(
                BenchmarkEntry(
                    command=result.get("command", "unknown"),
                    mean=result.get("mean", 0),
                    stddev=result.get("stddev"),
                    user=result.get("user", 0),
                    system=result.get("system", 0),
                    min=result.get("min", 0),
                    max=result.get("max", 0),
                    times=result.get("times", []),
                )
            )

        return entries

    def _calculate_speedup(self, baseline_mean: float, other_mean: float) -> float:
        """Calculate speedup percentage.

        Positive = faster than baseline
        Negative = slower than baseline
        """
        if baseline_mean == 0:
            return 0.0
        return round(((baseline_mean - other_mean) / baseline_mean) * 100, 1)

    def to_json(self, result: CompareResult) -> str:
        """Convert comparison result to JSON."""
        return json.dumps(
            {
                "baseline": result.baseline,
                "comparisons": [
                    {
                        "name": c.name,
                        "mean": c.mean,
                        "baseline_mean": c.baseline_mean,
                        "speedup_percent": c.speedup_percent,
                        "stddev": c.stddev,
                    }
                    for c in result.comparisons
                ],
            },
            indent=2,
        )
