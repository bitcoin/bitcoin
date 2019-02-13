#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests runCommandParseJSON via the runcommand regtest RPC."""

import os
import platform

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class RunCommandInterfaceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_runcommand(self):
        self.skip_if_no_runcommand(self.nodes[0])

        self.log.info("Testing runcommand...")
        command_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks/command.py')
        if platform.system() == "Windows":
            command_path = "python " + command_path

        self.log.info('bitcoin-cli runcommand %s ...' % command_path)

        res = self.nodes[0].runcommand(command_path + " go --success")
        assert_equal(res, {"success": True})

        res = self.nodes[0].runcommand(command_path + " go --fail")
        assert_equal(res, {"success": False, "error": "reason"})

        assert_raises_rpc_error(-1, "a", self.nodes[0].runcommand, command_path + " go --invalid")

        res = self.nodes[0].runcommand(command_path + " go --success", "hello")
        assert_equal(res, {"success": True, "stdin": "hello"})

    def run_test(self):
        self.test_runcommand()

if __name__ == '__main__':
    RunCommandInterfaceTest().main()
