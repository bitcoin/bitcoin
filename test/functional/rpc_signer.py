#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
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
        self.skip_if_no_wallet()

    def set_mock_result(self, node, res):
        with open(os.path.join(node.cwd, "mock_result"), "w") as f:
            f.write(res)

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def set_mock_rogue_mode(self, node, mode):
        with open(os.path.join(node.cwd, "mock_rogue_mode"), "w") as f:
            f.write(mode)

    def clear_mock_rogue_mode(self, node):
        os.remove(os.path.join(node.cwd, "mock_rogue_mode"))

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

        self.test_psbt_output_invariants()

    def test_psbt_output_invariants(self):
        self.log.info('Test PSBT output invariants against a rogue external signer')

        self.nodes[2].createwallet(wallet_name='hww_rogue_test', disable_private_keys=True, blank=False, external_signer=True)
        hww = self.nodes[2].get_wallet_rpc('hww_rogue_test')

        # Fund the wallet with one UTXO per sub-test so each has a fresh coin
        # available even if a previous attempt locked the UTXO.
        address = hww.getnewaddress(address_type='bech32m')
        for _ in range(7):
            self.nodes[0].sendtoaddress(address, 1)
        self.generate(self.nodes[0], 1)
        assert_equal(hww.getbalance(), 7)

        self.restart_node(2, [f"-signer={self.mock_signer_path()}", '-keypool=10'])
        self.nodes[2].loadwallet('hww_rogue_test')
        hww = self.nodes[2].get_wallet_rpc('hww_rogue_test')

        dest = self.nodes[0].getnewaddress()

        self.log.info('Rogue signer modifies output amount — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'change_amount')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

        self.log.info('Rogue signer modifies output script — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'change_script')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

        self.log.info('Rogue signer removes an output — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'remove_output')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

        self.log.info('Rogue signer uses SIGHASH_NONE — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'sighash_none')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

        self.log.info('Rogue signer attaches taproot keypath sig with unsafe sighash — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'tap_key_sig_unsafe')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

        self.log.info('Rogue signer attaches taproot scriptpath sig with unsafe sighash — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'tap_script_sig_unsafe')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

        self.log.info('Rogue signer attaches ECDSA partial sig with unsafe sighash — must be rejected')
        self.set_mock_rogue_mode(self.nodes[2], 'ecdsa_partial_sig_unsafe')
        assert_raises_rpc_error(-25, 'External signer failed to sign',
            hww.send, outputs={dest: 0.5})
        self.clear_mock_rogue_mode(self.nodes[2])

if __name__ == '__main__':
    RPCSignerTest(__file__).main()
