#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify framework startup failures only raise one exception since
multiple exceptions being raised muddies the waters of what actually
went wrong. We should maintain this bar of only raising one exception as
long as additional maintenance and complexity is low.

Test relaunches itself into child processes in order to trigger failures
without the parent process' BitcoinTestFramework also failing.
"""

from test_framework.util import rpc_port
from test_framework.test_framework import BitcoinTestFramework

import re
import subprocess
import sys

class FeatureFrameworkStartupFailures(BitcoinTestFramework):
    def add_options(self, parser):
        # These are only set when re-executing into child processes.
        parser.add_argument("--internal-rpc_timeout", dest="internal_rpc_timeout", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")
        parser.add_argument("--internal-extra_args", dest="internal_extra_args", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")
        parser.add_argument("--internal-start_stop", dest="internal_start_stop", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")

    def set_test_params(self):
        # If we are called with these arguments it means we are not in the
        # parent process.
        self.is_child = any(o is not None for o in [self.options.internal_rpc_timeout,
                                                    self.options.internal_extra_args,
                                                    self.options.internal_start_stop])
        # The parent process doesn't need a node itself.
        self.num_nodes = 1 if self.is_child else 0

        if self.options.internal_rpc_timeout is not None:
            self.rpc_timeout = self.options.internal_rpc_timeout
        if self.options.internal_extra_args:
            self.extra_args = [[self.options.internal_extra_args]]

    def setup_network(self):
        # Only proceed if num_nodes > 0 (child test processes),
        # as 0 (parent) is not supported by BitcoinTestFramework.setup_network().
        if self.num_nodes > 0:
            if self.options.internal_start_stop:
                self.add_nodes(self.num_nodes, self.extra_args)
                self.nodes[0].start()
                self.nodes[0].stop_node()
                assert False, "stop_node() should raise an exception when we haven't called wait_for_rpc_connection()"
            else:
                BitcoinTestFramework.setup_network(self)

    # Launches a child test process running this same file, now with specific
    # args, and verifies that we (only) raise the expected exception.
    def _verify_startup_failure(self, args, expected_exception):
        try:
            output = subprocess.run([sys.executable, __file__, f"--cachedir={self.options.cachedir}"] + args,
                                    encoding="utf-8",
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT,
                                    timeout=60 * self.options.timeout_factor).stdout
        except subprocess.TimeoutExpired as e:
            print(f"Unexpected timeout, subprocess output:\n{e.output}\nSubprocess output end", file=sys.stderr)
            raise

        errors = []

        traceback_count = len(re.findall("Traceback", output))
        if traceback_count != 1:
            errors.append(f"Found {traceback_count}/1 tracebacks - expecting exactly one with no knock-on exceptions.")

        matching_exception_count = len(re.findall(expected_exception, output))
        if matching_exception_count != 1:
            errors.append(f"Found {matching_exception_count}/1 occurrences of the specific exception: {expected_exception}")

        test_failure_msg_count = len(re.findall("Test failed. Test logging available at", output))
        if test_failure_msg_count != 1:
            errors.append(f"Found {test_failure_msg_count}/1 test failure output messages.")

        assert not errors, f"Child test didn't contain (only) expected errors:\n{chr(10).join(errors)}\n<CHILD OUTPUT BEGIN>:\n{output}\n<CHILD OUTPUT END>\n"

    def test_wrong_rpc_port(self):
        self.log.info("Verifying inability to connect to bitcoind's RPC interface due to wrong port results in one exception containing at least one OSError.")
        self._verify_startup_failure(
            # Lower the timeout so we don't wait that long.
            [f"--internal-rpc_timeout={int(max(3, self.options.timeout_factor))}",
            # Override RPC port to something TestNode isn't expecting so that we
            # are unable to establish an RPC connection.
            f"--internal-extra_args=-rpcport={rpc_port(2)}"],
            r"AssertionError: \[node 0\] Unable to connect to bitcoind after \d+s \(ignored errors: {[^}]*'OSError \w+'?: \d+[^}]*}, latest error: \w+\([^)]+\)\)"
        )

    def test_init_error(self):
        self.log.info("Verify startup failure due to invalid arg results in only one exception.")
        self._verify_startup_failure(
            ["--internal-extra_args=-nonexistentarg"],
            r"FailedToStartError: \[node 0\] bitcoind exited with status 1 during initialization\. Error: Error parsing command line arguments: Invalid parameter -nonexistentarg"
        )

    def test_start_stop(self):
        self.log.info("Verify start() then stop_node() on a node without wait_for_rpc_connection() in between raises an assert.")
        self._verify_startup_failure(
            ["--internal-start_stop=1"],
            r"AssertionError: \[node 0\] Should only call stop_node\(\) on a running node after wait_for_rpc_connection\(\) succeeded\. Did you forget to call the latter after start\(\)\? Not connected to process: \d+"
        )

    def run_test(self):
        assert not self.is_child, "Child test processes should fail at startup before even reaching run_test."

        self.test_wrong_rpc_port()
        self.test_init_error()
        self.test_start_stop()


if __name__ == '__main__':
    FeatureFrameworkStartupFailures(__file__).main()
