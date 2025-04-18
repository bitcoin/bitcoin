#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test IPC with bitcoin-cli"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    rpc_port
)

import subprocess
import tempfile

class TestBitcoinIpcCli(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()

    def setup_nodes(self):
        # Always run IPC node binary
        self.binary_paths.bitcoind = self.binary_paths.bitcoin_node

        # Work around default CI path exceeding maximum socket path length.
        # On Linux sun_path is 108 bytes, on macOS it's only 104. Includes
        # null terminator.
        self.socket_path = self.options.tmpdir + "/node0/regtest/node.sock"
        if len(self.socket_path.encode('utf-8')) < 104:
            self.extra_args = [["-ipcbind=unix"]]
            self.default_socket = True
        else:
            self.socket_path = tempfile.mktemp()
            self.extra_args = [[f"-ipcbind=unix:{self.socket_path}"]]
            self.default_socket = False
        super().setup_nodes()

    def test_cli(self, args, error=None):
        # Intentionally set wrong RPC password so only IPC not HTTP connections work
        args = [self.binary_paths.bitcoincli, f"-datadir={self.nodes[0].datadir_path}", "-rpcpassword=wrong"] + args + ["echo", "foo"]
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if error:
            assert_equal(result.stdout, error)
        else:
            assert_equal(result.stdout, '[\n  "foo"\n]\n')
        assert_equal(result.stderr, None)
        assert_equal(result.returncode, 1 if error else 0)

    def run_test(self):
        if not self.default_socket:
            self.log.info("Skipping a few checks because temporary directory path is too long")

        http_auth_error = "error: Authorization failed: Incorrect rpcuser or rpcpassword\n"
        http_connect_error = f"error: timeout on transient error: Could not connect to the server 127.0.0.1:{rpc_port(self.nodes[0].index)}\n\nMake sure the bitcoind server is running and that you are connecting to the correct RPC port.\nUse \"bitcoin-cli -help\" for more info.\n"
        http_ipc_error = "error: -rpcconnect and -ipcconnect options cannot both be enabled\n"
        ipc_connect_error = "error: Connection refused\n\nProbably bitcoin-node is not running or not listening on a unix socket. Can be started with:\n\n    bitcoin-node -chain=regtest -ipcbind=unix\n"

        for started in (True, False):
            auto_error = None if started else http_connect_error
            http_error = http_auth_error if started else http_connect_error
            ipc_error = None if started else ipc_connect_error

            if self.default_socket:
                self.test_cli([], auto_error)
                self.test_cli(["-rpcconnect=127.0.0.1"], http_error)
                self.test_cli(["-ipcconnect=auto"], auto_error)
                self.test_cli(["-ipcconnect=auto", "-rpcconnect=127.0.0.1"], http_error)
                self.test_cli(["-ipcconnect=unix"], ipc_error)

            self.test_cli([f"-ipcconnect=unix:{self.socket_path}"], ipc_error)
            self.test_cli(["-noipcconnect"], http_error)
            self.test_cli(["-ipcconnect=unix", "-rpcconnect=127.0.0.1"], http_ipc_error)

            self.stop_node(0)


if __name__ == '__main__':
    TestBitcoinIpcCli(__file__).main()
