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

from test_framework.util import (
    assert_raises_message,
    rpc_port,
)
from test_framework.test_framework import BitcoinTestFramework

from hashlib import md5
from os import linesep
import re
import subprocess
import sys
import time

class FeatureFrameworkStartupFailures(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_network(self):
        # Don't start the node yet, as we want to measure during run_test.
        self.add_nodes(self.num_nodes, self.extra_args)

    # Launches a child test process that runs this same file, but instantiates
    # a child test. Verifies that it raises only the expected exception, once.
    def _verify_startup_failure(self, test, internal_args, expected_exception):
        # Inherit sys.argv from parent, only overriding tmpdir to a subdirectory
        # so children don't fail due to colliding with the parent dir.
        assert self.options.tmpdir, "Framework should always set tmpdir."
        subdir = md5(expected_exception.encode('utf-8')).hexdigest()[:8]
        args = [sys.executable] + sys.argv + [f"--tmpdir={self.options.tmpdir}/{subdir}", f"--internal_test={test.__name__}"] + internal_args

        try:
            # The subprocess encoding argument gives different results for e.output
            # on Linux/Windows, so handle decoding by ourselves for consistency.
            output = subprocess.run(args, timeout=60 * self.options.timeout_factor,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.decode("utf-8")
        except subprocess.TimeoutExpired as e:
            print("Unexpected child process timeout!\n"
                  "WARNING: Timeouts like this halt execution of TestNode logic, "
                  "meaning dangling bitcoind processes are to be expected.\n"
                  f"<CHILD OUTPUT BEGIN>\n{e.output.decode('utf-8')}\n<CHILD OUTPUT END>",
                  file=sys.stderr)
            raise

        errors = []
        if (n := output.count("Traceback")) != 1:
            errors.append(f"Found {n}/1 tracebacks - expecting exactly one with no knock-on exceptions.")
        if (n := len(re.findall(expected_exception, output))) != 1:
            errors.append(f"Found {n}/1 occurrences of the specific exception: {expected_exception}")
        if (n := output.count("Test failed. Test logging available at")) != 1:
            errors.append(f"Found {n}/1 test failure output messages.")

        assert not errors, f"Child test didn't contain (only) expected errors:\n{linesep.join(errors)}\n<CHILD OUTPUT BEGIN>\n{output}\n<CHILD OUTPUT END>\n"

    def run_test(self):
        self.log.info("Verifying _verify_startup_failure() functionality (self-check).")
        assert_raises_message(
            AssertionError,
            ( "Child test didn't contain (only) expected errors:\n"
             f"Found 0/1 tracebacks - expecting exactly one with no knock-on exceptions.{linesep}"
             f"Found 0/1 occurrences of the specific exception: NonExistentError{linesep}"
              "Found 0/1 test failure output messages."),
            self._verify_startup_failure,
            TestSuccess, [],
            "NonExistentError",
        )

        self.log.info("Parent process is measuring node startup duration in order to obtain a reasonable timeout value for later test...")
        node_start_time = time.time()
        self.nodes[0].start()
        self.nodes[0].wait_for_rpc_connection()
        node_start_duration = time.time() - node_start_time
        self.nodes[0].stop_node()
        self.log.info(f"...measured {node_start_duration:.1f}s.")

        self.log.info("Verifying inability to connect to bitcoind's RPC interface due to wrong port results in one exception containing at least one OSError.")
        self._verify_startup_failure(
            TestWrongRpcPortStartupFailure, [f"--internal_node_start_duration={node_start_duration}"],
            r"AssertionError: \[node 0\] Unable to connect to bitcoind after \d+s \(ignored errors: {[^}]*'OSError \w+'?: \d+[^}]*}, latest: '[\w ]+'/\w+\([^)]+\)\)"
        )

        self.log.info("Verifying startup failure due to invalid arg results in only one exception.")
        self._verify_startup_failure(
            TestInitErrorStartupFailure, [],
            r"FailedToStartError: \[node 0\] bitcoind exited with status 1 during initialization\. Error: Error parsing command line arguments: Invalid parameter -nonexistentarg"
        )

        self.log.info("Verifying start() then stop_node() on a node without wait_for_rpc_connection() in between raises an assert.")
        self._verify_startup_failure(
            TestStartStopStartupFailure, [],
            r"AssertionError: \[node 0\] Should only call stop_node\(\) on a running node after wait_for_rpc_connection\(\) succeeded\. Did you forget to call the latter after start\(\)\? Not connected to process: \d+"
        )

class InternalTestMixin:
    def add_options(self, parser):
        # Just here to silence unrecognized argument error, we actually read the value in the if-main at the bottom.
        parser.add_argument("--internal_test", dest="internal_never_read", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF")

class TestWrongRpcPortStartupFailure(InternalTestMixin, BitcoinTestFramework):
    def add_options(self, parser):
        parser.add_argument("--internal_node_start_duration", dest="node_start_duration", help="ONLY TO BE USED WHEN TEST RELAUNCHES ITSELF", type=float)
        InternalTestMixin.add_options(self, parser)

    def set_test_params(self):
        self.num_nodes = 1
        # Override RPC listen port to something TestNode isn't expecting so that
        # we are unable to establish an RPC connection.
        self.extra_args = [[f"-rpcport={rpc_port(2)}"]]
        # Override the timeout to avoid waiting unnecessarily long to realize
        # nothing is on that port. Divide by timeout_factor to counter
        # multiplication in base, 2 * node_start_duration should be enough.
        self.rpc_timeout = max(3, 2 * self.options.node_start_duration) / self.options.timeout_factor

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
        self.nodes[0].stop_node()
        assert False, "stop_node() should raise an exception when we haven't called wait_for_rpc_connection()"

    def run_test(self):
        assert False, "Should have failed earlier during startup."

class TestSuccess(InternalTestMixin, BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_network(self):
        pass # Don't need to start our node.

    def run_test(self):
        pass # Just succeed.


if __name__ == '__main__':
    if class_name := next((m[1] for arg in sys.argv[1:] if (m := re.match(r'--internal_test=(.+)', arg))), None):
        internal_test = globals().get(class_name)
        assert internal_test, f"Unrecognized test: {class_name}"
        internal_test(__file__).main()
    else:
        FeatureFrameworkStartupFailures(__file__).main()
