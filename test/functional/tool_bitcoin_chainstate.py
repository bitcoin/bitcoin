#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoin-chainstate tool functionality

Test basic block processing via bitcoin-chainstate tool, including detecting
duplicates and malformed input.

Test that bitcoin-chainstate can load a datadir initialized with an assumeutxo
snapshot and extend the snapshot chain with new blocks.
"""

import subprocess

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet

START_HEIGHT = 199
# Hardcoded in regtest chainparams
SNAPSHOT_BASE_BLOCK_HEIGHT = 299
SNAPSHOT_BASE_BLOCK_HASH = "7cc695046fec709f8c9394b6f928f81e81fd3ac20977bb68760fa1faa7916ea2"


class BitcoinChainstateTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_bitcoin_chainstate()

    def set_test_params(self):
        """Use the pregenerated, deterministic chain up to height 199."""
        self.num_nodes = 2

    def setup_network(self):
        """Start with the nodes disconnected so that one can generate a snapshot
        including blocks the other hasn't yet seen."""
        self.add_nodes(2)
        self.start_nodes()

    def generate_snapshot_chain(self):
        self.log.info(f"Generate deterministic chain up to block {SNAPSHOT_BASE_BLOCK_HEIGHT} for node0 while node1 disconnected")
        n0 = self.nodes[0]
        assert_equal(n0.getblockcount(), START_HEIGHT)
        n0.setmocktime(n0.getblockheader(n0.getbestblockhash())['time'])
        mini_wallet = MiniWallet(n0)
        for i in range(SNAPSHOT_BASE_BLOCK_HEIGHT - n0.getblockchaininfo()["blocks"]):
            if i % 3 == 0:
                mini_wallet.send_self_transfer(from_node=n0)
            self.generate(n0, nblocks=1, sync_fun=self.no_op)
        assert_equal(n0.getblockcount(), SNAPSHOT_BASE_BLOCK_HEIGHT)
        assert_equal(n0.getbestblockhash(), SNAPSHOT_BASE_BLOCK_HASH)
        return n0.dumptxoutset('utxos.dat', "latest")

    def add_block(self, datadir, input, expected_stderr=None, expected_stdout=None):
        proc = subprocess.Popen(
            self.get_binaries().chainstate_argv() + ["-regtest", datadir],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        stdout, stderr = proc.communicate(input=input + "\n", timeout=5)
        self.log.debug("STDOUT: {0}".format(stdout.strip("\n")))
        self.log.info("STDERR: {0}".format(stderr.strip("\n")))

        if expected_stderr is not None and expected_stderr not in stderr:
            raise AssertionError(f"Expected stderr output '{expected_stderr}' does not partially match stderr:\n{stderr}")
        if expected_stdout is not None and expected_stdout not in stdout:
            raise AssertionError(f"Expected stdout output '{expected_stdout}' does not partially match stdout:\n{stdout}")

    def basic_test(self):
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        datadir = n1.chain_path
        n1.stop_node()
        block = n0.getblock(n0.getblockhash(START_HEIGHT+1), 0)
        self.log.info(f"Test bitcoin-chainstate {self.get_binaries().chainstate_argv()} with datadir: {datadir}")
        self.add_block(datadir, block, expected_stderr="Block has not yet been rejected")
        self.add_block(datadir, block, expected_stderr="duplicate")
        self.add_block(datadir, "00", expected_stderr="Block decode failed")
        self.add_block(datadir, "", expected_stderr="Empty line found")

    def assumeutxo_test(self, dump_output_path):
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        self.start_node(1)
        self.log.info("Submit headers for new blocks to node1, then load the snapshot so it activates")
        for height in range(START_HEIGHT+2, SNAPSHOT_BASE_BLOCK_HEIGHT+1):
            block = n0.getblock(n0.getblockhash(height), 0)
            n1.submitheader(block)
        assert_equal(n1.getblockcount(), START_HEIGHT+1)
        loaded = n1.loadtxoutset(dump_output_path)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_BLOCK_HEIGHT)
        datadir = n1.chain_path
        n1.stop_node()
        self.log.info(f"Test bitcoin-chainstate {self.get_binaries().chainstate_argv()} with an assumeutxo datadir: {datadir}")
        new_tip_hash = self.generate(n0, nblocks=1, sync_fun=self.no_op)[0]
        self.add_block(datadir, n0.getblock(new_tip_hash, 0), expected_stdout="Block tip changed")

    def run_test(self):
        dump_output = self.generate_snapshot_chain()
        self.basic_test()
        self.assumeutxo_test(dump_output['path'])

if __name__ == "__main__":
    BitcoinChainstateTest(__file__).main()
