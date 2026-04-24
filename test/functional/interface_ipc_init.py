#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test IPC initialization behavior."""

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

    def test_ipcmaxconnections(self):
        self.log.info("Test -ipcmaxconnections initialization behavior")
        node = self.nodes[0]

        with node.assert_debug_log([
            "file descriptors available",
            "Reserving 9 file descriptors for IPC (1 listening sockets, 8 accepted connections)",
        ]):
            self.restart_node(0, extra_args=["-ipcmaxconnections=8"])
        assert_equal(node.getblockcount(), 0)

        with node.assert_debug_log(["Reserving 1 file descriptors for IPC (1 listening sockets, 0 accepted connections)"]):
            self.restart_node(0, extra_args=["-ipcmaxconnections=0"])
        assert_equal(node.getblockcount(), 0)

        with node.assert_debug_log(["-ipcmaxconnections is 8 but -ipcbind is not enabled; option will have no effect."]):
            self.restart_node(0, extra_args=["-noipcbind", "-ipcmaxconnections=8"])
        assert_equal(node.getblockcount(), 0)

        self.stop_node(0)
        node.assert_start_raises_init_error(
            extra_args=["-ipcmaxconnections=-1"],
            expected_msg="Error: -ipcmaxconnections must be greater than or equal to zero",
        )
        self.start_node(0)

    def run_test(self):
        self.test_ipcmaxconnections()


if __name__ == '__main__':
    IPCInitTest(__file__).main()
