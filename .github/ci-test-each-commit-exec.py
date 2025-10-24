#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import subprocess
import sys
import shlex


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except Exception as e:
        sys.exit(e)


def main():
    print("Running tests on commit ...")
    run(["git", "log", "-1"])

    num_procs = int(run(["nproc"], stdout=subprocess.PIPE).stdout)

    run([
        "cmake",
        "-B",
        "build",
        "-Werror=dev",
        # Use clang++, because it is a bit faster and uses less memory than g++
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        # Use mold, because it is faster than the default linker
        "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold",
        # Use Debug build type for more debug checks, but enable optimizations
        "-DAPPEND_CXXFLAGS='-O3 -g2'",
        "-DAPPEND_CFLAGS='-O3 -g2'",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DWERROR=ON",
        "-DWITH_ZMQ=ON",
        "-DBUILD_GUI=ON",
        "-DBUILD_BENCH=ON",
        "-DBUILD_FUZZ_BINARY=ON",
        "-DWITH_USDT=ON",
        "-DBUILD_KERNEL_LIB=ON",
        "-DBUILD_KERNEL_TEST=ON",
        "-DCMAKE_CXX_FLAGS=-Wno-error=unused-member-function",
    ])
    run(["cmake", "--build", "build", "-j", str(num_procs)])
    run([
        "ctest",
        "--output-on-failure",
        "--stop-on-failure",
        "--test-dir",
        "build",
        "-j",
        str(num_procs),
    ])
    run([
        sys.executable,
        "./build/test/functional/test_runner.py",
        "-j",
        str(num_procs * 2),
        "--combinedlogslen=99999999",
    ])


if __name__ == "__main__":
    main()
