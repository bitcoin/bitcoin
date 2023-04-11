#!/usr/bin/env python3

import json
import sys
import subprocess
from pathlib import Path


def main():
    """Tests ordered roughly from faster to slower."""
    expect_code(run_verify("", "pub", '0.32'), 4, "Nonexistent version should fail")
    expect_code(run_verify("", "pub", '0.32.awefa.12f9h'), 11, "Malformed version should fail")
    expect_code(run_verify('--min-good-sigs 20', "pub", "22.0"), 9, "--min-good-sigs 20 should fail")

    print("- testing verification (22.0)", flush=True)
    _220 = run_verify("--json", "pub", "22.0")
    try:
        result = json.loads(_220.stdout.decode())
    except Exception:
        print("failed on 22.0 --json:")
        print_process_failure(_220)
        raise

    expect_code(_220, 0, "22.0 should succeed")
    v = result['verified_binaries']
    assert result['good_trusted_sigs']
    assert v['bitcoin-22.0-aarch64-linux-gnu.tar.gz'] == 'ac718fed08570a81b3587587872ad85a25173afa5f9fbbd0c03ba4d1714cfa3e'
    assert v['bitcoin-22.0-osx64.tar.gz'] == '2744d199c3343b2d94faffdfb2c94d75a630ba27301a70e47b0ad30a7e0155e9'
    assert v['bitcoin-22.0-x86_64-linux-gnu.tar.gz'] == '59ebd25dd82a51638b7a6bb914586201e67db67b919b2a1ff08925a7936d1b16'


def run_verify(global_args: str, command: str, command_args: str) -> subprocess.CompletedProcess:
    maybe_here = Path.cwd() / 'verify.py'
    path = maybe_here if maybe_here.exists() else Path.cwd() / 'contrib' / 'verify-binaries' / 'verify.py'

    if command == "pub":
        command += " --cleanup"

    return subprocess.run(
        f"{path} {global_args} {command} {command_args}",
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)


def expect_code(completed: subprocess.CompletedProcess, expected_code: int, msg: str):
    if completed.returncode != expected_code:
        print(f"{msg!r} failed: got code {completed.returncode}, expected {expected_code}")
        print_process_failure(completed)
        sys.exit(1)
    else:
        print(f"✓ {msg!r} passed")


def print_process_failure(completed: subprocess.CompletedProcess):
    print(f"stdout:\n{completed.stdout.decode()}")
    print(f"stderr:\n{completed.stderr.decode()}")


if __name__ == '__main__':
    main()
