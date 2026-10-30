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

import os
import subprocess
import sys

class TestBitcoinIpcCli(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()

    def test_cli(self, args, error=None):
        # Intentionally set wrong RPC password so only IPC not HTTP connections work
        args = [self.binary_paths.bitcoincli, f"-datadir={self.nodes[0].datadir_path}", "-rpcpassword=wrong"] + args + ["echo", "foo"]
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        self.log.debug(f"bitcoin-cli returncode={result.returncode} stdout={result.stdout!r}")
        if error is None:
            assert_equal(result.stdout, '[\n  "foo"\n]\n')
        elif isinstance(error, tuple):
            # Accept any of the alternative error messages.
            assert any(result.stdout.startswith(e) for e in error), \
                f"Output didn't start with any expected error {error!r}:\n{result.stdout}"
        else:
            assert result.stdout.startswith(error), f"Output didn't start with the expected error {error!r}:\n{result.stdout}"
        assert_equal(result.stderr, None)
        assert_equal(result.returncode, 0 if error is None else 1)

    def run_test(self):
        node = self.nodes[0]
        # Diagnostic logging retained for debugging Windows IPC path issues.
        # fwd= logs the forward-slash form separately since MinGW may need forward
        # slashes where MSVC uses backslashes; encoded_len= checks the 108-byte limit.
        ipc_path_str = str(node.ipc_socket_path)
        ipc_path_fwd = ipc_path_str.replace('\\', '/')
        ipc_path_enc_len = len(os.fsencode(node.ipc_socket_path))
        self.log.info(f"ipc_socket_path={ipc_path_str!r} fwd={ipc_path_fwd!r} encoded_len={ipc_path_enc_len}")
        self.log.info(f"ipc_socket_path parent exists={node.ipc_socket_path.parent.exists()} socket exists={node.ipc_socket_path.exists()}")
        if node.ipc_tmp_dir:
            self.log.info(f"ipc_tmp_dir={node.ipc_tmp_dir.name!r}")
            self.log.info("Skipping a few checks because temporary directory path is too long")

        http_auth_error = "error: Authorization failed: Incorrect rpcuser or rpcpassword were specified."
        http_connect_error = f"error: Error while attempting to communicate with server 127.0.0.1:{rpc_port(node.index)} (Could not connect to the server)\n\nMake sure the bitcoind server is running and that you are connecting to the correct RPC port.\nUse \"bitcoin-cli -help\" for more info.\n"
        # On POSIX, connect() to a stopped node returns ECONNREFUSED and bitcoin-cli
        # appends help text; match the full expected output. On Windows, WSAECONNREFUSED
        # produces a different OS string with no appended help text, so match only a prefix.
        # Also on Windows, bitcoind removes the socket file on clean shutdown (on Linux the
        # file persists and connect() still returns ECONNREFUSED), so a subsequent connect
        # sees ENOENT ("No such file or directory") instead; accept either Windows string.
        # TODO: clarify whether bitcoind explicitly removes the socket on shutdown on all
        # platforms, or only Windows, and whether the POSIX test could also see ENOENT.
        ipc_connect_error = (
            ("error: No connection could be made because the target machine actively refused it",
             "error: No such file or directory")
            if sys.platform == 'win32' else
            "error: Connection refused\n\nProbably bitcoin-node is not running or not listening on a unix socket. Can be started with:\n\n    bitcoin-node -chain=regtest -ipcbind=unix\n"
        )
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

            # Log socket state to diagnose ENOENT vs ECONNREFUSED behaviour on Windows.
            self.log.info(f"started={started} socket exists={node.ipc_socket_path.exists()}")
            self.test_cli([f"-ipcconnect=unix:{node.ipc_socket_path}"], ipc_error)
            self.test_cli(["-noipcconnect"], http_error)
            self.test_cli(["-ipcconnect=unix", "-rpcconnect=127.0.0.1"], ipc_http_conflict)

            self.stop_node(0)


if __name__ == '__main__':
    TestBitcoinIpcCli(__file__).main()
