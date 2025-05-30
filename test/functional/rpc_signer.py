#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test external signer.

Verify that a bitcoind node can use an external signer command.
See also wallet_signer.py for tests that require wallet context.
"""
import os
import platform
import sys

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class RPCSignerTest(BitcoinTestFramework):
    def mock_signer_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'signer.py')
        return sys.executable + " " + path

    def set_test_params(self):
        self.num_nodes = 4

        self.extra_args = [
            [],
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
            ["-signer=fake.py"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_external_signer()

    def set_mock_result(self, node, res):
        with open(os.path.join(node.cwd, "mock_result"), "w", encoding="utf8") as f:
            f.write(res)

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def run_test(self):
        self.log.debug(f"-signer={self.mock_signer_path()}")

        assert_raises_rpc_error(-1, 'Error: restart bitcoind with -signer=<cmd>',
            self.nodes[0].enumeratesigners
        )

        # Handle script missing:
        assert_raises_rpc_error(
            -1,
            "CreateProcess failed: The system cannot find the file specified."
            if platform.system() == "Windows"
            else "execve failed: No such file or directory",
            self.nodes[3].enumeratesigners,
        )

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'RunCommandParseJSON error',
            self.nodes[1].enumeratesigners
        )
        self.clear_mock_result(self.nodes[1])

        self.set_mock_result(self.nodes[1], '0 [{"type": "trezor", "model": "trezor_t", "error": "fingerprint not found"}]')
        assert_raises_rpc_error(-1, 'fingerprint not found',
            self.nodes[1].enumeratesigners
        )
        self.clear_mock_result(self.nodes[1])

        assert_equal({'fingerprint': '00000001', 'name': 'trezor_t'} in self.nodes[1].enumeratesigners()['signers'], True)

if __name__ == '__main__':
    RPCSignerTest(__file__).main()
