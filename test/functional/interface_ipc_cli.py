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

class TestBitcoinIpcCli(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()

    def test_cli(self, args, error=None, call=None, expected=None, returncode=None):
        # Intentionally set wrong RPC password so only IPC not HTTP connections work
        args = [self.binary_paths.bitcoincli, f"-datadir={self.nodes[0].datadir_path}", "-rpcpassword=wrong"] + args + (call or ["echo", "foo"])
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if error is None:
            assert_equal(result.stdout, expected if expected is not None else '[\n  "foo"\n]\n')
        else:
            assert result.stdout.startswith(error), f"Output didn't start with the expected error {error!r}:\n{result.stdout}"
        assert_equal(result.stderr, None)
        assert_equal(result.returncode, returncode if returncode is not None else (0 if error is None else 1))

    def test_whitelist(self):
        """A method the caller may not use is reported as a permission error over IPC.

        The username bitcoin-cli sends over IPC is the -rpcuser value; it selects
        which -rpcwhitelist applies and is not authenticated.
        """
        self.log.info("Testing -rpcwhitelist denials over IPC")
        # -rpcwhitelistdefault=0 so the whitelist applies only to ipcuser and the
        # test framework's own cookie connection keeps working.
        self.restart_node(0, extra_args=["-rpcwhitelistdefault=0", "-rpcwhitelist=ipcuser:getblockcount"])
        # Name the socket explicitly, because -ipcconnect=auto does not find it
        # when the datadir path is too long and the socket is in a temporary
        # directory instead.
        args = [f"-ipcconnect=unix:{self.nodes[0].ipc_socket_path}", "-rpcuser=ipcuser"]
        # A permitted method still works, so the whitelist is in force.
        self.test_cli(args, call=["getblockcount"], expected="0\n")
        # Before the denial carried a JSON-RPC error object, the reply body was
        # empty over IPC and bitcoin-cli reported a JSON type error instead.
        self.test_cli(args,
                      error="error code: -38\nerror message:\nUser is not permitted to call method echo",
                      returncode=38)
        self.restart_node(0)

    def run_test(self):
        node = self.nodes[0]
        if node.ipc_tmp_dir:
            self.log.info("Skipping a few checks because temporary directory path is too long")

        http_auth_error = "error: Authorization failed: Incorrect rpcuser or rpcpassword were specified."
        http_connect_error = f"error: Error while attempting to communicate with server 127.0.0.1:{rpc_port(node.index)} (Could not connect to the server)\n\nMake sure the bitcoind server is running and that you are connecting to the correct RPC port.\nUse \"bitcoin-cli -help\" for more info.\n"
        ipc_connect_error = "error: Connection refused\n\nProbably bitcoin-node is not running or not listening on a unix socket. Can be started with:\n\n    bitcoin-node -chain=regtest -ipcbind=unix\n"
        ipc_http_conflict = "error: -rpcconnect and -ipcconnect options cannot both be enabled\n"

        for started in (True, False):
            auto_error = None if started else http_connect_error
            http_error = http_auth_error if started else http_connect_error
            ipc_error = None if started else ipc_connect_error

            if not node.ipc_tmp_dir:
                self.test_cli([], auto_error)
                self.test_cli(["-rpcconnect=127.0.0.1"], http_error)
                self.test_cli(["-ipcconnect=auto"], auto_error)
                self.test_cli(["-ipcconnect=auto", "-rpcconnect=127.0.0.1"], http_error)
                self.test_cli(["-ipcconnect=unix"], ipc_error)

            self.test_cli([f"-ipcconnect=unix:{node.ipc_socket_path}"], ipc_error)
            self.test_cli(["-noipcconnect"], http_error)
            self.test_cli(["-ipcconnect=unix", "-rpcconnect=127.0.0.1"], ipc_http_conflict)

            self.stop_node(0)

        self.start_node(0)
        self.test_whitelist()


if __name__ == '__main__':
    TestBitcoinIpcCli(__file__).main()
