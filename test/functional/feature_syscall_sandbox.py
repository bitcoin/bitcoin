#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoind aborts if a disallowed syscall is used when compiled with the syscall sandbox."""

from test_framework.test_framework import BitcoinTestFramework, SkipTest


class SyscallSandboxTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        if not self.is_syscall_sandbox_compiled():
            raise SkipTest("bitcoind has not been built with syscall sandbox enabled.")

    def run_test(self):
        disallowed_syscall_terminated_bitcoind = False
        expected_log_entry = 'ERROR: The syscall "getgroups" (syscall number 115) is not allowed by the syscall sandbox'
        if self.config["components"].getboolean(
            "ENABLE_SYSCALL_SANDBOX_MODE_KILL_WITHOUT_DEBUG"
        ):
            expected_log_entry = ""
        with self.nodes[0].assert_debug_log([expected_log_entry]):
            self.log.info("Invoking disallowed syscall")
            try:
                self.nodes[0].invokedisallowedsyscall()
            except ConnectionRefusedError:
                disallowed_syscall_terminated_bitcoind = True
        assert disallowed_syscall_terminated_bitcoind
        self.nodes = []


if __name__ == "__main__":
    SyscallSandboxTest().main()
