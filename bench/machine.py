"""Machine specification detection for benchmark context."""

from __future__ import annotations

import logging
import subprocess
from dataclasses import dataclass
from typing import Any

logger = logging.getLogger(__name__)


@dataclass
class MachineSpecs:
    """Machine hardware specifications."""

    cpu_model: str
    architecture: str
    cpu_cores: int
    disk_type: str
    os_kernel: str
    total_ram_gb: float

    def to_dict(self) -> dict[str, Any]:
        """Convert to dictionary for JSON serialization."""
        return {
            "cpu_model": self.cpu_model,
            "architecture": self.architecture,
            "cpu_cores": self.cpu_cores,
            "disk_type": self.disk_type,
            "os_kernel": self.os_kernel,
            "total_ram_gb": self.total_ram_gb,
        }

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> MachineSpecs:
        """Create from dictionary."""
        return cls(
            cpu_model=data.get("cpu_model", "Unknown"),
            architecture=data.get("architecture", "Unknown"),
            cpu_cores=data.get("cpu_cores", 0),
            disk_type=data.get("disk_type", "Unknown"),
            os_kernel=data.get("os_kernel", "Unknown"),
            total_ram_gb=data.get("total_ram_gb", 0.0),
        )

    def get_machine_id(self) -> str:
        """Get short machine identifier from architecture.

        Returns:
            Short ID like "amd64" or "arm64"
        """
        return get_machine_id(self.architecture)


def _run_command(cmd: list[str]) -> str:
    """Run a command and return stdout, or empty string on failure."""
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
        return result.stdout.strip()
    except (subprocess.TimeoutExpired, FileNotFoundError, OSError) as e:
        logger.debug(f"Command {cmd} failed: {e}")
        return ""


def _get_cpu_info() -> tuple[str, str, int]:
    """Get CPU model, architecture, and core count from lscpu."""
    output = _run_command(["lscpu"])

    cpu_model = "Unknown"
    architecture = "Unknown"
    cpu_cores = 0

    for line in output.split("\n"):
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        key = key.strip()
        value = value.strip()

        if key == "Model name":
            cpu_model = value
        elif key == "Architecture":
            architecture = value
        elif key == "CPU(s)":
            try:
                cpu_cores = int(value)
            except ValueError:
                pass

    # Fallback for architecture
    if architecture == "Unknown":
        architecture = _run_command(["uname", "-m"]) or "Unknown"

    return cpu_model, architecture, cpu_cores


def _get_os_kernel() -> str:
    """Get the OS kernel version from uname -r."""
    kernel = _run_command(["uname", "-r"])
    return kernel if kernel else "Unknown"


def _get_total_ram_gb() -> float:
    """Get total RAM in GB from /proc/meminfo."""
    try:
        with open("/proc/meminfo") as f:
            for line in f:
                if line.startswith("MemTotal:"):
                    # Format: "MemTotal:       16384000 kB"
                    parts = line.split()
                    if len(parts) >= 2:
                        kb = int(parts[1])
                        return round(kb / (1024 * 1024), 1)  # Convert kB to GB
    except (OSError, ValueError) as e:
        logger.debug(f"Failed to read /proc/meminfo: {e}")
    return 0.0


def _get_disk_type() -> str:
    """Get the fastest disk type on the system.

    Priority: NVMe > SATA SSD > HDD
    Uses lsblk to check ROTA (rotational) flag: 0 = SSD/NVMe, 1 = HDD
    """
    output = _run_command(["lsblk", "-d", "-o", "NAME,ROTA,MODEL", "-n"])

    has_nvme = False
    has_ssd = False
    has_hdd = False

    for line in output.split("\n"):
        if not line.strip():
            continue

        parts = line.split()
        if len(parts) < 2:
            continue

        name = parts[0]
        try:
            rota = int(parts[1])
        except (ValueError, IndexError):
            continue

        if name.startswith("nvme"):
            has_nvme = True
        elif rota == 0:
            has_ssd = True
        elif rota == 1:
            has_hdd = True

    if has_nvme:
        return "NVMe SSD"
    elif has_ssd:
        return "SATA SSD"
    elif has_hdd:
        return "HDD"
    else:
        return "Unknown"


def get_machine_specs() -> MachineSpecs:
    """Detect and return current machine specifications."""
    cpu_model, architecture, cpu_cores = _get_cpu_info()
    disk_type = _get_disk_type()
    os_kernel = _get_os_kernel()
    total_ram_gb = _get_total_ram_gb()

    specs = MachineSpecs(
        cpu_model=cpu_model,
        architecture=architecture,
        cpu_cores=cpu_cores,
        disk_type=disk_type,
        os_kernel=os_kernel,
        total_ram_gb=total_ram_gb,
    )

    logger.info(
        f"Detected machine: {cpu_model} ({architecture}, {cpu_cores} cores, "
        f"{total_ram_gb}GB RAM, {disk_type}, {os_kernel})"
    )
    return specs


# Architecture to short ID mapping
ARCH_TO_ID = {
    "x86_64": "amd64",
    "amd64": "amd64",
    "aarch64": "arm64",
    "arm64": "arm64",
}


def get_machine_id(architecture: str | None = None) -> str:
    """Get short machine identifier from architecture.

    Args:
        architecture: Architecture string (e.g., "x86_64", "aarch64").
                      If None, auto-detect from system.

    Returns:
        Short ID like "amd64" or "arm64"
    """
    if architecture is None:
        architecture = _run_command(["uname", "-m"]) or "unknown"

    return ARCH_TO_ID.get(architecture.lower(), architecture.lower())
