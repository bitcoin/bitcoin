#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-chainstate tool functionality

Test basic block processing via bitcoin-chainstate tool, including detecting
duplicates and malformed input.
"""

import subprocess

from test_framework.test_framework import BitcoinTestFramework

START_HEIGHT = 199


class BitcoinChainstateTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_bitcoin_chainstate()

    def set_test_params(self):
        """Use the pregenerated, deterministic chain up to height 199."""
        self.num_nodes = 2

    def setup_network(self):
        """Start with the nodes disconnected so that one can generate a block
        the other hasn't yet seen."""
        self.add_nodes(2)
        self.start_nodes()

    def add_block(self, datadir, input, *, expected_stderr=None, expected_stdout=None):
        proc = subprocess.Popen(
            self.get_binaries().chainstate_argv() + ["-regtest", datadir],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        stdout, stderr = proc.communicate(input=input + "\n", timeout=5 * self.options.timeout_factor)
        self.log.debug("STDOUT: {0}".format(stdout.strip("\n")))
        self.log.info("STDERR: {0}".format(stderr.strip("\n")))

        if expected_stderr is not None and expected_stderr not in stderr:
            raise AssertionError(f"Expected stderr output '{expected_stderr}' does not partially match stderr:\n{stderr}")
        if expected_stdout is not None and expected_stdout not in stdout:
            raise AssertionError(f"Expected stdout output '{expected_stdout}' does not partially match stdout:\n{stdout}")

        return stdout

    def basic_test(self):
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        datadir = n1.chain_path
        n1.stop_node()
        block = n0.getblock(n0.getblockhash(START_HEIGHT+1), 0)
        self.log.info(f"Test bitcoin-chainstate {self.get_binaries().chainstate_argv()} with datadir: {datadir}")
        stdout = self.add_block(datadir, block, expected_stderr="Block has not yet been rejected", expected_stdout="Valid block")
        assert "Block tip changed" in stdout, stdout
        self.add_block(datadir, block, expected_stderr="duplicate")
        self.add_block(datadir, "00", expected_stderr="Block decode failed")
        self.add_block(datadir, "", expected_stderr="Empty line found")

    def run_test(self):
        self.generate(self.nodes[0], nblocks=1, sync_fun=self.no_op)
        self.basic_test()

if __name__ == "__main__":
    BitcoinChainstateTest(__file__).main()
