"""System capability detection for graceful degradation.

Detects available tools and features, allowing the benchmark to run
on systems without all capabilities (with appropriate warnings).
"""

from __future__ import annotations

import os
import shutil
from dataclasses import dataclass
from pathlib import Path


# Known paths for drop-caches on NixOS
DROP_CACHES_PATHS = [
    "/run/wrappers/bin/drop-caches",
    "/usr/local/bin/drop-caches",
]


@dataclass
class Capabilities:
    """Detected system capabilities."""

    # Cache management
    can_drop_caches: bool
    drop_caches_path: str | None

    # Required tools
    has_hyperfine: bool
    has_flamegraph: bool
    has_perf: bool
    has_nix: bool

    # System info
    cpu_count: int
    is_nixos: bool
    is_ci: bool

    def check_for_run(self, instrumented: bool = False) -> list[str]:
        """Check if we have required capabilities for a benchmark run.

        Returns list of errors (empty if all good).
        """
        errors = []

        if not self.has_hyperfine:
            errors.append("hyperfine not found in PATH (required for benchmarking)")

        if instrumented:
            if not self.has_flamegraph:
                errors.append(
                    "flamegraph not found in PATH (required for --instrumented)"
                )
            if not self.has_perf:
                errors.append("perf not found in PATH (required for --instrumented)")

        return errors

    def check_for_build(self) -> list[str]:
        """Check if we have required capabilities for building.

        Returns list of errors (empty if all good).
        """
        errors = []

        if not self.has_nix:
            errors.append("nix not found in PATH (required for building)")

        return errors

    def get_warnings(self) -> list[str]:
        """Get warnings about missing optional capabilities."""
        warnings = []

        if not self.can_drop_caches:
            warnings.append(
                "drop-caches not available - cache won't be cleared between runs"
            )

        return warnings


def _check_executable(name: str) -> bool:
    """Check if an executable is available in PATH."""
    return shutil.which(name) is not None


def _find_drop_caches() -> str | None:
    """Find drop-caches executable."""
    for path in DROP_CACHES_PATHS:
        if Path(path).exists() and os.access(path, os.X_OK):
            return path
    return None


def _is_nixos() -> bool:
    """Check if we're running on NixOS."""
    return Path("/etc/NIXOS").exists()


def detect_capabilities() -> Capabilities:
    """Auto-detect system capabilities."""
    drop_caches_path = _find_drop_caches()

    return Capabilities(
        can_drop_caches=drop_caches_path is not None,
        drop_caches_path=drop_caches_path,
        has_hyperfine=_check_executable("hyperfine"),
        has_flamegraph=_check_executable("flamegraph"),
        has_perf=_check_executable("perf"),
        has_nix=_check_executable("nix"),
        cpu_count=os.cpu_count() or 1,
        is_nixos=_is_nixos(),
        is_ci=os.environ.get("CI", "").lower() in ("true", "1", "yes"),
    )
