#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""A dummy test to run an arbitrary extra command. (Expert use only)
"""
import subprocess

from test_framework.test_framework import BitcoinTestFramework


class RpcAuthTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 0  # No node/datadir needed

    def setup_network(self):
        pass

    def run_test(self):
        cmd = self.options.bash_cmd_extra_tests
        if cmd:
            self.log.info(f"Starting to run: {cmd}\n")
            subprocess.run(["bash", "-c", cmd], check=True)
            self.log.info(f"Command passed: {cmd}")
        else:
            self.log.info("No command to run.")


if __name__ == "__main__":
    RpcAuthTest(__file__).main()
