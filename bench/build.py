"""Build phase - compile bitcoind at a specified commit."""

from __future__ import annotations

import logging
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .capabilities import Capabilities
    from .config import Config

from .utils import GitState, git_checkout, git_rev_parse

logger = logging.getLogger(__name__)


@dataclass
class BuiltBinary:
    """A built binary."""

    name: str
    path: Path
    commit: str


@dataclass
class BuildResult:
    """Result of the build phase."""

    binary: BuiltBinary


def parse_commit_spec(spec: str) -> tuple[str, str | None]:
    """Parse a commit spec like 'abc123:name' or 'abc123'.

    Returns (commit, name) where name may be None.
    """
    if ":" in spec:
        commit, name = spec.split(":", 1)
        return commit, name
    return spec, None


class BuildPhase:
    """Build bitcoind binary at a specified commit."""

    def __init__(
        self,
        config: Config,
        capabilities: Capabilities,
        repo_path: Path | None = None,
    ):
        self.config = config
        self.capabilities = capabilities
        self.repo_path = repo_path or Path.cwd()

    def run(
        self,
        commit_spec: str,
        output_dir: Path | None = None,
    ) -> BuildResult:
        """Build bitcoind at given commit.

        Args:
            commit_spec: Commit spec like 'abc123:name' or 'abc123'
            output_dir: Where to store binary (default: ./binaries)

        Returns:
            BuildResult with the built binary
        """
        # Check prerequisites
        errors = self.capabilities.check_for_build()
        if errors:
            raise RuntimeError("Build prerequisites not met:\n" + "\n".join(errors))

        output_dir = output_dir or Path(self.config.binaries_dir)

        # Parse commit spec and resolve to full hash
        commit, name = parse_commit_spec(commit_spec)
        commit_hash = git_rev_parse(commit, self.repo_path)

        # Default name to short hash if not provided
        if name is None:
            name = commit_hash[:12]

        logger.info(f"Building binary: {name} ({commit_hash[:12]})")
        logger.info(f"  Repo: {self.repo_path}")
        logger.info(f"  Output: {output_dir}")

        # Setup output path
        binary_dir = output_dir / name
        binary_dir.mkdir(parents=True, exist_ok=True)
        binary_path = binary_dir / "bitcoind"

        # Check if we can skip existing build
        if self.config.skip_existing and binary_path.exists():
            logger.info(f"  Skipping {name} - binary exists")
            return BuildResult(
                binary=BuiltBinary(name=name, path=binary_path, commit=commit_hash)
            )

        # Save git state for restoration
        git_state = GitState(self.repo_path)
        git_state.save()

        try:
            self._build_commit(name, commit_hash, binary_path)
        finally:
            # Always restore git state
            git_state.restore()

        return BuildResult(
            binary=BuiltBinary(name=name, path=binary_path, commit=commit_hash)
        )

    def _build_commit(self, name: str, commit: str, output_path: Path) -> None:
        """Build bitcoind for a commit."""
        logger.info(f"Building {name} ({commit[:12]})")

        if self.config.dry_run:
            logger.info(f"  [DRY RUN] Would build {commit[:12]} -> {output_path}")
            return

        # Checkout the commit
        logger.info(f"  Checking out {commit[:12]}...")
        git_checkout(commit, self.repo_path)

        # Build with nix
        cmd = ["nix", "build", "-L"]

        logger.info(f"  Running: {' '.join(cmd)}")
        logger.info(f"  Working directory: {self.repo_path}")
        result = subprocess.run(
            cmd,
            cwd=self.repo_path,
        )

        if result.returncode != 0:
            raise RuntimeError(f"Build failed for {name} ({commit[:12]})")

        # Copy binary to output location
        nix_binary = self.repo_path / "result" / "bin" / "bitcoind"
        if not nix_binary.exists():
            raise RuntimeError(f"Built binary not found at {nix_binary}")

        logger.info(f"  Copying {nix_binary} -> {output_path}")

        # Remove existing binary if present (may be read-only from nix)
        if output_path.exists():
            output_path.chmod(0o755)
            output_path.unlink()

        shutil.copy2(nix_binary, output_path)
        output_path.chmod(0o755)  # Ensure it's executable and writable
        logger.info(f"  Built {name} binary: {output_path}")

        # Clean up nix result symlink
        result_link = self.repo_path / "result"
        if result_link.is_symlink():
            logger.debug(f"  Removing nix result symlink: {result_link}")
            result_link.unlink()
