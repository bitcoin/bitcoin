"""Configuration management for benchcoin.

Layered configuration (lowest to highest priority):
1. Built-in defaults
2. bench.toml config file
3. Environment variables (BENCH_*)
4. CLI arguments
"""

from __future__ import annotations

import os
import tomllib
from dataclasses import dataclass
from pathlib import Path
from typing import Any


# Built-in defaults
DEFAULTS = {
    "chain": "main",
    "dbcache": 450,
    "stop_height": 855000,
    "runs": 3,
    "connect": "",  # Empty = use public P2P network
    "binaries_dir": "./binaries",
    "output_dir": "./bench-output",
}

# Profile overrides
PROFILES = {
    "quick": {
        "stop_height": 1500,
        "runs": 1,
    },
    "full": {
        "stop_height": 855000,
        "runs": 3,
    },
    "ci": {
        "stop_height": 855000,
        "runs": 3,
        "connect": "148.251.128.115:33333",
    },
}

# Environment variable mapping
ENV_MAPPING = {
    "BENCH_DATADIR": "datadir",
    "BENCH_TMP_DATADIR": "tmp_datadir",
    "BENCH_BINARIES_DIR": "binaries_dir",
    "BENCH_OUTPUT_DIR": "output_dir",
    "BENCH_STOP_HEIGHT": "stop_height",
    "BENCH_DBCACHE": "dbcache",
    "BENCH_CONNECT": "connect",
    "BENCH_RUNS": "runs",
    "BENCH_CHAIN": "chain",
}


@dataclass
class Config:
    """Benchmark configuration."""

    # Core benchmark settings
    chain: str = "main"
    dbcache: int = 450
    stop_height: int = 855000
    runs: int = 3
    connect: str = ""  # Empty = use public P2P network

    # Paths
    datadir: str | None = None
    tmp_datadir: str | None = None
    binaries_dir: str = "./binaries"
    output_dir: str = "./bench-output"

    # Behavior flags
    instrumented: str = "uninstrumented"  # "uninstrumented" or "instrumented"
    skip_existing: bool = False
    no_cache_drop: bool = False
    verbose: bool = False
    dry_run: bool = False

    # Profile used (for reference)
    profile: str = "full"

    @property
    def is_instrumented(self) -> bool:
        """Whether instrumented mode is enabled (flamegraphs, debug logs)."""
        return self.instrumented == "instrumented"

    def __post_init__(self) -> None:
        # If tmp_datadir not set, derive from output_dir
        if self.tmp_datadir is None:
            self.tmp_datadir = str(Path(self.output_dir) / "tmp-datadir")

        # Instrumented mode forces runs=1
        if self.is_instrumented and self.runs != 1:
            self.runs = 1

    def validate(self) -> list[str]:
        """Validate configuration, return list of errors."""
        errors = []

        # datadir is optional (None = fresh sync)
        if self.datadir is not None and not Path(self.datadir).exists():
            errors.append(f"datadir does not exist: {self.datadir}")

        if self.stop_height < 1:
            errors.append("stop_height must be positive")

        if self.dbcache < 1:
            errors.append("dbcache must be positive")

        if self.runs < 1:
            errors.append("runs must be positive")

        if self.chain not in ("main", "testnet", "signet", "regtest"):
            errors.append(f"invalid chain: {self.chain}")

        return errors


def load_toml(path: Path) -> tuple[dict[str, Any], dict[str, dict[str, Any]]]:
    """Load configuration from TOML file.

    Returns:
        Tuple of (base_config, profiles_dict)
    """
    if not path.exists():
        return {}, {}

    with open(path, "rb") as f:
        data = tomllib.load(f)

    # Flatten structure: merge [defaults] and [paths] into top level
    result = {}
    if "defaults" in data:
        result.update(data["defaults"])
    if "paths" in data:
        result.update(data["paths"])

    # Extract profiles
    profiles = data.get("profiles", {})

    return result, profiles


def load_env() -> dict[str, Any]:
    """Load configuration from environment variables."""
    result = {}

    for env_var, config_key in ENV_MAPPING.items():
        value = os.environ.get(env_var)
        if value is not None:
            # Convert numeric values
            if config_key in ("stop_height", "dbcache", "runs"):
                try:
                    value = int(value)
                except ValueError:
                    pass  # Keep as string, will fail validation
            result[config_key] = value

    return result


def apply_profile(
    config: dict[str, Any],
    profile_name: str,
    toml_profiles: dict[str, dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Apply a named profile to configuration.

    Args:
        config: Base configuration dict
        profile_name: Name of profile to apply
        toml_profiles: Profiles loaded from TOML file (override built-in)
    """
    result = config.copy()
    result["profile"] = profile_name

    # Apply built-in profile first
    if profile_name in PROFILES:
        result.update(PROFILES[profile_name])

    # Then apply TOML profile (overrides built-in)
    if toml_profiles and profile_name in toml_profiles:
        result.update(toml_profiles[profile_name])

    return result


def build_config(
    cli_args: dict[str, Any] | None = None,
    config_file: Path | None = None,
    profile: str = "full",
) -> Config:
    """Build configuration from all sources.

    Priority (lowest to highest):
    1. Built-in defaults
    2. Config file (bench.toml) base settings
    3. Built-in profile overrides
    4. Config file profile overrides
    5. Environment variables
    6. CLI arguments
    """
    # Start with defaults
    config = DEFAULTS.copy()

    # Load config file
    if config_file is None:
        config_file = Path("bench.toml")
    file_config, toml_profiles = load_toml(config_file)
    config.update(file_config)

    # Apply profile (built-in first, then TOML overrides)
    config = apply_profile(config, profile, toml_profiles)

    # Load environment variables
    env_config = load_env()
    config.update(env_config)

    # Apply CLI arguments (filter out None values)
    if cli_args:
        for key, value in cli_args.items():
            if value is not None:
                config[key] = value

    # Build Config object (filter to only valid fields)
    valid_fields = {f.name for f in Config.__dataclass_fields__.values()}
    filtered = {k: v for k, v in config.items() if k in valid_fields}

    return Config(**filtered)
