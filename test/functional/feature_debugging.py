#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify that debugging bitcoind works"""
from test_framework.test_framework import BitcoinTestFramework, SkipTest

from shutil import which
import platform
import subprocess

class DebuggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.gdb, self.lldb = which('gdb'), which('lldb')

        # The lldb.exe distributed with Visual Studio often lacks a liblldb.dll,
        # so dry-run it to make sure it's operational.
        if platform.system() == "Windows" and self.lldb:
            if subprocess.run([self.lldb, '--version'], timeout=5 * self.options.timeout_factor).returncode != 0:
                self.lldb = None

    def skip_if_missing_debugger(self):
        if not (self.gdb or self.lldb):
            raise SkipTest("Missing both GDB and LLDB")

    def skip_test_if_missing_module(self):
        self.skip_if_missing_debugger()

        if platform.system() == "Linux":
            try:
                with open('/proc/sys/kernel/yama/ptrace_scope', 'r') as f:
                    self.scope = int(f.read().strip())
                if self.scope != 0:
                    raise SkipTest("Debugging any process is disallowed by /proc/sys/kernel/yama/ptrace_scope.")
            except FileNotFoundError:
                pass  # Yama LSM not present, not sure whether debugging is blocked.

    def attempt_attaching(self, pid, expect_success: bool, test_waitfordebugger: bool):
        def verify_attach(name, args, expect_success):
            self.log.info(f"Verifying {name} {'can' if expect_success else 'cannot'} attach...")
            result = subprocess.run(args, timeout=15 * self.options.timeout_factor, capture_output=True, text=True)
            if expect_success != (result.returncode == 0):
                if expect_success:
                    raise AssertionError(f"Expected success but {args} failed ({result.returncode}):\n{result.stderr}")
                else:
                    raise AssertionError(f"Expected failure but {args} succeeded.")
            self.log.info("...Done")

        if test_waitfordebugger:
            # When expecting success and running the node with -waitfordebugger
            # we:
            # * Set a breakpoint in something like SocketHandler as it is called
            #   repeatedly once the node is initialized and responding to RPCs.
            # * Continue execution until the breakpoint is hit.
            # * Continue execution a second time before detaching, requiring
            #   this step verifies that the prior continue finished by hitting
            #   the breakpoint we set.
            #
            # Had we just detached without continuing to the breakpoint, while
            # inside WaitForDebugger(), it could result in no debugger being
            # detected, so the node would not leave that function and start
            # responding to RPCs. This would later wait_for_rpc_connection()
            # calls in tests would not complete.
            if self.gdb:
                verify_attach("GDB", [self.gdb, '-p', str(pid), '--batch'] +
                                     (['-ex', 'b SocketHandler', '-ex', 'continue', '-ex', 'continue'] if expect_success else []) +
                                     ['-ex', 'detach'], expect_success)
            if self.lldb:
                verify_attach("LLDB", [self.lldb, '-p', str(pid), '--batch'] +
                                      (['-o', 'b SocketHandler', '-o', 'continue', '-o', 'continue'] if expect_success else []) +
                                      ['-o', 'detach -s false'], expect_success)
        else:
            if self.gdb:
                verify_attach("GDB", [self.gdb, '-p', str(pid), '--batch'] +
                                     ['-ex', 'detach'], expect_success)
            if self.lldb:
                verify_attach("LLDB", [self.lldb, '-p', str(pid), '--batch'] +
                                      ['-o', 'detach -s false'], expect_success)

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Verifying we can attach to node")
        assert node.rpc_connected, "Node should be up and responding to RPCs before we start checking whether one can debug it."
        self.attempt_attaching(node.process.pid, expect_success=True, test_waitfordebugger=False)
        node.stop_node()

        if self.is_wait_for_debugger_compiled():
            self.log.info("Verifying we can attach to node with -waitfordebugger")
            node.start(extra_args=["-waitfordebugger"])
            self.attempt_attaching(node.process.pid, expect_success=True, test_waitfordebugger=True)
            self.log.debug("Ensure node is fully connected before teardown")
            node.wait_for_rpc_connection()
        else:
            self.log.info("Verifying -waitfordebugger support is disabled")
            node.assert_start_raises_init_error(
                ['-waitfordebugger'],
                expected_msg="Error: -waitfordebugger is unavailable in this build. Rebuild with -DENABLE_WAIT_FOR_DEBUGGER=ON.")


if __name__ == '__main__':
    DebuggingTest(__file__).main()
