#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Monitor block intervals and scan staking logs for kernel failures."""

import json
import subprocess
from pathlib import Path

THRESHOLD_SECONDS = 8 * 60  # eight minutes


def rpc(*args: str) -> bytes:
    """Invoke bitcoin-cli and return stdout."""
    return subprocess.check_output(["bitcoin-cli", *args])


def get_block_time(height: int) -> int:
    block_hash = rpc("getblockhash", str(height)).decode().strip()
    header = json.loads(rpc("getblockheader", block_hash).decode())
    return header["time"]


def check_interval() -> None:
    height = int(rpc("getblockcount").strip())
    if height < 1:
        print("Not enough blocks to compute interval.")
        return
    t_prev = get_block_time(height - 1)
    t_curr = get_block_time(height)
    interval = t_curr - t_prev
    print(f"Block interval: {interval} seconds")
    if interval > THRESHOLD_SECONDS:
        print("Interval exceeds eight minutes; scanning debug.log for kernel failures...")
        log_path = Path.home() / ".bitcoin" / "debug.log"
        if log_path.exists():
            with log_path.open("r", encoding="utf8") as fh:
                kernel_lines = [line.strip() for line in fh if "kernel" in line.lower()]
            for line in kernel_lines[-10:]:
                print(line)
        else:
            print(f"No log file found at {log_path}")


if __name__ == "__main__":
    check_interval()
