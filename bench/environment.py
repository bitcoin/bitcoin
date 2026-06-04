"""Execution environments for benchcoin phases."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .config import Config


@dataclass(frozen=True)
class BuildEnvironment:
    """Process-level settings for building binaries."""

    binaries_dir: Path
    skip_existing: bool = False
    dry_run: bool = False

    @classmethod
    def from_config(cls, config: Config) -> BuildEnvironment:
        """Build an environment from CLI/config defaults."""
        return cls(
            binaries_dir=Path(config.binaries_dir),
            skip_existing=config.skip_existing,
            dry_run=config.dry_run,
        )


@dataclass(frozen=True)
class BenchmarkEnvironment:
    """Process-level settings for one benchmark execution."""

    tmp_datadir: Path
    no_cache_drop: bool = False
    dry_run: bool = False


@dataclass(frozen=True)
class ExperimentEnvironment:
    """Process-level settings for an experiment run."""

    output_dir: Path
    binaries_dir: Path
    skip_existing: bool = False
    no_cache_drop: bool = False
    dry_run: bool = False

    @classmethod
    def from_config(cls, config: Config) -> ExperimentEnvironment:
        """Build an environment from CLI/config defaults."""
        return cls(
            output_dir=Path(config.output_dir),
            binaries_dir=Path(config.binaries_dir),
            skip_existing=config.skip_existing,
            no_cache_drop=config.no_cache_drop,
            dry_run=config.dry_run,
        )
