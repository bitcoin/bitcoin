#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify that debugging bitcoind is not allowed on Linux

This is important when wallets hold private keys in memory.

For Windows there doesn't seem to be much to do once user has SeDebugPrivilege.

For MacOS, it depends on the com.apple.security.get-task-allow entitlement which
is set during development.

Also verifies that debugging becomes allowed when using -waitfordebugger.
"""
from test_framework.test_framework import SkipTest
from test_framework.util import assert_equal
from feature_debugging import DebuggingTest

import os

class DebuggingDisallowedTest(DebuggingTest):
    def set_test_params(self):
        DebuggingTest.set_test_params(self)

    def skip_test_if_missing_module(self):
        self.skip_if_missing_debugger()
        self.skip_if_platform_not_linux()
        self.skip_if_no_wallet()
        if os.geteuid() == 0:
            raise SkipTest("Can't verify debugging being disallowed when running with root privileges.")

        try:
            with open('/proc/sys/kernel/yama/ptrace_scope', 'r') as f:
                self.scope = int(f.read().strip())
            if self.scope == 0:
                raise SkipTest("Debugging any process is allowed by /proc/sys/kernel/yama/ptrace_scope.")
        except FileNotFoundError:
            pass  # Yama LSM not present, not sure whether debugging is blocked.

    def run_test(self):
        node = self.nodes[0]
        assert node.rpc_connected, "Node should be up and responding to RPCs before we start checking whether one can debug it."
        node_pid = node.process.pid

        # Verify no other process is attached and blocking us.
        tracer = None
        with open(f'/proc/{node_pid}/status', 'r') as f:
            for line in f:
                if line.startswith('TracerPid:'):
                    tracer = int(line.split()[1])
        assert_equal(tracer, 0)

        self.attempt_attaching(node_pid, expect_success=False, test_waitfordebugger=False)
        self.stop_node(0)

        if self.is_wait_for_debugger_compiled():
            self.log.info("Verifying that -waitfordebugger *changes process permissions* so that we can attach.")
            # Using TestNode.start() over BitcoinTestFramework.start_node() here
            # as the former does not block waiting for the RPC connection.
            # -waitfordebugger means we will not have an RPC connection until
            # a debugger has been attached.
            node.start(['-waitfordebugger'])
            self.attempt_attaching(self.nodes[0].process.pid, expect_success=True, test_waitfordebugger=True)
            self.log.debug("Ensure node is fully connected before teardown")
            node.wait_for_rpc_connection()


if __name__ == '__main__':
    DebuggingDisallowedTest(__file__).main()
