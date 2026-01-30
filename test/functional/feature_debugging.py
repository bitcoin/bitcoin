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

    def attempt_attaching(self, pid, expect_success: bool):
        def verify_attach(name, args, expect_success):
            self.log.info(f"Verifying {name} {'can' if expect_success else 'cannot'} attach...")
            result = subprocess.run(args, timeout=15 * self.options.timeout_factor, capture_output=True, text=True)
            if expect_success != (result.returncode == 0):
                if expect_success:
                    raise AssertionError(f"Expected success but {args} failed ({result.returncode}):\n{result.stderr}")
                else:
                    raise AssertionError(f"Expected failure but {args} succeeded.")
            self.log.info("...Done")

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
        self.attempt_attaching(node.process.pid, expect_success=True)


if __name__ == '__main__':
    DebuggingTest(__file__).main()
