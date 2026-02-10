#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import argparse
import os
import shlex
import subprocess
import sys
from pathlib import Path


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(str(e))


GENERATE_OPTIONS = {
    "standard": [
        "-DBUILD_BENCH=ON",
        "-DBUILD_KERNEL_LIB=ON",
        "-DBUILD_UTIL_CHAINSTATE=ON",
        "-DCMAKE_COMPILE_WARNING_AS_ERROR=ON",
    ],
    "fuzz": [
        "-DVCPKG_MANIFEST_NO_DEFAULT_FEATURES=ON",
        "-DVCPKG_MANIFEST_FEATURES=wallet",
        "-DBUILD_GUI=OFF",
        "-DWITH_ZMQ=OFF",
        "-DBUILD_FOR_FUZZING=ON",
        "-DCMAKE_COMPILE_WARNING_AS_ERROR=ON",
    ],
}


def generate(ci_type):
    command = [
        "cmake",
        "-B",
        "build",
        "-Werror=dev",
        "--preset",
        "vs2022",
    ] + GENERATE_OPTIONS[ci_type]
    run(command)


def build():
    command = [
        "cmake",
        "--build",
        "build",
        "--config",
        "Release",
    ]
    if run(command + ["-j", str(os.process_cpu_count())], check=False).returncode != 0:
        print("Build failure. Verbose build follows.")
        run(command + ["-j1", "--verbose"])


def check_manifests(ci_type):
    if ci_type != "standard":
        print(f"Skipping manifest validation for '{ci_type}' ci type.")
        return

    release_dir = Path.cwd() / "build" / "bin" / "Release"
    manifest_path = release_dir / "bitcoind.manifest"
    cmd_bitcoind_manifest = [
        "mt.exe",
        "-nologo",
        f"-inputresource:{release_dir / 'bitcoind.exe'}",
        f"-out:{manifest_path}",
    ]
    run(cmd_bitcoind_manifest)
    print(manifest_path.read_text())

    skips = {  # Skip as they currently do not have manifests
        "fuzz.exe",
        "bench_bitcoin.exe",
        "test_bitcoin-qt.exe",
        "test_kernel.exe",
        "bitcoin-chainstate.exe",
    }
    for entry in release_dir.iterdir():
        if entry.suffix.lower() != ".exe":
            continue
        if entry.name in skips:
            print(f"Skipping {entry.name} (no manifest present)")
            continue
        print(f"Checking {entry.name}")
        cmd_check_manifest = [
            "mt.exe",
            "-nologo",
            f"-inputresource:{entry}",
            "-validate_manifest",
        ]
        run(cmd_check_manifest)


def prepare_tests(ci_type):
    if ci_type == "standard":
        run([sys.executable, "-m", "pip", "install", "pyzmq"])
    elif ci_type == "fuzz":
        repo_dir = os.path.join(os.getcwd(), "qa-assets")
        clone_cmd = [
            "git",
            "clone",
            "--depth=1",
            "https://github.com/bitcoin-core/qa-assets",
            repo_dir,
        ]
        run(clone_cmd)
        print("Using qa-assets repo from commit ...")
        run(["git", "-C", repo_dir, "log", "-1"])


def run_tests(ci_type):
    build_dir = "build"
    num_procs = str(os.process_cpu_count())
    release_bin = os.path.join(os.getcwd(), build_dir, "bin", "Release")

    if ci_type == "standard":
        test_envs = {
            "BITCOIN_BIN": "bitcoin.exe",
            "BITCOIND": "bitcoind.exe",
            "BITCOINCLI": "bitcoin-cli.exe",
            "BITCOIN_BENCH": "bench_bitcoin.exe",
            "BITCOINTX": "bitcoin-tx.exe",
            "BITCOINUTIL": "bitcoin-util.exe",
            "BITCOINWALLET": "bitcoin-wallet.exe",
            "BITCOINCHAINSTATE": "bitcoin-chainstate.exe",
        }
        for var, exe in test_envs.items():
            os.environ[var] = os.path.join(release_bin, exe)

        ctest_cmd = [
            "ctest",
            "--test-dir",
            build_dir,
            "--output-on-failure",
            "--stop-on-failure",
            "-j",
            num_procs,
            "--build-config",
            "Release",
        ]
        run(ctest_cmd)

        test_cmd = [
            sys.executable,
            os.path.join(build_dir, "test", "functional", "test_runner.py"),
            "--jobs",
            num_procs,
            "--quiet",
            f"--tmpdirprefix={os.getcwd()}",
            "--combinedlogslen=99999999",
            *shlex.split(os.environ.get("TEST_RUNNER_EXTRA", "").strip()),
        ]
        run(test_cmd)

    elif ci_type == "fuzz":
        os.environ["BITCOINFUZZ"] = os.path.join(release_bin, "fuzz.exe")
        fuzz_cmd = [
            sys.executable,
            os.path.join(build_dir, "test", "fuzz", "test_runner.py"),
            "--par",
            num_procs,
            "--loglevel",
            "DEBUG",
            os.path.join(os.getcwd(), "qa-assets", "fuzz_corpora"),
        ]
        run(fuzz_cmd)


def main():
    parser = argparse.ArgumentParser(description="Utility to run Windows CI steps.")
    parser.add_argument("ci_type", choices=GENERATE_OPTIONS, help="CI type to run.")
    steps = [
        "generate",
        "build",
        "check_manifests",
        "prepare_tests",
        "run_tests",
    ]
    parser.add_argument("step", choices=steps, help="CI step to perform.")
    args = parser.parse_args()

    if args.step == "generate":
        generate(args.ci_type)
    elif args.step == "build":
        build()
    elif args.step == "check_manifests":
        check_manifests(args.ci_type)
    elif args.step == "prepare_tests":
        prepare_tests(args.ci_type)
    elif args.step == "run_tests":
        run_tests(args.ci_type)


if __name__ == "__main__":
    main()
