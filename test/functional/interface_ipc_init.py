#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test IPC initialization behavior."""

import tempfile
from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class IPCInitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()
        self.nodes[0].args = [arg for arg in self.nodes[0].args if not arg.startswith("-ipcbind=")]
        self.ipc_tmpdir = tempfile.TemporaryDirectory(prefix="btc-ipc-init-")
        self.ipcbind_path = Path(self.ipc_tmpdir.name) / "ipcinit.sock"

    def test_ipcbind_max_connections(self):
        self.log.info("Test -ipcbind max-connections initialization behavior")
        node = self.nodes[0]

        # Test default value (16)
        with node.assert_debug_log([
            "Reserving 17 file descriptors for IPC (1 listening sockets, 16 accepted connections)",
        ]):
            self.restart_node(0, extra_args=[f"-ipcbind=unix:{self.ipcbind_path}"])

        ipcbind_arg = f"-ipcbind=unix:{self.ipcbind_path}:max-connections={{}}"

        with node.assert_debug_log([
            "file descriptors available",
            "Reserving 9 file descriptors for IPC (1 listening sockets, 8 accepted connections)",
        ]):
            self.restart_node(0, extra_args=[ipcbind_arg.format(8)])
        assert_equal(node.getblockcount(), 0)

        with node.assert_debug_log(["Reserving 1 file descriptors for IPC (1 listening sockets, 0 accepted connections)"]):
            self.restart_node(0, extra_args=[ipcbind_arg.format(0)])
        assert_equal(node.getblockcount(), 0)

    def run_test(self):
        self.test_ipcbind_max_connections()


if __name__ == '__main__':
    IPCInitTest(__file__).main()
