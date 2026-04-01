#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test IPC initialization behavior."""

from test_framework.test_framework import BitcoinTestFramework


class IPCInitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()

    def test_ipc_startup_logging(self):
        self.log.info("Test IPC startup logging")

        with self.nodes[0].assert_debug_log(["file descriptors available"]):
            self.restart_node(0)

    def run_test(self):
        self.test_ipc_startup_logging()


if __name__ == '__main__':
    IPCInitTest(__file__).main()
