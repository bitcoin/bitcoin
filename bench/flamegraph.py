"""Flamegraph generation helpers."""

from __future__ import annotations

import logging
import subprocess
from dataclasses import dataclass
from pathlib import Path

from .capabilities import Capabilities

logger = logging.getLogger(__name__)


@dataclass
class DifferentialFlamegraphResult:
    """Result of generating differential flamegraphs."""

    before_svg: Path
    after_svg: Path


class DifferentialFlamegraphPhase:
    """Generate differential flamegraphs from two perf.data files."""

    def __init__(self, capabilities: Capabilities):
        self.capabilities = capabilities

    def run(
        self,
        before_perf: Path,
        after_perf: Path,
        output_dir: Path,
        before_name: str = "before",
        after_name: str = "after",
    ) -> DifferentialFlamegraphResult:
        """Generate before-widths and after-widths differential flamegraphs."""
        errors = self.capabilities.check_for_diff_flamegraphs()
        if errors:
            raise RuntimeError(
                "Differential flamegraph prerequisites not met:\n" + "\n".join(errors)
            )

        if not before_perf.exists():
            raise FileNotFoundError(f"Before perf data not found: {before_perf}")
        if not after_perf.exists():
            raise FileNotFoundError(f"After perf data not found: {after_perf}")

        output_dir.mkdir(parents=True, exist_ok=True)

        before_folded = self._fold_perf(before_perf, output_dir, before_name)
        after_folded = self._fold_perf(after_perf, output_dir, after_name)

        after_svg = output_dir / "diff-after.svg"
        before_svg = output_dir / "diff-before.svg"

        self._write_diff(
            first_folded=before_folded,
            second_folded=after_folded,
            output_svg=after_svg,
            title=f"{after_name} widths, {after_name} - {before_name}",
        )
        self._write_diff(
            first_folded=after_folded,
            second_folded=before_folded,
            output_svg=before_svg,
            title=f"{before_name} widths, {after_name} - {before_name}",
            negate=True,
        )

        logger.info(f"Generated differential flamegraph: {after_svg}")
        logger.info(f"Generated differential flamegraph: {before_svg}")

        return DifferentialFlamegraphResult(before_svg=before_svg, after_svg=after_svg)

    def _fold_perf(self, perf_data: Path, output_dir: Path, name: str) -> Path:
        """Convert perf.data to folded stacks."""
        stacks_file = output_dir / f"{name}.perf.stacks"
        folded_file = output_dir / f"{name}.folded"

        logger.info(f"Generating perf script output: {stacks_file}")
        with stacks_file.open("w") as stacks:
            subprocess.run(
                ["perf", "script", "-i", str(perf_data)],
                check=True,
                stdout=stacks,
            )

        logger.info(f"Folding stacks: {folded_file}")
        with folded_file.open("w") as folded:
            subprocess.run(
                ["stackcollapse-perf.pl", str(stacks_file)],
                check=True,
                stdout=folded,
            )

        return folded_file

    def _write_diff(
        self,
        first_folded: Path,
        second_folded: Path,
        output_svg: Path,
        title: str,
        negate: bool = False,
    ) -> None:
        """Write one differential flamegraph."""
        diff_cmd = [
            "difffolded.pl",
            "-n",
            "-x",
            str(first_folded),
            str(second_folded),
        ]
        flamegraph_cmd = ["flamegraph.pl", "--title", title]
        if negate:
            flamegraph_cmd.append("--negate")

        logger.info(f"Writing differential flamegraph: {output_svg}")
        with output_svg.open("w") as svg:
            diff_proc = subprocess.Popen(diff_cmd, stdout=subprocess.PIPE)
            try:
                subprocess.run(
                    flamegraph_cmd,
                    stdin=diff_proc.stdout,
                    stdout=svg,
                    check=True,
                )
            finally:
                if diff_proc.stdout:
                    diff_proc.stdout.close()

            if diff_proc.wait() != 0:
                raise subprocess.CalledProcessError(diff_proc.returncode, diff_cmd)
