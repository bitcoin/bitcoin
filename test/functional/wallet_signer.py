#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test external signer.

Verify that a bitcoind node can use an external signer command
"""
import os
import platform

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class SignerTest(BitcoinTestFramework):
    def mock_signer_path(self):
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'mocks', 'signer.py')
        if platform.system() == "Windows":
            return "py " + path
        else:
            return path

    def set_test_params(self):
        self.num_nodes = 3

        self.extra_args = [
            [],
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
            [f"-signer={self.mock_signer_path()}", '-keypool=10'],
            ["-signer=fake.py"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_external_signer()

    def set_mock_result(self, node, res):
        with open(os.path.join(node.cwd, "mock_result"), "w", encoding="utf8") as f:
            f.write(res)

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def run_test(self):
        self.log.debug(f"-signer={self.mock_signer_path()}")

        assert_raises_rpc_error(-4, 'Error: restart bitcoind with -signer=<cmd>',
            self.nodes[0].enumeratesigners
        )

        # Handle script missing:
        assert_raises_rpc_error(-1, 'execve failed: No such file or directory',
            self.nodes[2].enumeratesigners
        )

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'RunCommandParseJSON error',
            self.nodes[1].enumeratesigners
        )
        self.clear_mock_result(self.nodes[1])

        self.set_mock_result(self.nodes[1], '0 [{"type": "trezor", "model": "trezor_t", "error": "fingerprint not found"}]')
        assert_raises_rpc_error(-4, 'fingerprint not found',
            self.nodes[1].enumeratesigners
        )
        self.clear_mock_result(self.nodes[1])

        # Create new wallets for an external signer.
        # disable_private_keys and descriptors must be true:
        assert_raises_rpc_error(-4, "Private keys must be disabled when using an external signer", self.nodes[1].createwallet, wallet_name='not_hww', disable_private_keys=False, descriptors=True, external_signer=True)
        if self.is_bdb_compiled():
            assert_raises_rpc_error(-4, "Descriptor support must be enabled when using an external signer", self.nodes[1].createwallet, wallet_name='not_hww', disable_private_keys=True, descriptors=False, external_signer=True)
        else:
            assert_raises_rpc_error(-4, "Compiled without bdb support (required for legacy wallets)", self.nodes[1].createwallet, wallet_name='not_hww', disable_private_keys=True, descriptors=False, external_signer=True)

        self.nodes[1].createwallet(wallet_name='hww', disable_private_keys=True, descriptors=True, external_signer=True)
        hww = self.nodes[1].get_wallet_rpc('hww')

        result = hww.enumeratesigners()
        assert_equal(len(result['signers']), 2)
        assert_equal(result['signers'][0]["fingerprint"], "00000001")
        assert_equal(result['signers'][0]["name"], "trezor_t")

        # Flag can't be set afterwards (could be added later for non-blank descriptor based watch-only wallets)
        self.nodes[1].createwallet(wallet_name='not_hww', disable_private_keys=True, descriptors=True, external_signer=False)
        not_hww = self.nodes[1].get_wallet_rpc('not_hww')
        assert_raises_rpc_error(-8, "Wallet flag is immutable: external_signer", not_hww.setwalletflag, "external_signer", True)

if __name__ == '__main__':
    SignerTest().main()
