#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

"""Run tests against an extracted Guix release package."""

import configparser
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    workspace = Path.cwd()
    packages = list((workspace / "guix-test-package").glob("bitcoin-*.tar.gz"))
    packages += list((workspace / "guix-test-package").glob("bitcoin-*.zip"))
    if len(packages) != 1:
        raise RuntimeError(f"Expected one Guix package, found {packages}")

    extracted = workspace / "guix-test-package" / "extracted"
    shutil.unpack_archive(str(packages[0]), extracted)
    roots = list(extracted.glob("bitcoin-*"))
    if len(roots) != 1:
        raise RuntimeError(f"Expected one package directory, found {roots}")
    package = roots[0]
    extension = ".exe" if sys.platform == "win32" else ""

    if sys.platform == "darwin":
        # Apple Silicon requires signatures. Sign the extracted copy only.
        for directory in (package / "bin", package / "libexec"):
            for binary in directory.iterdir():
                if binary.is_file() and os.access(binary, os.X_OK):
                    if subprocess.run(["codesign", "--verify", str(binary)], capture_output=True).returncode != 0:
                        subprocess.run(["codesign", "--sign", "-", str(binary)], check=True)

    config = configparser.ConfigParser()
    with (workspace / "guix-test-package" / "config.ini").open(encoding="utf-8") as file:
        config.read_file(file)
    config["environment"]["SRCDIR"] = str(workspace)
    config["environment"]["BUILDDIR"] = str(workspace)
    config["environment"]["RPCAUTH"] = str(workspace / "share" / "rpcauth" / "rpcauth.py")
    with (workspace / "test" / "config.ini").open("w", encoding="utf-8") as file:
        config.write(file)

    env = os.environ.copy()
    binaries = {
        "BITCOIN_BIN": "bitcoin",
        "BITCOIND": "bitcoind",
        "BITCOINCLI": "bitcoin-cli",
        "BITCOINUTIL": "bitcoin-util",
        "BITCOINTX": "bitcoin-tx",
        "BITCOINWALLET": "bitcoin-wallet",
    }
    for variable, name in binaries.items():
        env[variable] = str(package / "bin" / f"{name}{extension}")
    env["PATH"] = os.pathsep.join([str(package / "bin"), env["PATH"]])

    sys.path.insert(0, str(workspace / "test"))
    from download_utils import download_script_assets

    unit_data = workspace / "unit_test_data"
    download_script_assets(unit_data)
    env["DIR_UNIT_TEST_DATA"] = str(unit_data)
    subprocess.run([str(package / "libexec" / f"test_bitcoin{extension}"), "-l", "test_suite"], env=env, check=True)

    subprocess.run([sys.executable, str(workspace / "test" / "get_previous_releases.py")], check=True)
    functional_command = [
        sys.executable,
        str(workspace / "test" / "functional" / "test_runner.py"),
        f"--jobs={os.cpu_count() or 1}",
        "--quiet",
        "--failfast",
        "--timeout-factor=40" if sys.platform == "win32" else "--timeout-factor=1",
    ]
    if sys.platform == "win32":
        # Old releases cannot read the Unicode test runner directory on Windows.
        for name in ("feature_unsupported_utxo_db", "wallet_ancient_migration"):
            functional_command += ["--exclude", name]
    subprocess.run(functional_command, env=env, check=True)

    if sys.platform == "win32":
        for name in ("feature_unsupported_utxo_db", "wallet_ancient_migration"):
            subprocess.run([
                sys.executable,
                str(workspace / "test" / "functional" / f"{name}.py"),
                "--previous-releases",
                f"--tmpdir={workspace / f'test_{name}'}",
            ], env=env, check=True)


if __name__ == "__main__":
    main()
