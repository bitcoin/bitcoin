"""Benchmark configuration from TOML files.

Provides a portable, reproducible benchmark config that can be shared
to run identical benchmarks on different machines.
"""

from __future__ import annotations

import itertools
import logging
import tomllib
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

logger = logging.getLogger(__name__)


@dataclass
class BenchmarkConfig:
    """Benchmark configuration loaded from TOML.

    This represents a portable benchmark specification that can be shared
    to reproduce benchmarks on different machines.
    """

    # Benchmark metadata
    start_height: int
    runs: int

    # Parameter matrix - each key maps to list of values
    # These create multiple benchmark configurations
    matrix: dict[str, list[Any]] = field(default_factory=dict)

    # All bitcoind flags (optional - empty/missing values excluded from command)
    bitcoind_args: dict[str, Any] = field(default_factory=dict)

    # Instrumented mode debug flags
    instrumented_debug: list[str] = field(default_factory=list)

    # Source file path (for reference)
    source_file: Path | None = None

    @classmethod
    def from_toml(cls, path: Path) -> BenchmarkConfig:
        """Load configuration from a TOML file.

        Expected format:
            [benchmark]
            start_height = 840000
            runs = 2

            [bitcoind]
            stopatheight = 855000
            chain = "main"
            connect = "..."
            prune = 10000
            daemon = false
            printtoconsole = false

            [bitcoind.matrix]
            dbcache = [450, 32000]
            instrumented = [false, true]

            [bitcoind.instrumented]
            debug = ["coindb", "leveldb", "bench", "validation"]
        """
        with open(path, "rb") as f:
            data = tomllib.load(f)

        benchmark = data.get("benchmark", {})
        bitcoind = data.get("bitcoind", {}).copy()

        # Extract matrix from bitcoind section
        matrix: dict[str, list[Any]] = bitcoind.pop("matrix", {})

        # Extract instrumented debug flags (separate from regular bitcoind args)
        instrumented = bitcoind.pop("instrumented", {})
        instrumented_debug = instrumented.get("debug", [])

        config = cls(
            start_height=benchmark.get("start_height", 0),
            runs=benchmark.get("runs", 3),
            matrix=matrix,
            bitcoind_args=bitcoind,
            instrumented_debug=instrumented_debug,
            source_file=path,
        )

        logger.info(f"Loaded benchmark config from {path}")
        logger.info(f"  Start height: {config.start_height}, Runs: {config.runs}")
        if config.matrix:
            logger.info(f"  Matrix parameters: {list(config.matrix.keys())}")
        if config.bitcoind_args:
            logger.info(f"  Bitcoind flags: {list(config.bitcoind_args.keys())}")

        return config

    @staticmethod
    def _value_to_name(value: Any) -> str:
        """Convert a matrix value to a name string."""
        if isinstance(value, bool):
            return str(value).lower()
        return str(value)

    def expand_matrix(self) -> list[dict[str, Any]]:
        """Expand parameter matrix into list of configurations.

        Returns list of dicts, each containing:
            - name: combined name from values like "450-false"
            - All parameter values from the matrix

        Example:
            matrix = {
                'dbcache': [450, 32000],
                'instrumented': [false, true]
            }

            Returns:
                [
                    {'name': '450-false', 'dbcache': 450, 'instrumented': False},
                    {'name': '450-true', 'dbcache': 450, 'instrumented': True},
                    {'name': '32000-false', 'dbcache': 32000, 'instrumented': False},
                    {'name': '32000-true', 'dbcache': 32000, 'instrumented': True},
                ]
        """
        if not self.matrix:
            return [{"name": "default"}]

        # Get all parameter names and their values
        param_names = list(self.matrix.keys())
        param_values = [self.matrix[name] for name in param_names]

        # Generate all combinations
        results = []
        for combination in itertools.product(*param_values):
            entry: dict[str, Any] = {}

            # Build combined name from values
            name_parts = [self._value_to_name(v) for v in combination]
            entry["name"] = "-".join(name_parts)

            # Add each parameter value
            for param_name, value in zip(param_names, combination):
                entry[param_name] = value

            results.append(entry)

        return results

    def get_matrix_entry(self, name: str) -> dict[str, Any] | None:
        """Get a specific matrix entry by its combined name.

        Args:
            name: Combined name like "default-uninstrumented"

        Returns:
            Dict with parameter values, or None if not found
        """
        for entry in self.expand_matrix():
            if entry["name"] == name:
                return entry
        return None

    def get_matrix_names(self) -> list[str]:
        """Get list of all matrix entry names."""
        return [entry["name"] for entry in self.expand_matrix()]

    def _format_bitcoind_arg(self, key: str, value: Any) -> str | None:
        """Format a single bitcoind argument, returning None if it should be skipped."""
        # Skip empty strings and None
        if value is None or value == "":
            return None

        # Format based on type
        if isinstance(value, bool):
            return f"-{key}={1 if value else 0}"
        else:
            return f"-{key}={value}"

    def generate_command_template(self) -> str:
        """Generate bitcoind command template with placeholders.

        Placeholders use {param} format for matrix parameters.
        Empty/missing bitcoind args are excluded.

        Returns command like:
            bitcoind -datadir={datadir} -dbcache={dbcache} -stopatheight=855000 ...
        """
        parts = ["bitcoind"]

        # Placeholder for datadir (always user-provided)
        parts.append("-datadir={datadir}")

        # Matrix parameters as placeholders
        for param_name in self.matrix.keys():
            if param_name != "instrumented":  # instrumented is a flag, not a param
                parts.append(f"-{param_name}={{{param_name}}}")

        # Bitcoind args from config (skip empty/missing)
        for key, value in self.bitcoind_args.items():
            formatted = self._format_bitcoind_arg(key, value)
            if formatted:
                parts.append(formatted)

        return " ".join(parts)

    def get_bitcoind_arg(self, key: str, default: Any = None) -> Any:
        """Get a bitcoind arg value, with optional default."""
        return self.bitcoind_args.get(key, default)

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization.

        This captures the config for logging with results.
        """
        result: dict[str, Any] = {
            "start_height": self.start_height,
            "runs": self.runs,
            "command_template": self.generate_command_template(),
        }

        # Include non-empty bitcoind args
        bitcoind = {k: v for k, v in self.bitcoind_args.items() if v not in (None, "")}
        if bitcoind:
            result["bitcoind"] = bitcoind

        # Include matrix definition
        if self.matrix:
            result["matrix"] = self.matrix

        return result

    def validate(self) -> list[str]:
        """Validate configuration, return list of errors."""
        errors = []

        if self.start_height < 0:
            errors.append("start_height must be non-negative")

        if self.runs < 1:
            errors.append("runs must be positive")

        # Validate matrix entries are non-empty lists
        for param_name, values in self.matrix.items():
            if not values:
                errors.append(f"matrix.{param_name} must have at least one value")

        return errors
