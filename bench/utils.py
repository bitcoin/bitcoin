"""Utility functions for git operations."""

from __future__ import annotations

import logging
import subprocess
from pathlib import Path

logger = logging.getLogger(__name__)


class GitState:
    """Saved git state for restoration after operations."""

    def __init__(self, repo_path: Path | None = None):
        self.repo_path = repo_path or Path.cwd()
        self.original_branch: str | None = None
        self.original_commit: str | None = None
        self.was_detached: bool = False

    def save(self) -> None:
        """Save current git state."""
        # Check if we're on a branch or detached HEAD
        result = subprocess.run(
            ["git", "symbolic-ref", "--short", "HEAD"],
            capture_output=True,
            text=True,
            cwd=self.repo_path,
        )

        if result.returncode == 0:
            self.original_branch = result.stdout.strip()
            self.was_detached = False
        else:
            # Detached HEAD - save commit hash
            result = subprocess.run(
                ["git", "rev-parse", "HEAD"],
                capture_output=True,
                text=True,
                check=True,
                cwd=self.repo_path,
            )
            self.original_commit = result.stdout.strip()
            self.was_detached = True

        logger.debug(
            f"Saved git state: branch={self.original_branch}, "
            f"commit={self.original_commit}, detached={self.was_detached}"
        )

    def restore(self) -> None:
        """Restore saved git state."""
        if self.original_branch:
            logger.debug(f"Restoring branch: {self.original_branch}")
            subprocess.run(
                ["git", "checkout", self.original_branch],
                check=True,
                cwd=self.repo_path,
            )
        elif self.original_commit:
            logger.debug(f"Restoring detached HEAD: {self.original_commit}")
            subprocess.run(
                ["git", "checkout", self.original_commit],
                check=True,
                cwd=self.repo_path,
            )


class GitError(Exception):
    """Git operation failed."""

    pass


def git_checkout(commit: str, repo_path: Path | None = None) -> None:
    """Checkout a specific commit."""
    repo_path = repo_path or Path.cwd()
    logger.info(f"Checking out {commit[:12]}")

    result = subprocess.run(
        ["git", "checkout", commit],
        cwd=repo_path,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        raise GitError(f"Failed to checkout {commit}: {result.stderr}")


def git_rev_parse(ref: str, repo_path: Path | None = None) -> str:
    """Resolve a git reference to a full commit hash."""
    repo_path = repo_path or Path.cwd()

    result = subprocess.run(
        ["git", "rev-parse", ref],
        cwd=repo_path,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        raise GitError(f"Failed to resolve {ref}: {result.stderr}")

    return result.stdout.strip()
