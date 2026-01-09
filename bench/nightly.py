"""Nightly benchmark history management and chart generation."""

from __future__ import annotations

import hashlib
import json
import logging
import re
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Any

logger = logging.getLogger(__name__)

NUM_COLORS = 10


def series_color_index(key: str) -> int:
    """Compute deterministic color index from series key using MD5."""
    hash_bytes = hashlib.md5(key.encode()).digest()
    return int.from_bytes(hash_bytes[:4], "little") % NUM_COLORS


def _normalize_cpu_model(cpu_model: str) -> str:
    """Normalize CPU model to short identifier.

    Examples:
        "AMD Ryzen 7 7700 8-Core Processor" -> "ryzen77008core"
        "AMD EPYC 7763 64-Core Processor" -> "epyc776364core"
        "Apple M2 Pro" -> "applem2pro"
        "Intel(R) Core(TM) i7-12700K" -> "corei712700k"
    """
    model = cpu_model.lower()
    for word in ["processor", "(r)", "(tm)", "amd", "intel"]:
        model = model.replace(word, "")
    parts = [p.strip() for p in model.split() if p.strip()]
    return "".join(parts).replace("-", "").replace(" ", "")[:25]


def _bucket_ram(ram_gb: float) -> int:
    """Bucket RAM to nearest standard size for grouping."""
    if ram_gb <= 0:
        return 0
    if ram_gb <= 32:
        return round(ram_gb / 8) * 8
    return round(ram_gb / 32) * 32


def _normalize_kernel(kernel: str) -> str:
    """Extract major.minor from kernel version."""
    parts = kernel.split(".")
    if len(parts) >= 2:
        return f"{parts[0]}.{parts[1].split('-')[0]}"
    return kernel.split("-")[0]


def _normalize_disk_type(disk_type: str) -> str:
    """Normalize disk type to short form."""
    disk_lower = disk_type.lower()
    if "nvme" in disk_lower:
        return "nvme"
    if "ssd" in disk_lower:
        return "ssd"
    if "hdd" in disk_lower:
        return "hdd"
    return disk_lower[:10]


def _extract_cpu_short_name(cpu_model: str) -> str:
    """Extract a readable short CPU name for labels."""
    patterns = [
        r"Ryzen \d+ \d+",
        r"EPYC \d+",
        r"M\d+ (?:Pro|Max|Ultra)?",
        r"i[3579]-\d+\w*",
        r"Xeon \w+-\d+",
    ]

    for pattern in patterns:
        match = re.search(pattern, cpu_model, re.IGNORECASE)
        if match:
            return match.group(0)

    words = cpu_model.replace("(R)", "").replace("(TM)", "").split()
    meaningful = [
        w
        for w in words
        if w.lower() not in ["processor", "core", "amd", "intel", "apple"]
    ]
    return " ".join(meaningful[:2]) if meaningful else cpu_model[:20]


def series_key(result: "NightlyResult") -> str:
    """Generate unique series key from machine specs and config.

    Format: {cpu_short}|{ram}GB|{disk}|{kernel}|db{dbcache}|{start}-{stop}
    Example: ryzen77008core|64GB|nvme|6.6|db450|840000-900000
    """
    machine = result.machine or {}
    config = result.config or {}
    bitcoind = config.get("bitcoind", {})

    cpu = _normalize_cpu_model(machine.get("cpu_model", "unknown"))
    ram = _bucket_ram(machine.get("total_ram_gb", 0))
    disk = _normalize_disk_type(machine.get("disk_type", "unknown"))
    kernel = _normalize_kernel(machine.get("os_kernel", "unknown"))

    dbcache = bitcoind.get("dbcache", result.dbcache)
    start = config.get("start_height", 0)
    stop = bitcoind.get("stopatheight", 0)

    return f"{cpu}|{ram}GB|{disk}|{kernel}|db{dbcache}|{start}-{stop}"


def series_label(result: "NightlyResult") -> str:
    """Generate human-readable series label for chart legend."""
    machine = result.machine or {}
    config = result.config or {}
    bitcoind = config.get("bitcoind", {})

    arch = machine.get("architecture", "unknown")
    cpu_model = machine.get("cpu_model", "Unknown")
    cpu_short = _extract_cpu_short_name(cpu_model)
    ram = machine.get("total_ram_gb", 0)
    ram_str = f"{int(ram)}GB RAM" if ram else "?GB RAM"

    start = config.get("start_height", 0)
    stop = bitcoind.get("stopatheight", 0)
    block_range = f"{start}-{stop}" if start and stop else "?-?"

    dbcache = result.dbcache

    return f"{arch}, {cpu_short}, {ram_str}, {block_range}, dbcache {dbcache}"


# HTML template for the nightly chart homepage
NIGHTLY_CHART_TEMPLATE = """<!DOCTYPE html>
<html>
  <head>
    <title>Bitcoin Core Nightly IBD Benchmark</title>
    <link href="https://cdn.jsdelivr.net/npm/tailwindcss@2.2.19/dist/tailwind.min.css" rel="stylesheet">
    <script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
    <style>
      :root {{
        --bg-primary: #f3f4f6;
        --bg-card: #ffffff;
        --text-primary: #111827;
        --text-secondary: #4b5563;
        --text-muted: #6b7280;
        --link-color: #2563eb;
        --shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1);
      }}
      .dark {{
        --bg-primary: #111827;
        --bg-card: #1f2937;
        --text-primary: #f9fafb;
        --text-secondary: #d1d5db;
        --text-muted: #9ca3af;
        --link-color: #60a5fa;
        --shadow: 0 1px 3px 0 rgb(0 0 0 / 0.3);
      }}
      body {{
        background-color: var(--bg-primary);
        color: var(--text-primary);
        transition: background-color 0.2s, color 0.2s;
      }}
      .card {{
        background-color: var(--bg-card);
        box-shadow: var(--shadow);
        transition: background-color 0.2s;
      }}
      .text-secondary {{ color: var(--text-secondary); }}
      .text-muted {{ color: var(--text-muted); }}
      .link {{ color: var(--link-color); }}
      .theme-toggle {{
        position: fixed;
        top: 1rem;
        right: 1rem;
        padding: 0.5rem;
        border-radius: 0.5rem;
        background-color: var(--bg-card);
        border: 1px solid var(--text-muted);
        cursor: pointer;
        font-size: 1.25rem;
        transition: background-color 0.2s;
      }}
      .theme-toggle:hover {{
        opacity: 0.8;
      }}
    </style>
  </head>
  <body class="p-4 md:p-8">
    <button class="theme-toggle" onclick="toggleTheme()" title="Toggle dark/light mode">
      <span id="theme-icon">🌙</span>
    </button>
    <div class="w-full">
      <h1 class="text-3xl font-bold mb-2">Bitcoin Core Nightly IBD Benchmark</h1>
      <p class="text-secondary mb-4">
        IBD from a single networked peer
      </p>
      <div class="card rounded-lg p-4">
        <div id="nightly-chart" style="width:100%; height:calc(100vh - 200px); min-height:400px;"></div>
      </div>
      <p class="text-sm text-muted mt-4 text-center">
        <a href="results/" class="link hover:underline">View PR benchmark results</a>
      </p>
    </div>
    <script>
      const data = {chart_data};

      // Detect system preference and saved preference
      function getPreferredTheme() {{
        const saved = localStorage.getItem('theme');
        if (saved) return saved;
        return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
      }}

      function setTheme(theme) {{
        document.documentElement.classList.toggle('dark', theme === 'dark');
        document.getElementById('theme-icon').textContent = theme === 'dark' ? '☀️' : '🌙';
        localStorage.setItem('theme', theme);
        updateChart(theme);
      }}

      function toggleTheme() {{
        const current = document.documentElement.classList.contains('dark') ? 'dark' : 'light';
        setTheme(current === 'dark' ? 'light' : 'dark');
      }}

      // Colorblind-friendly palette
      const PALETTE = [
        '#636EFA', '#EF553B', '#00CC96', '#AB63FA', '#FFA15A',
        '#19D3F3', '#FF6692', '#B6E880', '#FF97FF', '#FECB52'
      ];

      // Get color from pre-computed index (computed in Python using MD5)
      function getSeriesColor(colorIndex) {{
        return PALETTE[colorIndex % PALETTE.length];
      }}

      function getErrorColor(hexColor) {{
        const r = parseInt(hexColor.slice(1, 3), 16);
        const g = parseInt(hexColor.slice(3, 5), 16);
        const b = parseInt(hexColor.slice(5, 7), 16);
        return `rgba(${{r}}, ${{g}}, ${{b}}, 0.3)`;
      }}

      // Convert seconds to minutes
      const toMinutes = (seconds) => seconds / 60;

      function buildTraces() {{
        // Group data by series_key
        const seriesMap = new Map();

        data.forEach(d => {{
          const key = d.series_key || d.config;  // Fallback for legacy data
          if (!seriesMap.has(key)) {{
            seriesMap.set(key, {{
              label: d.series_label || (d.config + ' dbcache'),
              colorIndex: d.color_index || 0,
              points: []
            }});
          }}
          seriesMap.get(key).points.push(d);
        }});

        const traces = [];

        // Sort series by label for consistent legend order
        const sortedSeries = Array.from(seriesMap.entries())
          .sort((a, b) => a[1].label.localeCompare(b[1].label));

        sortedSeries.forEach(([seriesKey, series]) => {{
          const points = series.points.sort((a, b) => a.date.localeCompare(b.date));
          const color = getSeriesColor(series.colorIndex);

          traces.push({{
            name: series.label,
            x: points.map(d => d.date),
            y: points.map(d => toMinutes(d.mean)),
            text: points.map(d => d.commit.slice(0, 8)),
            customdata: points.map(d => [d.commit, toMinutes(d.stddev || 0)]),
            hovertemplate: '<b>%{{text}}</b><br>%{{y:.1f}} min<br>\u00b1%{{customdata[1]:.1f}} min<extra>' + series.label + '</extra>',
            mode: 'lines+markers',
            line: {{ color: color, width: 2 }},
            marker: {{ size: 8 }},
            error_y: {{
              type: 'data',
              array: points.map(d => toMinutes(d.stddev || 0)),
              visible: true,
              color: getErrorColor(color),
              thickness: 1.5
            }}
          }});
        }});

        return traces;
      }}

      function getLayout(theme) {{
        const isDark = theme === 'dark';
        return {{
          title: {{
            text: 'Sync from block 840,000 to 900,000',
            font: {{ size: 18, color: isDark ? '#f9fafb' : '#111827' }}
          }},
          xaxis: {{
            title: {{ text: 'Date', font: {{ color: isDark ? '#d1d5db' : '#4b5563' }} }},
            tickangle: -45,
            tickfont: {{ color: isDark ? '#9ca3af' : '#6b7280' }},
            gridcolor: isDark ? '#374151' : '#e5e7eb',
            linecolor: isDark ? '#4b5563' : '#d1d5db'
          }},
          yaxis: {{
            title: {{ text: 'Time (minutes)', font: {{ color: isDark ? '#d1d5db' : '#4b5563' }} }},
            rangemode: 'tozero',
            tickfont: {{ color: isDark ? '#9ca3af' : '#6b7280' }},
            gridcolor: isDark ? '#374151' : '#e5e7eb',
            linecolor: isDark ? '#4b5563' : '#d1d5db'
          }},
          legend: {{
            orientation: 'h',
            yanchor: 'top',
            y: -0.15,
            xanchor: 'center',
            x: 0.5,
            font: {{ color: isDark ? '#d1d5db' : '#4b5563', size: 11 }},
            bgcolor: isDark ? 'rgba(31, 41, 55, 0.8)' : 'rgba(255, 255, 255, 0.8)',
            bordercolor: isDark ? '#4b5563' : '#d1d5db',
            borderwidth: 1,
            itemclick: 'toggle',
            itemdoubleclick: 'toggleothers'
          }},
          hovermode: 'closest',
          hoverlabel: {{
            bgcolor: isDark ? '#1f2937' : '#ffffff',
            bordercolor: isDark ? '#4b5563' : '#d1d5db',
            font: {{ color: isDark ? '#f9fafb' : '#111827', size: 13 }}
          }},
          margin: {{ t: 80, b: 150 }},
          paper_bgcolor: 'rgba(0,0,0,0)',
          plot_bgcolor: 'rgba(0,0,0,0)'
        }};
      }}

      const config = {{
        responsive: true,
        displayModeBar: 'hover',
        modeBarButtonsToRemove: ['lasso2d', 'select2d', 'autoScale2d'],
        displaylogo: false
      }};

      function updateChart(theme) {{
        Plotly.react('nightly-chart', buildTraces(), getLayout(theme), config);
      }}

      // Initialize
      setTheme(getPreferredTheme());
    </script>
  </body>
</html>"""


@dataclass
class NightlyResult:
    """A single nightly benchmark result with embedded config and machine info."""

    date: str
    commit: str
    mean: float
    stddev: float
    runs: int
    config: dict[
        str, Any
    ]  # Full benchmark config (dbcache inside config.bitcoind.dbcache)
    machine: dict[str, Any]  # Full machine specs

    @property
    def dbcache(self) -> int:
        """Extract dbcache from nested config."""
        return self.config.get("bitcoind", {}).get("dbcache", 0)

    @property
    def machine_id(self) -> str:
        """Get short machine ID from architecture."""
        from bench.machine import ARCH_TO_ID

        arch = self.machine.get("architecture", "unknown")
        return ARCH_TO_ID.get(arch.lower(), arch.lower())

    @property
    def instrumentation(self) -> str:
        """Get instrumentation mode from config."""
        return self.config.get("instrumentation", "uninstrumented")

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        return {
            "date": self.date,
            "commit": self.commit,
            "mean": self.mean,
            "stddev": self.stddev,
            "runs": self.runs,
            "config": self.config,
            "machine": self.machine,
        }

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> NightlyResult:
        """Create from dictionary.

        Handles both new format (embedded config/machine) and legacy format.
        """
        # Check if this is the new format (has config as dict)
        if isinstance(data.get("config"), dict):
            return cls(
                date=data["date"],
                commit=data["commit"],
                mean=data["mean"],
                stddev=data["stddev"],
                runs=data["runs"],
                config=data["config"],
                machine=data.get("machine", {}),
            )

        # Legacy format - convert to new format
        dbcache = data.get("dbcache", 0)
        return cls(
            date=data["date"],
            commit=data["commit"],
            mean=data["mean"],
            stddev=data["stddev"],
            runs=data["runs"],
            config={
                "bitcoind": {"dbcache": dbcache},
                "instrumentation": "uninstrumented",
            },
            machine={},
        )


class NightlyHistory:
    """Manages the nightly benchmark history stored in JSON.

    Each result is self-contained with its own config and machine info.
    """

    def __init__(self, history_file: Path):
        self.history_file = history_file
        self.results: list[NightlyResult] = []
        self._load()

    def _load(self) -> None:
        """Load history from JSON file."""
        if self.history_file.exists():
            with open(self.history_file) as f:
                data = json.load(f)
            self.results = [NightlyResult.from_dict(r) for r in data.get("results", [])]
            logger.info(f"Loaded {len(self.results)} results from {self.history_file}")
        else:
            self.results = []
            logger.info(f"No existing history at {self.history_file}")

    def save(self) -> None:
        """Save history to JSON file."""
        self.history_file.parent.mkdir(parents=True, exist_ok=True)
        data: dict = {"results": [r.to_dict() for r in self.results]}
        with open(self.history_file, "w") as f:
            json.dump(data, f, indent=2)
        logger.info(f"Saved {len(self.results)} results to {self.history_file}")

    def append(self, result: NightlyResult) -> None:
        """Append a new result to history."""
        # Check for duplicate (same date, commit, dbcache)
        for existing in self.results:
            if (
                existing.date == result.date
                and existing.commit == result.commit
                and existing.dbcache == result.dbcache
            ):
                logger.warning(
                    f"Duplicate result for {result.date} {result.commit[:8]} dbcache={result.dbcache}, replacing"
                )
                self.results.remove(existing)
                break

        self.results.append(result)
        # Sort by date, then dbcache
        self.results.sort(key=lambda r: (r.date, r.dbcache))
        logger.info(
            f"Appended result: {result.date} {result.commit[:8]} dbcache={result.dbcache} {result.mean:.1f}s"
        )

    def get_latest(self, dbcache: int | str) -> NightlyResult | None:
        """Get the most recent result for a given dbcache config.

        Args:
            dbcache: DB cache size in MB (int) or config name like '450', '32000'

        Returns:
            Most recent NightlyResult for that dbcache, or None if not found
        """
        # Handle string config names like '450', '32000'
        if isinstance(dbcache, str):
            try:
                dbcache = int(dbcache)
            except ValueError:
                return None

        matching = [r for r in self.results if r.dbcache == dbcache]
        if not matching:
            return None
        # Results are sorted by date, so last one is most recent
        return matching[-1]

    def get_chart_data(self) -> list[dict]:
        """Get results in format suitable for chart embedding.

        Returns data with series_key and series_label for dynamic grouping.
        Also includes legacy 'config' field for backward compatibility.
        """
        chart_data = []
        for r in self.results:
            key = series_key(r)
            chart_data.append(
                {
                    "date": r.date,
                    "commit": r.commit,
                    "mean": r.mean,
                    "stddev": r.stddev,
                    "config": str(r.dbcache),  # Legacy compatibility
                    "series_key": key,
                    "series_label": series_label(r),
                    "color_index": series_color_index(key),
                }
            )
        return chart_data

    def append_from_results_json(
        self,
        results_file: Path,
        commit: str,
        benchmark_config: dict[str, Any],
        machine_specs: dict[str, Any],
        date_str: str | None = None,
    ) -> None:
        """Append result from a hyperfine results.json file.

        Args:
            results_file: Path to hyperfine results.json
            commit: Git commit hash
            benchmark_config: Full benchmark config dict (includes bitcoind.dbcache)
            machine_specs: Machine specs dict
            date_str: Date string (YYYY-MM-DD), defaults to today
        """
        if not results_file.exists():
            raise FileNotFoundError(f"Results file not found: {results_file}")

        with open(results_file) as f:
            data = json.load(f)

        # Hyperfine output has a "results" array with one entry per command
        # For nightly, we only have one command (master)
        results = data.get("results", [])
        if not results:
            raise ValueError(f"No results found in {results_file}")

        # Use the first (and should be only) result
        result_data = results[0]
        mean = result_data.get("mean", 0)
        stddev = result_data.get("stddev", 0)
        runs = len(result_data.get("times", []))

        if date_str is None:
            date_str = date.today().isoformat()

        result = NightlyResult(
            date=date_str,
            commit=commit,
            mean=mean,
            stddev=stddev if stddev else 0,
            runs=runs,
            config=benchmark_config,
            machine=machine_specs,
        )
        self.append(result)


def generate_nightly_chart(history: NightlyHistory, output_file: Path) -> None:
    """Generate the nightly chart HTML page.

    Args:
        history: NightlyHistory instance with loaded results
        output_file: Path to write index.html
    """
    # Convert results to simplified format for JS chart (config as string, not object)
    chart_data = json.dumps(history.get_chart_data())

    html = NIGHTLY_CHART_TEMPLATE.format(chart_data=chart_data)

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(html)
    logger.info(f"Generated nightly chart: {output_file}")


# HTML/JS snippet for PR comparison chart (embedded in report)
PR_CHART_SNIPPET = """
<div id="pr-comparison-chart" style="width:100%; height:800px;"></div>
<script src="https://cdn.plot.ly/plotly-2.27.0.min.js"></script>
<script>
  const nightlyData = {nightly_data};
  const prData = {pr_data};

  // Colorblind-friendly palette
  const PALETTE = [
    '#636EFA', '#EF553B', '#00CC96', '#AB63FA', '#FFA15A',
    '#19D3F3', '#FF6692', '#B6E880', '#FF97FF', '#FECB52'
  ];

  // Get color from pre-computed index (computed in Python using MD5)
  function getSeriesColor(colorIndex) {{
    return PALETTE[colorIndex % PALETTE.length];
  }}

  function getErrorColor(hexColor) {{
    const r = parseInt(hexColor.slice(1, 3), 16);
    const g = parseInt(hexColor.slice(3, 5), 16);
    const b = parseInt(hexColor.slice(5, 7), 16);
    return `rgba(${{r}}, ${{g}}, ${{b}}, 0.3)`;
  }}

  const toMinutes = (seconds) => seconds / 60;

  function buildTraces() {{
    const traces = [];

    // Group nightly data by series_key
    const nightlySeriesMap = new Map();
    nightlyData.forEach(d => {{
      const key = d.series_key || d.config;
      if (!nightlySeriesMap.has(key)) {{
        nightlySeriesMap.set(key, {{
          label: d.series_label || (d.config + ' dbcache'),
          colorIndex: d.color_index || 0,
          points: []
        }});
      }}
      nightlySeriesMap.get(key).points.push(d);
    }});

    // Add nightly traces
    const sortedSeries = Array.from(nightlySeriesMap.entries())
      .sort((a, b) => a[1].label.localeCompare(b[1].label));

    sortedSeries.forEach(([seriesKey, series]) => {{
      const points = series.points.sort((a, b) => a.date.localeCompare(b.date));
      const color = getSeriesColor(series.colorIndex);

      traces.push({{
        name: series.label + ' (nightly)',
        x: points.map(d => d.date),
        y: points.map(d => toMinutes(d.mean)),
        text: points.map(d => d.commit.slice(0, 8)),
        customdata: points.map(d => [d.commit, toMinutes(d.stddev || 0)]),
        hovertemplate: '<b>%{{text}}</b><br>%{{y:.1f}} min<br>\\u00b1%{{customdata[1]:.1f}} min<extra>' + series.label + ' nightly</extra>',
        mode: 'lines+markers',
        line: {{ color: color, width: 2 }},
        marker: {{ size: 6 }},
        error_y: {{
          type: 'data',
          array: points.map(d => toMinutes(d.stddev || 0)),
          visible: true,
          color: getErrorColor(color),
          thickness: 1.5
        }}
      }});
    }});

    // Add PR traces (star markers)
    prData.forEach(pr => {{
      const key = pr.series_key || pr.config;
      const label = pr.series_label || (pr.config + ' dbcache');
      const color = getSeriesColor(pr.color_index || 0);

      traces.push({{
        name: label + ' (PR)',
        x: [pr.date],
        y: [toMinutes(pr.mean)],
        text: [pr.commit.slice(0, 8)],
        customdata: [[pr.commit, toMinutes(pr.stddev || 0)]],
        hovertemplate: '<b>PR %{{text}}</b><br>%{{y:.1f}} min<br>\\u00b1%{{customdata[1]:.1f}} min<extra>' + label + ' PR</extra>',
        mode: 'markers',
        marker: {{
          symbol: 'star',
          size: 16,
          color: color,
          line: {{ color: '#ffffff', width: 2 }}
        }},
        showlegend: false
      }});
    }});

    return traces;
  }}

  const layout = {{
    title: {{
      text: 'PR vs Nightly Sync Performance',
      font: {{ size: 16 }}
    }},
    xaxis: {{
      title: {{ text: 'Date' }},
      tickangle: -45
    }},
    yaxis: {{
      title: {{ text: 'Time (minutes)' }},
      rangemode: 'tozero'
    }},
    legend: {{
      orientation: 'h',
      yanchor: 'top',
      y: -0.15,
      xanchor: 'center',
      x: 0.5,
      font: {{ size: 11 }},
      itemclick: 'toggle',
      itemdoubleclick: 'toggleothers'
    }},
    hovermode: 'closest',
    margin: {{ t: 60, b: 150 }}
  }};

  const config = {{
    responsive: true,
    displayModeBar: 'hover',
    modeBarButtonsToRemove: ['lasso2d', 'select2d', 'autoScale2d'],
    displaylogo: false
  }};

  Plotly.newPlot('pr-comparison-chart', buildTraces(), layout, config);
</script>
"""


def generate_pr_chart_snippet(
    history: NightlyHistory,
    pr_results: list[dict],
) -> str:
    """Generate HTML/JS snippet for PR comparison chart.

    Args:
        history: NightlyHistory with nightly results
        pr_results: List of PR result dicts with keys: config, mean, stddev, commit, date

    Returns:
        HTML string to embed in report
    """
    nightly_data = json.dumps(history.get_chart_data())
    pr_data = json.dumps(pr_results)

    return PR_CHART_SNIPPET.format(nightly_data=nightly_data, pr_data=pr_data)


class NightlyPhase:
    """CLI interface for nightly benchmark operations."""

    def __init__(self, history_file: Path):
        self.history_file = history_file

    def append(
        self,
        results_file: Path,
        commit: str,
        dbcache: int,
        date_str: str | None = None,
        benchmark_config_file: Path | None = None,
        instrumentation: str = "uninstrumented",
        machine_specs_file: Path | None = None,
    ) -> None:
        """Append a result from hyperfine results.json to history.

        Args:
            results_file: Path to hyperfine results.json
            commit: Git commit hash
            dbcache: DB cache size in MB
            date_str: Date string (YYYY-MM-DD), defaults to today
            benchmark_config_file: Path to benchmark config TOML
            instrumentation: Instrumentation mode ('uninstrumented' or 'instrumented')
            machine_specs_file: Path to pre-captured machine specs JSON (optional)
        """
        from bench.benchmark_config import BenchmarkConfig

        history = NightlyHistory(self.history_file)

        # Get machine specs from file if provided, otherwise detect current machine
        if machine_specs_file:
            machine_specs = json.loads(machine_specs_file.read_text())
            logger.info(f"Using pre-captured machine specs from {machine_specs_file}")
        else:
            from bench.machine import get_machine_specs

            machine_specs = get_machine_specs().to_dict()

        # Build benchmark config dict
        if benchmark_config_file:
            benchmark_config = BenchmarkConfig.from_toml(benchmark_config_file)
            config_dict = benchmark_config.to_dict()
        else:
            config_dict = {}

        # Ensure dbcache is in the config
        if "bitcoind" not in config_dict:
            config_dict["bitcoind"] = {}
        config_dict["bitcoind"]["dbcache"] = dbcache
        config_dict["instrumentation"] = instrumentation

        history.append_from_results_json(
            results_file=results_file,
            commit=commit,
            benchmark_config=config_dict,
            machine_specs=machine_specs,
            date_str=date_str,
        )
        history.save()

    def chart(self, output_file: Path) -> None:
        """Generate the nightly chart HTML.

        Args:
            output_file: Path to write index.html
        """
        history = NightlyHistory(self.history_file)
        generate_nightly_chart(history, output_file)
