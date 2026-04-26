#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Special script to run each bench sanity check
"""
import shlex
import subprocess

from test_framework.test_framework import BitcoinTestFramework


class BenchSanityCheck(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0  # No node/datadir needed

    def setup_network(self):
        pass

    def skip_test_if_missing_module(self):
        self.skip_if_no_bitcoin_bench()

    def add_options(self, parser):
        parser.add_argument(
            "--bench",
            default=".*",
            help="Regex to filter the bench to run (default=%(default)s)",
        )

    def run_test(self):
        cmd = self.get_binaries().bench_argv() + [
            f"-filter={self.options.bench}",
            "-sanity-check",
        ]
        self.log.info(f"Starting: {shlex.join(cmd)}")
        subprocess.run(cmd, check=True)
        self.log.info("Success!")


if __name__ == "__main__":
    BenchSanityCheck(__file__).main()
