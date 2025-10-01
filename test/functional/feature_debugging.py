#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify that debugging bitcoind is not allowed

This is important when wallets hold private keys in memory.
"""
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import (
    assert_equal,
    assert_not_equal,
)

from shutil import which
import os
import subprocess

class DebuggingDisallowedTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        # For Windows there doesn't seem to be much to do once user has
        # SeDebugPrivilege.
        # For MacOS, it depends on the com.apple.security.get-task-allow
        # entitlement which is set during development.
        self.skip_if_platform_not_linux()
        # We don't really care about other processes reading our memory if we don't even support wallets with private keys.
        self.skip_if_no_wallet()
        if os.geteuid() == 0:
            raise SkipTest("Can't verify debugging being disallowed when running with root privileges.")

        try:
            with open('/proc/sys/kernel/yama/ptrace_scope', 'r', encoding='utf8') as f:
                self.scope = int(f.read().strip())
            if self.scope == 0:
                raise SkipTest("Debugging any process is allowed by /proc/sys/kernel/yama/ptrace_scope.")
        except FileNotFoundError:
            pass  # Yama LSM not present, not sure whether debugging is blocked.

        self.gdb, self.lldb = which('gdb'), which('lldb')
        if not (self.gdb or self.lldb):
            raise SkipTest("Missing both GDB and LLDB")

    def attempt_attaching(self, pid, expect_success: bool):
        def verify_attach(name, args, expect_success):
            self.log.info(f"Verifying {name} {'can' if expect_success else 'cannot'} attach")
            exit_code = subprocess.run(args, timeout=15 * self.options.timeout_factor, capture_output=True).returncode
            if expect_success:
                assert_equal(exit_code, 0)
            else:
                assert_not_equal(exit_code, 0)

        if self.gdb:
            verify_attach("GDB", [self.gdb, '-p', str(pid), '--batch'] +
                                 ['--ex', 'detach'], expect_success)
        if self.lldb:
            verify_attach("LLDB", [self.lldb, '-p', str(pid), '--batch'] +
                                  ['-o', 'detach -s false'], expect_success)

    def run_test(self):
        assert self.nodes[0].rpc_connected, "Node should be up and responding to RPCs before we start checking whether one can debug it."
        node_pid = self.nodes[0].process.pid

        assert os.path.exists(f'/proc/{node_pid}')

        # TODO: When wallets are loaded in bitcoind, we should ideally disable:
        # Dumpable flag (/proc/{node_pid}/coredump_filter)
        # Reading of memory: assert_raises(PermissionError, lambda: open(f'/proc/{node_pid}/mem', 'rb', encoding='utf8').seek(1))

        # Verify no other process is attached and blocking us.
        tracer = None
        with open(f'/proc/{node_pid}/status', 'r', encoding='utf8') as f:
            for line in f:
                if line.startswith('TracerPid:'):
                    tracer = int(line.split()[1])
        assert_equal(tracer, 0)

        self.attempt_attaching(node_pid, expect_success=False)
        self.stop_node(0)


if __name__ == '__main__':
    DebuggingDisallowedTest(__file__).main()
