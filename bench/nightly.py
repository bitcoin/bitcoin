"""Nightly benchmark history management and chart generation."""

from __future__ import annotations

import json
import logging
from dataclasses import dataclass
from datetime import date
from pathlib import Path
from typing import Any

logger = logging.getLogger(__name__)

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
        IBD from a single networked peer from block 840,000 to 900,000 on a Hetzner AX52
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

      // Colors that work well in both themes
      const colors = {{
        '450': {{ line: '#3b82f6', error: 'rgba(59, 130, 246, 0.3)' }},    // Blue
        '32000': {{ line: '#f97316', error: 'rgba(249, 115, 22, 0.3)' }}   // Orange
      }};

      // Convert seconds to minutes
      const toMinutes = (seconds) => seconds / 60;

      function buildTraces() {{
        const smallData = data.filter(d => d.config === '450');
        const largeData = data.filter(d => d.config === '32000');
        const traces = [];

        if (smallData.length > 0) {{
          traces.push({{
            name: '450MB dbcache',
            x: smallData.map(d => d.date),
            y: smallData.map(d => toMinutes(d.mean)),
            text: smallData.map(d => d.commit.slice(0, 8)),
            customdata: smallData.map(d => [d.commit, toMinutes(d.stddev)]),
            hovertemplate: '<b>%{{text}}</b><br>%{{y:.1f}} min<br>\u00b1%{{customdata[1]:.1f}} min<extra>450MB</extra>',
            mode: 'lines+markers',
            line: {{ color: colors['450'].line, width: 2 }},
            marker: {{ size: 8 }},
            error_y: {{
              type: 'data',
              array: smallData.map(d => toMinutes(d.stddev)),
              visible: true,
              color: colors['450'].error,
              thickness: 1.5
            }}
          }});
        }}

        if (largeData.length > 0) {{
          traces.push({{
            name: '32GB dbcache',
            x: largeData.map(d => d.date),
            y: largeData.map(d => toMinutes(d.mean)),
            text: largeData.map(d => d.commit.slice(0, 8)),
            customdata: largeData.map(d => [d.commit, toMinutes(d.stddev)]),
            hovertemplate: '<b>%{{text}}</b><br>%{{y:.1f}} min<br>\u00b1%{{customdata[1]:.1f}} min<extra>32GB</extra>',
            mode: 'lines+markers',
            line: {{ color: colors['32000'].line, width: 2 }},
            marker: {{ size: 8 }},
            error_y: {{
              type: 'data',
              array: largeData.map(d => toMinutes(d.stddev)),
              visible: true,
              color: colors['32000'].error,
              thickness: 1.5
            }}
          }});
        }}

        return traces;
      }}

      function getLayout(theme) {{
        const isDark = theme === 'dark';
        return {{
          title: {{
            text: 'Sync from block 840,000 to 855,000',
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
            yanchor: 'bottom',
            y: 1.02,
            xanchor: 'right',
            x: 1,
            font: {{ color: isDark ? '#d1d5db' : '#4b5563' }}
          }},
          hovermode: 'closest',
          hoverlabel: {{
            bgcolor: isDark ? '#1f2937' : '#ffffff',
            bordercolor: isDark ? '#4b5563' : '#d1d5db',
            font: {{ color: isDark ? '#f9fafb' : '#111827', size: 13 }}
          }},
          margin: {{ t: 80, b: 80 }},
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
    """A single nightly benchmark result."""

    date: str
    commit: str
    config: str  # 'default' or 'large'
    dbcache: int
    mean: float
    stddev: float
    runs: int

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        return {
            "date": self.date,
            "commit": self.commit,
            "config": self.config,
            "dbcache": self.dbcache,
            "mean": self.mean,
            "stddev": self.stddev,
            "runs": self.runs,
        }

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> NightlyResult:
        """Create from dictionary."""
        return cls(
            date=data["date"],
            commit=data["commit"],
            config=data["config"],
            dbcache=data["dbcache"],
            mean=data["mean"],
            stddev=data["stddev"],
            runs=data["runs"],
        )


class NightlyHistory:
    """Manages the nightly benchmark history stored in JSON."""

    def __init__(self, history_file: Path):
        self.history_file = history_file
        self.results: list[NightlyResult] = []
        self.machine: dict | None = None
        self.config: dict | None = None
        self._load()

    def _load(self) -> None:
        """Load history from JSON file."""
        if self.history_file.exists():
            with open(self.history_file) as f:
                data = json.load(f)
            self.results = [NightlyResult.from_dict(r) for r in data.get("results", [])]
            self.machine = data.get("machine")
            self.config = data.get("config")
            logger.info(f"Loaded {len(self.results)} results from {self.history_file}")
        else:
            self.results = []
            self.machine = None
            self.config = None
            logger.info(f"No existing history at {self.history_file}")

    def save(self) -> None:
        """Save history to JSON file."""
        self.history_file.parent.mkdir(parents=True, exist_ok=True)
        data: dict = {"results": [r.to_dict() for r in self.results]}
        if self.config:
            data["config"] = self.config
        if self.machine:
            data["machine"] = self.machine
        with open(self.history_file, "w") as f:
            json.dump(data, f, indent=2)
        logger.info(f"Saved {len(self.results)} results to {self.history_file}")

    def set_machine(self, machine_specs: dict) -> None:
        """Set or update the machine specifications."""
        self.machine = machine_specs
        logger.info(f"Set machine specs: {machine_specs.get('cpu_model', 'Unknown')}")

    def set_config(self, benchmark_config: dict) -> None:
        """Set or update the benchmark configuration."""
        self.config = benchmark_config
        logger.info(
            f"Set benchmark config: {benchmark_config.get('start_height', '?')} -> "
            f"{benchmark_config.get('stop_height', '?')}"
        )

    def append(self, result: NightlyResult) -> None:
        """Append a new result to history."""
        # Check for duplicate (same date, commit, config)
        for existing in self.results:
            if (
                existing.date == result.date
                and existing.commit == result.commit
                and existing.config == result.config
            ):
                logger.warning(
                    f"Duplicate result for {result.date} {result.commit[:8]} {result.config}, replacing"
                )
                self.results.remove(existing)
                break

        self.results.append(result)
        # Sort by date, then config
        self.results.sort(key=lambda r: (r.date, r.config))
        logger.info(
            f"Appended result: {result.date} {result.commit[:8]} {result.config} {result.mean:.1f}s"
        )

    def get_latest(self, config: str) -> NightlyResult | None:
        """Get the most recent result for a given config.

        Args:
            config: Config name (e.g., '450', '32000')

        Returns:
            Most recent NightlyResult for that config, or None if not found
        """
        matching = [r for r in self.results if r.config == config]
        if not matching:
            return None
        # Results are sorted by date, so last one is most recent
        return matching[-1]

    def get_chart_data(self) -> list[dict]:
        """Get results in format suitable for chart embedding."""
        return [r.to_dict() for r in self.results]

    def append_from_results_json(
        self,
        results_file: Path,
        commit: str,
        config: str,
        dbcache: int,
        date_str: str | None = None,
    ) -> None:
        """Append result from a hyperfine results.json file.

        Args:
            results_file: Path to hyperfine results.json
            commit: Git commit hash
            config: Config name ('default' or 'large')
            dbcache: DB cache size in MB
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
            config=config,
            dbcache=dbcache,
            mean=mean,
            stddev=stddev if stddev else 0,
            runs=runs,
        )
        self.append(result)


def generate_nightly_chart(history: NightlyHistory, output_file: Path) -> None:
    """Generate the nightly chart HTML page.

    Args:
        history: NightlyHistory instance with loaded results
        output_file: Path to write index.html
    """
    # Convert results to JSON for embedding in HTML
    chart_data = json.dumps([r.to_dict() for r in history.results])

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

  const colors = {{
    '450': {{ line: '#3b82f6', error: 'rgba(59, 130, 246, 0.3)' }},
    '32000': {{ line: '#f97316', error: 'rgba(249, 115, 22, 0.3)' }}
  }};
  const prColors = {{
    '450': '#10b981',   // Green for PR 450
    '32000': '#ef4444'  // Red for PR 32000
  }};

  const toMinutes = (seconds) => seconds / 60;

  function buildTraces() {{
    const traces = [];

    // Nightly traces (lines with markers)
    ['450', '32000'].forEach(config => {{
      const configData = nightlyData.filter(d => d.config === config);
      if (configData.length > 0) {{
        traces.push({{
          name: config === '450' ? '450MB nightly' : '32GB nightly',
          x: configData.map(d => d.date),
          y: configData.map(d => toMinutes(d.mean)),
          text: configData.map(d => d.commit.slice(0, 8)),
          customdata: configData.map(d => [d.commit, toMinutes(d.stddev || 0)]),
          hovertemplate: '<b>%{{text}}</b><br>%{{y:.1f}} min<br>\\u00b1%{{customdata[1]:.1f}} min<extra>' + (config === '450' ? '450MB' : '32GB') + ' nightly</extra>',
          mode: 'lines+markers',
          line: {{ color: colors[config].line, width: 2 }},
          marker: {{ size: 6 }},
          error_y: {{
            type: 'data',
            array: configData.map(d => toMinutes(d.stddev || 0)),
            visible: true,
            color: colors[config].error,
            thickness: 1.5
          }}
        }});
      }}
    }});

    // PR traces (star markers)
    prData.forEach(pr => {{
      const config = pr.config;
      traces.push({{
        name: config === '450' ? '450MB PR' : '32GB PR',
        x: [pr.date],
        y: [toMinutes(pr.mean)],
        text: [pr.commit.slice(0, 8)],
        customdata: [[pr.commit, toMinutes(pr.stddev || 0)]],
        hovertemplate: '<b>PR %{{text}}</b><br>%{{y:.1f}} min<br>\\u00b1%{{customdata[1]:.1f}} min<extra>' + (config === '450' ? '450MB' : '32GB') + ' PR</extra>',
        mode: 'markers',
        marker: {{
          symbol: 'star',
          size: 16,
          color: prColors[config],
          line: {{ color: '#ffffff', width: 2 }}
        }}
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
      yanchor: 'bottom',
      y: 1.02,
      xanchor: 'right',
      x: 1
    }},
    hovermode: 'closest',
    margin: {{ t: 60, b: 80 }}
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
        config: str,
        dbcache: int,
        date_str: str | None = None,
        capture_machine: bool = False,
        benchmark_config_file: Path | None = None,
    ) -> None:
        """Append a result from hyperfine results.json to history.

        Args:
            results_file: Path to hyperfine results.json
            commit: Git commit hash
            config: Config name ('default' or 'large')
            dbcache: DB cache size in MB
            date_str: Date string (YYYY-MM-DD), defaults to today
            capture_machine: If True, detect and store machine specs
            benchmark_config_file: If provided, load and store benchmark config
        """
        history = NightlyHistory(self.history_file)

        if capture_machine:
            from bench.machine import get_machine_specs

            specs = get_machine_specs()
            history.set_machine(specs.to_dict())

        if benchmark_config_file:
            from bench.benchmark_config import BenchmarkConfig

            benchmark_config = BenchmarkConfig.from_toml(benchmark_config_file)
            history.set_config(benchmark_config.to_dict())

        history.append_from_results_json(
            results_file, commit, config, dbcache, date_str
        )
        history.save()

    def chart(self, output_file: Path) -> None:
        """Generate the nightly chart HTML.

        Args:
            output_file: Path to write index.html
        """
        history = NightlyHistory(self.history_file)
        generate_nightly_chart(history, output_file)
