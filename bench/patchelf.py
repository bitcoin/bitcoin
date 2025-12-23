"""Patchelf utilities for fixing guix-built binaries on NixOS."""

from __future__ import annotations

import logging
import os
import subprocess
from pathlib import Path

logger = logging.getLogger(__name__)


def get_nix_interpreter() -> str | None:
    """Get the path to the nix store's dynamic linker.

    Returns None if not on NixOS or can't find it.
    """
    # Check if we're on NixOS
    if not Path("/etc/NIXOS").exists():
        return None

    # Find the interpreter from the current glibc
    # We can get this by checking what the current shell uses
    try:
        result = subprocess.run(
            ["patchelf", "--print-interpreter", "/bin/sh"],
            capture_output=True,
            text=True,
        )
        if result.returncode == 0:
            interp = result.stdout.strip()
            if interp and Path(interp).exists():
                return interp
    except FileNotFoundError:
        pass

    return None


def get_binary_interpreter(binary: Path) -> str | None:
    """Get the interpreter (dynamic linker) of a binary."""
    try:
        result = subprocess.run(
            ["patchelf", "--print-interpreter", str(binary)],
            capture_output=True,
            text=True,
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except FileNotFoundError:
        logger.debug("patchelf not found")
    return None


def needs_patching(binary: Path) -> bool:
    """Check if a binary needs to be patched for NixOS.

    Returns True if:
    - We're on NixOS
    - The binary has a non-nix interpreter (e.g., /lib64/ld-linux-x86-64.so.2)
    """
    nix_interp = get_nix_interpreter()
    if not nix_interp:
        # Not on NixOS, no patching needed
        return False

    binary_interp = get_binary_interpreter(binary)
    if not binary_interp:
        # Can't determine interpreter, assume no patching needed
        return False

    # Check if the binary's interpreter is already in the nix store
    if binary_interp.startswith("/nix/store/"):
        return False

    # Binary uses a non-nix interpreter (e.g., /lib64/...)
    return True


def patch_binary(binary: Path) -> bool:
    """Patch a binary to use the nix store's dynamic linker.

    Returns True if patching was successful or not needed.
    """
    if not needs_patching(binary):
        logger.debug(f"Binary {binary} does not need patching")
        return True

    nix_interp = get_nix_interpreter()
    if not nix_interp:
        logger.warning("Cannot patch binary: unable to find nix interpreter")
        return False

    original_interp = get_binary_interpreter(binary)
    logger.info(f"Patching binary: {binary}")
    logger.info(f"  Original interpreter: {original_interp}")
    logger.info(f"  New interpreter: {nix_interp}")

    # Make sure binary is writable
    try:
        os.chmod(binary, 0o755)
    except OSError as e:
        logger.warning(f"Could not make binary writable: {e}")

    try:
        result = subprocess.run(
            ["patchelf", "--set-interpreter", nix_interp, str(binary)],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            logger.error(f"patchelf failed: {result.stderr}")
            return False
        logger.info("  Patching successful")
        return True
    except FileNotFoundError:
        logger.error("patchelf not found - install it or use nix develop")
        return False


def ensure_binary_runnable(binary: Path) -> bool:
    """Ensure a binary can run on this system.

    Patches the binary if necessary (on NixOS with non-nix binaries).
    Returns True if the binary should be runnable.
    """
    if not binary.exists():
        logger.error(f"Binary not found: {binary}")
        return False

    # Check if patching is needed and do it
    if needs_patching(binary):
        return patch_binary(binary)

    return True
