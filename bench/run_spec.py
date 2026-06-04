"""Internal benchmark run specification."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class RunSpec:
    """Concrete settings for one benchmark run."""

    full_ibd: bool
    start_height: int
    runs: int
    dbcache: int
    instrumentation: str = "uninstrumented"
    bitcoind_args: dict[str, Any] = field(default_factory=dict)
    instrumented_debug: list[str] = field(default_factory=list)
    source_file: Path | None = None

    @property
    def is_instrumented(self) -> bool:
        """Whether this run records profiling artifacts."""
        return self.instrumentation == "instrumented"

    def format_bitcoind_arg(self, key: str, value: Any) -> str | None:
        """Format a single bitcoind argument, skipping empty values."""
        if value is None or value == "":
            return None
        if isinstance(value, bool):
            return f"-{key}={1 if value else 0}"
        return f"-{key}={value}"

    def to_dict(self) -> dict[str, Any]:
        """Convert to a serializable config snapshot."""
        result: dict[str, Any] = {
            "full_ibd": self.full_ibd,
            "start_height": self.start_height,
            "runs": self.runs,
            "instrumentation": self.instrumentation,
            "bitcoind": {
                **{
                    k: v
                    for k, v in self.bitcoind_args.items()
                    if v not in (None, "")
                },
                "dbcache": self.dbcache,
            },
        }
        if self.instrumented_debug:
            result["instrumented_debug"] = self.instrumented_debug
        return result
