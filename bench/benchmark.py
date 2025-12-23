"""Benchmark phase - run hyperfine benchmarks on bitcoind binaries."""

from __future__ import annotations

import logging
import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING

from .patchelf import ensure_binary_runnable

if TYPE_CHECKING:
    from .benchmark_config import BenchmarkConfig
    from .capabilities import Capabilities
    from .config import Config


logger = logging.getLogger(__name__)

# Debug flags for instrumented mode
INSTRUMENTED_DEBUG_FLAGS = ["coindb", "leveldb", "bench", "validation"]


@dataclass
class BinaryResult:
    """Result for a single binary."""

    name: str
    flamegraph: Path | None = None
    debug_log: Path | None = None


@dataclass
class BenchmarkResult:
    """Result of the benchmark phase."""

    results_file: Path
    instrumented: bool
    binaries: list[BinaryResult] = field(default_factory=list)


def parse_binary_spec(spec: str) -> tuple[str, Path]:
    """Parse a binary spec like 'name:/path/to/binary'.

    Returns (name, path).
    """
    if ":" not in spec:
        raise ValueError(f"Invalid binary spec '{spec}': must be NAME:PATH")
    name, path_str = spec.split(":", 1)
    if not name:
        raise ValueError(f"Invalid binary spec '{spec}': name cannot be empty")
    return name, Path(path_str)


class BenchmarkPhase:
    """Run hyperfine benchmarks on bitcoind binaries."""

    def __init__(
        self,
        config: Config,
        capabilities: Capabilities,
        benchmark_config: BenchmarkConfig | None = None,
    ):
        self.config = config
        self.capabilities = capabilities
        self.benchmark_config = benchmark_config
        self._temp_scripts: list[Path] = []

    def run(
        self,
        binaries: list[tuple[str, Path]],
        datadir: Path | None,
        output_dir: Path,
    ) -> BenchmarkResult:
        """Run benchmarks on given binaries.

        Args:
            binaries: List of (name, binary_path) tuples
            datadir: Source datadir with blockchain snapshot (None for fresh sync)
            output_dir: Where to store results

        Returns:
            BenchmarkResult with paths to outputs
        """
        if not binaries:
            raise ValueError("At least one binary is required")

        # Validate all binaries exist
        for name, path in binaries:
            if not path.exists():
                raise FileNotFoundError(f"Binary not found: {path} ({name})")

        # Ensure binaries can run on this system (patches guix binaries on NixOS)
        for name, path in binaries:
            if not ensure_binary_runnable(path):
                raise RuntimeError(f"Binary {name} at {path} cannot be made runnable")

        # Check prerequisites
        errors = self.capabilities.check_for_run(self.config.instrumented)
        if errors:
            raise RuntimeError("Benchmark prerequisites not met:\n" + "\n".join(errors))

        # Log warnings about missing optional capabilities
        for warning in self.capabilities.get_warnings():
            logger.warning(warning)

        # Setup directories
        output_dir.mkdir(parents=True, exist_ok=True)
        tmp_datadir = Path(self.config.tmp_datadir)
        tmp_datadir.mkdir(parents=True, exist_ok=True)

        results_file = output_dir / "results.json"

        logger.info("Starting benchmark")
        logger.info(f"  Output dir: {output_dir}")
        logger.info(f"  Temp datadir: {tmp_datadir}")
        if datadir:
            logger.info(f"  Source datadir: {datadir}")
        else:
            logger.info("  Mode: Fresh sync (no source datadir)")
        logger.info(f"  Binaries: {len(binaries)}")
        for name, path in binaries:
            logger.info(f"    {name}: {path}")
        logger.info(f"  Instrumented: {self.config.instrumented}")
        logger.info(f"  Runs: {self.config.runs}")
        logger.info(f"  dbcache: {self.config.dbcache}")
        if self.benchmark_config:
            logger.info(f"  Config: {self.benchmark_config.source_file}")

        try:
            # Create hook scripts for hyperfine
            setup_script = self._create_setup_script(tmp_datadir)
            prepare_script = self._create_prepare_script(tmp_datadir, datadir)
            cleanup_script = self._create_cleanup_script(tmp_datadir)

            # Build hyperfine command
            cmd = self._build_hyperfine_cmd(
                binaries=binaries,
                tmp_datadir=tmp_datadir,
                results_file=results_file,
                setup_script=setup_script,
                prepare_script=prepare_script,
                cleanup_script=cleanup_script,
                output_dir=output_dir,
            )

            # Log the commands being benchmarked
            logger.info("Commands to benchmark:")
            for name, path in binaries:
                bitcoind_cmd = self._build_bitcoind_cmd(path, tmp_datadir)
                logger.info(f"  {name}: {bitcoind_cmd}")

            if self.config.dry_run:
                logger.info(f"[DRY RUN] Would run: {' '.join(cmd)}")
                return BenchmarkResult(
                    results_file=results_file,
                    instrumented=self.config.instrumented,
                )

            # Log the full hyperfine command
            logger.info("Running hyperfine...")
            logger.info(f"  Command: {' '.join(cmd[:7])} ...")  # First few args
            logger.debug(f"  Full command: {' '.join(cmd)}")
            subprocess.run(cmd, check=True)

            # Collect results
            benchmark_result = BenchmarkResult(
                results_file=results_file,
                instrumented=self.config.instrumented,
            )

            # For instrumented runs, collect flamegraphs and debug logs
            if self.config.instrumented:
                logger.info("Collecting instrumented artifacts...")
                for name, _path in binaries:
                    binary_result = BinaryResult(name=name)

                    flamegraph_file = output_dir / f"{name}-flamegraph.svg"
                    debug_log_file = output_dir / f"{name}-debug.log"

                    if flamegraph_file.exists():
                        binary_result.flamegraph = flamegraph_file
                        logger.info(f"  Flamegraph ({name}): {flamegraph_file}")
                    if debug_log_file.exists():
                        binary_result.debug_log = debug_log_file
                        logger.info(f"  Debug log ({name}): {debug_log_file}")

                    benchmark_result.binaries.append(binary_result)

            # Clean up tmp_datadir
            if tmp_datadir.exists():
                logger.debug(f"Cleaning up tmp_datadir: {tmp_datadir}")
                shutil.rmtree(tmp_datadir)

            return benchmark_result

        finally:
            # Clean up temp scripts
            for script in self._temp_scripts:
                if script.exists():
                    script.unlink()
            self._temp_scripts.clear()

    def _create_temp_script(self, commands: list[str], name: str) -> Path:
        """Create a temporary shell script."""
        content = "#!/usr/bin/env bash\nset -euxo pipefail\n"
        content += "\n".join(commands) + "\n"

        fd, path = tempfile.mkstemp(suffix=".sh", prefix=f"bench_{name}_")
        os.write(fd, content.encode())
        os.close(fd)
        os.chmod(path, 0o755)

        script_path = Path(path)
        self._temp_scripts.append(script_path)
        logger.debug(f"Created {name} script: {script_path}")
        for cmd in commands:
            logger.debug(f"  {cmd}")
        return script_path

    def _create_setup_script(self, tmp_datadir: Path) -> Path:
        """Create setup script (runs once before all timing runs)."""
        commands = [
            f'mkdir -p "{tmp_datadir}"',
            f'rm -rf "{tmp_datadir}"/*',
        ]
        return self._create_temp_script(commands, "setup")

    def _create_prepare_script(
        self, tmp_datadir: Path, original_datadir: Path | None
    ) -> Path:
        """Create prepare script (runs before each timing run)."""
        commands = [
            f'rm -rf "{tmp_datadir}"/*',
        ]

        # Copy datadir if provided (skip for fresh sync)
        if original_datadir:
            commands.append(f'cp -r "{original_datadir}"/* "{tmp_datadir}"')

        # Drop caches if available
        if self.capabilities.can_drop_caches and not self.config.no_cache_drop:
            commands.append(self.capabilities.drop_caches_path)

        # Clean debug logs
        commands.append(
            f'find "{tmp_datadir}" -name debug.log -delete 2>/dev/null || true'
        )

        return self._create_temp_script(commands, "prepare")

    def _create_cleanup_script(self, tmp_datadir: Path) -> Path:
        """Create cleanup script (runs after all timing runs for each command)."""
        commands = [
            f'rm -rf "{tmp_datadir}"/*',
        ]
        return self._create_temp_script(commands, "cleanup")

    def _build_bitcoind_cmd(
        self,
        binary: Path,
        tmp_datadir: Path,
    ) -> str:
        """Build the bitcoind command string for hyperfine."""
        if not self.benchmark_config:
            raise ValueError("benchmark_config is required")

        parts = []

        # Add flamegraph wrapper for instrumented mode
        if self.config.instrumented:
            parts.append("flamegraph")
            parts.append("--palette bitcoin")
            parts.append("--title 'bitcoind IBD'")
            parts.append("-c 'record -F 101 --call-graph fp'")
            parts.append("--")

        # Bitcoind command
        parts.append(str(binary))
        parts.append(f"-datadir={tmp_datadir}")

        # Add dbcache from matrix entry
        parts.append(f"-dbcache={self.config.dbcache}")

        # Add all bitcoind args from benchmark config
        for key, value in self.benchmark_config.bitcoind_args.items():
            formatted = self.benchmark_config._format_bitcoind_arg(key, value)
            if formatted:
                parts.append(formatted)

        # Debug flags for instrumented mode
        if self.config.instrumented and self.benchmark_config.instrumented_debug:
            for flag in self.benchmark_config.instrumented_debug:
                parts.append(f"-debug={flag}")

        return " ".join(parts)

    def _build_hyperfine_cmd(
        self,
        binaries: list[tuple[str, Path]],
        tmp_datadir: Path,
        results_file: Path,
        setup_script: Path,
        prepare_script: Path,
        cleanup_script: Path,
        output_dir: Path,
    ) -> list[str]:
        """Build the hyperfine command."""
        cmd = [
            "hyperfine",
            "--shell=bash",
            f"--setup={setup_script}",
            f"--prepare={prepare_script}",
            f"--cleanup={cleanup_script}",
            f"--runs={self.config.runs}",
            f"--export-json={results_file}",
            "--show-output",
        ]

        # Add command names and build commands
        for name, binary_path in binaries:
            cmd.append(f"--command-name={name}")

        # Build the actual commands to benchmark
        for name, binary_path in binaries:
            bitcoind_cmd = self._build_bitcoind_cmd(binary_path, tmp_datadir)

            # For instrumented runs, append the conclude logic to each command
            if self.config.instrumented:
                conclude = self._create_conclude_commands(name, tmp_datadir, output_dir)
                bitcoind_cmd += f" && {conclude}"

            cmd.append(bitcoind_cmd)

        return cmd

    def _create_conclude_commands(
        self,
        name: str,
        tmp_datadir: Path,
        output_dir: Path,
    ) -> str:
        """Create inline conclude commands for a specific binary."""
        # Return shell commands to run after each benchmark
        commands = []

        # Move flamegraph if exists
        commands.append(
            f'if [ -e flamegraph.svg ]; then mv flamegraph.svg "{output_dir}/{name}-flamegraph.svg"; fi'
        )

        # Copy debug log if exists
        commands.append(
            f'debug_log=$(find "{tmp_datadir}" -name debug.log -print -quit); '
            f'if [ -n "$debug_log" ]; then cp "$debug_log" "{output_dir}/{name}-debug.log"; fi'
        )

        return " && ".join(commands)
