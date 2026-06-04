"""Artifact layout and metadata for experiment outputs."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any


@dataclass(frozen=True)
class RunArtifactRecord:
    """Serializable metadata for one subject/profile run."""

    subject: str
    profile: str
    output_dir: Path
    results_file: Path
    debug_log: Path | None = None
    flamegraph: Path | None = None
    perf_data: Path | None = None
    folded_stacks: Path | None = None


class ArtifactStore:
    """Own experiment artifact paths and metadata."""

    def __init__(self, root: Path):
        self.root = root

    @property
    def manifest_path(self) -> Path:
        """Path to the experiment artifact manifest."""
        return self.root / "artifacts.json"

    def run_dir(self, profile: str, subject: str) -> Path:
        """Directory for one profile/subject run."""
        return self.root / "runs" / profile / subject

    def tmp_datadir(self, tmp_root: Path | None, profile: str, subject: str) -> Path:
        """Temporary datadir for one profile/subject run."""
        if tmp_root:
            return tmp_root / profile / subject
        return self.run_dir(profile, subject) / "tmp-datadir"

    def comparison_dir(self, name: str, profile: str) -> Path:
        """Directory for one comparison/profile derivation."""
        return self.root / "comparisons" / name / profile

    def write_manifest(
        self,
        runs: list[RunArtifactRecord],
        comparisons: list[Path],
    ) -> Path:
        """Write artifact metadata."""
        self.root.mkdir(parents=True, exist_ok=True)
        data = {
            "runs": [self._record_to_dict(record) for record in runs],
            "comparisons": [
                str(path.relative_to(self.root)) if path.is_relative_to(self.root) else str(path)
                for path in comparisons
            ],
        }
        self.manifest_path.write_text(json.dumps(data, indent=2) + "\n")
        return self.manifest_path

    def load_manifest(self) -> dict[str, Any]:
        """Load artifact metadata."""
        return json.loads(self.manifest_path.read_text())

    def _record_to_dict(self, record: RunArtifactRecord) -> dict[str, Any]:
        """Convert a run record to relative-path JSON."""
        result: dict[str, Any] = {
            "subject": record.subject,
            "profile": record.profile,
            "output_dir": self._relative(record.output_dir),
            "results_file": self._relative(record.results_file),
        }
        for key in ("debug_log", "flamegraph", "perf_data", "folded_stacks"):
            value = getattr(record, key)
            if value:
                result[key] = self._relative(value)
        return result

    def _relative(self, path: Path) -> str:
        """Return path relative to root when possible."""
        return str(path.relative_to(self.root)) if path.is_relative_to(self.root) else str(path)
