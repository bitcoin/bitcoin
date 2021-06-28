#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify that starting dashd with -h works as expected."""
import subprocess

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class HelpTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        # Don't start the node

    def run_test(self):
        self.log.info("Start dashd with -h for help text")
        self.nodes[0].start(extra_args=['-h'], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        # Node should exit immediately and output help to stdout.
        ret_code = self.nodes[0].process.wait(timeout=1)
        assert_equal(ret_code, 0)
        output = self.nodes[0].process.stdout.read()
        assert b'Options' in output
        self.log.info("Help text received: {} (...)".format(output[0:60]))
        self.nodes[0].running = False

        self.log.info("Start dashd with -version for version information")
        self.nodes[0].start(extra_args=['-version'], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        # Node should exit immediately and output version to stdout.
        ret_code = self.nodes[0].process.wait(timeout=1)
        assert_equal(ret_code, 0)
        output = self.nodes[0].process.stdout.read()
        assert b'version' in output
        self.log.info("Version text received: {} (...)".format(output[0:60]))

        # Test that arguments not in the help results in an error
        self.log.info("Start dashdd with -fakearg to make sure it does not start")
        self.nodes[0].start(extra_args=['-fakearg'], stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        # Node should exit immediately and output an error to stderr
        ret_code = self.nodes[0].process.wait(timeout=1)
        assert_equal(ret_code, 1)
        output = self.nodes[0].process.stderr.read()
        assert b'Error parsing command line arguments' in output
        self.log.info("Error message received: {} (...)".format(output[0:60]))

        # Clean up TestNode state
        self.nodes[0].running = False
        self.nodes[0].process = None
        self.nodes[0].rpc_connected = False
        self.nodes[0].rpc = None

if __name__ == '__main__':
    HelpTest().main()
