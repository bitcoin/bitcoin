#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify various startup failures only raise one exception since multiple
exceptions being raised muddies the waters of what actually went wrong.
We should maintain this bar of only raising one exception as long as
additional maintenance and complexity is low.

Test relaunches itself into a child processes in order to trigger failure
without the parent process' BitcoinTestFramework also failing.
"""

from test_framework.util import rpc_port
from test_framework.test_framework import BitcoinTestFramework

import re
import subprocess
import sys

class FeatureFrameworkRPCFailure(BitcoinTestFramework):
    def set_test_params(self):
        # Only run a node for child processes
        self.num_nodes = 1 if any(o is not None for o in [self.options.internal_rpc_timeout,
                                                          self.options.internal_extra_args,
                                                          self.options.internal_start_stop]) else 0

        if self.options.internal_rpc_timeout is not None:
            self.rpc_timeout = self.options.internal_rpc_timeout
        if self.options.internal_extra_args:
            self.extra_args = [[self.options.internal_extra_args]]

    def add_options(self, parser):
        parser.add_argument("--internal-rpc_timeout", dest="internal_rpc_timeout", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")
        parser.add_argument("--internal-extra_args", dest="internal_extra_args", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")
        parser.add_argument("--internal-start_stop", dest="internal_start_stop", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")

    def setup_network(self):
        # Avoid doing anything if num_nodes == 0, otherwise we fail.
        if self.num_nodes > 0:
            if self.options.internal_start_stop:
                self.add_nodes(self.num_nodes, self.extra_args)
                self.nodes[0].start()
                self.nodes[0].stop_node()
            else:
                BitcoinTestFramework.setup_network(self)

    def _run_test_internal(self, args, expected_exception):
        try:
            result = subprocess.run([sys.executable, __file__] + args, encoding="utf-8", stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=10 * self.options.timeout_factor)
        except subprocess.TimeoutExpired as e:
            print(f"Unexpected timeout, subprocess output:\n{e.output}\nSubprocess output end", file=sys.stderr)
            raise

        success = True

        traceback_count = len(re.findall("Traceback", result.stdout))
        if traceback_count != 1:
            self.log.error(f"Found {traceback_count}/1 tracebacks - expecting exactly one with no knock-on exceptions.")
            success = False

        matching_exception_count = len(re.findall(expected_exception, result.stdout))
        if matching_exception_count != 1:
            self.log.error(f"Found {matching_exception_count}/1 occurrences of the specific exception: {expected_exception}")
            success = False

        test_failure_msg_count = len(re.findall("Test failed. Test logging available at", result.stdout))
        if test_failure_msg_count != 1:
            self.log.error(f"Found {test_failure_msg_count}/1 test failure output messages.")
            success = False

        if not success:
            raise AssertionError(f"Child test didn't contain (only) expected errors.\n<CHILD OUTPUT BEGIN>:\n{result.stdout}\n<CHILD OUTPUT END>\n")

    def test_instant_rpc_timeout(self):
        self.log.info("Verifying timeout in connecting to bitcoind's RPC interface results in only one exception.")
        self._run_test_internal(
            ["--internal-rpc_timeout=0"],
            "AssertionError: \\[node 0\\] Unable to connect to bitcoind after 0s"
        )

    def test_wrong_rpc_port(self):
        self.log.info("Verifying inability to connect to bitcoind's RPC interface due to wrong port results in one exception containing at least one OSError.")
        self._run_test_internal(
            # Lower the timeout so we don't wait that long.
            [f"--internal-rpc_timeout={int(max(3, self.options.timeout_factor))}",
            # Override RPC port to something TestNode isn't expecting so that we
            # are unable to establish an RPC connection.
            f"--internal-extra_args=-rpcport={rpc_port(2)}"],
            r"AssertionError: \[node 0\] Unable to connect to bitcoind after \d+s \(ignored errors: {[^}]*'OSError \w+'?: \d+[^}]*}, latest error: \w+\([^)]+\)\)"
        )

    def test_init_error(self):
        self.log.info("Verify startup failure due to invalid arg results in only one exception.")
        self._run_test_internal(
            ["--internal-extra_args=-nonexistentarg"],
            "FailedToStartError: \\[node 0\\] bitcoind exited with status 1 during initialization. Error: Error parsing command line arguments: Invalid parameter -nonexistentarg"
        )

    def test_start_stop(self):
        self.log.info("Verify start() then stop_node() on a node without wait_for_rpc_connection() in between triggers an assert.")
        self._run_test_internal(
            ["--internal-start_stop=1"],
            r"AssertionError: \[node 0\] Should only call stop_node\(\) on a running node after wait_for_rpc_connection\(\) succeeded\. Did you forget to call the latter after start\(\)\? Not connected to process: \d+"
        )

    def run_test(self):
        if self.options.internal_rpc_timeout is None and self.options.internal_extra_args is None:
            self.test_instant_rpc_timeout()
            self.test_wrong_rpc_port()
            self.test_init_error()
            self.test_start_stop()


if __name__ == '__main__':
    FeatureFrameworkRPCFailure(__file__).main()
