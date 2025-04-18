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
    def set_test_params(self):
        self.num_nodes = 0  # Parent process doesn't need a node itself.

    def setup_network(self):
        # BitcoinTestFramework.setup_network() does not support 0 nodes.
        pass

    # Launches a child test process running this same file, but instantiating
    # a child test, and verifies that we (only) raise the expected exception.
    def _verify_startup_failure(self, test, expected_exception):
        try:
            output = subprocess.run([sys.executable, __file__, f"--cachedir={self.options.cachedir}", f"--internal_test={test.__name__}"],
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

    def run_test(self):
        self.log.info("Verifying inability to connect to bitcoind's RPC interface due to wrong port results in one exception containing at least one OSError.")
        self._verify_startup_failure(
            TestWrongRpcPortStartupFailure,
            r"AssertionError: \[node 0\] Unable to connect to bitcoind after \d+s \(ignored errors: {[^}]*'OSError \w+'?: \d+[^}]*}, latest error: \w+\([^)]+\)\)"
        )

        self.log.info("Verify startup failure due to invalid arg results in only one exception.")
        self._verify_startup_failure(
            TestInitErrorStartupFailure,
            r"FailedToStartError: \[node 0\] bitcoind exited with status 1 during initialization\. Error: Error parsing command line arguments: Invalid parameter -nonexistentarg"
        )

        self.log.info("Verify start() then stop_node() on a node without wait_for_rpc_connection() in between raises an assert.")
        self._verify_startup_failure(
            TestStartStopStartupFailure,
            r"AssertionError: \[node 0\] Should only call stop_node\(\) on a running node after wait_for_rpc_connection\(\) succeeded\. Did you forget to call the latter after start\(\)\? Not connected to process: \d+"
        )

class InternalTestMixin:
    def add_options(self, parser):
        # Just here to silence unrecognized argument error, we actually read the value in the if-main at the bottom.
        parser.add_argument("--internal_test", dest="internal_never_read", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")

class TestWrongRpcPortStartupFailure(InternalTestMixin, BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # Lower the timeout so we don't wait that long.
        self.rpc_timeout = max(3, self.options.timeout_factor)
        # Override RPC port to something TestNode isn't expecting so that we
        # are unable to establish an RPC connection.
        self.extra_args = [[f"-rpcport={rpc_port(2)}"]]

    def run_test(self):
        assert False, "Should have failed earlier during startup."

class TestInitErrorStartupFailure(InternalTestMixin, BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-nonexistentarg"]]

    def run_test(self):
        assert False, "Should have failed earlier during startup."

class TestStartStopStartupFailure(InternalTestMixin, BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_network(self):
        self.add_nodes(self.num_nodes, self.extra_args)
        self.nodes[0].start()
        self.nodes[0].stop_node() # This should raise an exception
        assert False, "stop_node() should raise an exception when we haven't called wait_for_rpc_connection()"

    def run_test(self):
        assert False, "Should have failed earlier during startup."


if __name__ == '__main__':
    if not any('--internal_test=' in arg for arg in sys.argv):
        FeatureFrameworkStartupFailures(__file__).main()
    else:
        for arg in sys.argv:
            if arg[:16] == '--internal_test=':
                class_name = arg[16:]
                c = sys.modules[__name__].__dict__[class_name]
                assert c, f"Unrecognized test: {class_name}"
                c(__file__).main()
                break
