#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test IPC initialization behavior."""

import socket
import tempfile
import time
from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

MAX_IPC_CONNECTIONS = 1 << 20


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
        self.ipcbind_path2 = Path(self.ipc_tmpdir.name) / "ipcinit2.sock"

    def test_ipcbind_max_connections(self):
        self.log.info("Test -ipcbind max-connections initialization behavior")
        node = self.nodes[0]

        # Test default value (16)
        with node.assert_debug_log([
            "Reserving 17 file descriptors for IPC (1 listening sockets, 16 connection slots)",
        ]):
            self.restart_node(0, extra_args=[f"-ipcbind=unix:{self.ipcbind_path}"])

        ipcbind_arg = f"-ipcbind=unix:{self.ipcbind_path},max-connections={{}}"

        with node.assert_debug_log([
            "Reserving 9 file descriptors for IPC (1 listening sockets, 8 connection slots)",
        ]):
            self.restart_node(0, extra_args=[ipcbind_arg.format(8)])
        assert_equal(node.getblockcount(), 0)

        self.stop_node(0)
        node.assert_start_raises_init_error(
            extra_args=[ipcbind_arg.format(0)],
            expected_msg=f"Error: Invalid -ipcbind address 'unix:{self.ipcbind_path},max-connections=0': max-connections must be at least 1",
        )
        node.assert_start_raises_init_error(
            extra_args=[ipcbind_arg.format(MAX_IPC_CONNECTIONS + 1)],
            expected_msg=f"Error: Invalid -ipcbind address 'unix:{self.ipcbind_path},max-connections={MAX_IPC_CONNECTIONS + 1}': max-connections must be at most {MAX_IPC_CONNECTIONS}",
        )

        # Individually valid limits must not exceed the aggregate IPC FD cap.
        half_max = MAX_IPC_CONNECTIONS // 2
        node.assert_start_raises_init_error(
            extra_args=[
                ipcbind_arg.format(half_max),
                f"-ipcbind=unix:{self.ipcbind_path2},max-connections={half_max}",
            ],
            expected_msg="Error: Too many IPC file descriptors requested",
        )

    def test_ipcbind_connection_limiting(self):
        self.log.info("Test per-address -ipcbind connection limiting")
        node = self.nodes[0]
        self.restart_node(0, extra_args=[
            f"-ipcbind=unix:{self.ipcbind_path},max-connections=1",
            f"-ipcbind=unix:{self.ipcbind_path2},max-connections=1",
            "-debug=ipc",
        ])

        def ipc_client(path):
            client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            client.connect(str(path))
            return client

        # Each listener independently accepts its first client.
        with node.assert_debug_log(["IPC server: socket connected."], timeout=5):
            first1 = ipc_client(self.ipcbind_path)
        with node.assert_debug_log(["IPC server: socket connected."], timeout=5):
            first2 = ipc_client(self.ipcbind_path2)

        # Further clients can connect to the socket backlog but must not be
        # accepted while their listener's only slot is occupied.
        with node.assert_debug_log([], unexpected_msgs=["IPC server: socket connected."]):
            second1 = ipc_client(self.ipcbind_path)
            second2 = ipc_client(self.ipcbind_path2)
            time.sleep(0.5)

        # Freeing a slot on either listener immediately admits its waiting client.
        with node.assert_debug_log([
            "IPC server: socket disconnected.",
            "IPC server: socket connected.",
        ], timeout=5):
            first1.close()
        with node.assert_debug_log([
            "IPC server: socket disconnected.",
            "IPC server: socket connected.",
        ], timeout=5):
            first2.close()

        with node.assert_debug_log(["IPC server: socket disconnected."], timeout=5):
            second1.close()
        with node.assert_debug_log(["IPC server: socket disconnected."], timeout=5):
            second2.close()

    def run_test(self):
        self.test_ipcbind_max_connections()
        self.test_ipcbind_connection_limiting()


if __name__ == '__main__':
    IPCInitTest(__file__).main()
