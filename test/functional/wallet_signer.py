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
            return "\"python " + path + "\""
        else:
            return path

    def set_test_params(self):
        self.num_nodes = 5

        self.extra_args = [
            [],
            ['-signer=%s' % self.mock_signer_path() , '-addresstype=bech32'],
            ['-signer=%s' % self.mock_signer_path(), '-addresstype=p2sh-segwit'],
            ['-signer=%s' % self.mock_signer_path(), '-addresstype=legacy'],
            ['-signer=%s' % "fake.py"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_mock_result(self, node, res):
        f = open(os.path.join(node.cwd, "mock_result"), "w", encoding="utf8")
        f.write(res)
        f.close()

    def clear_mock_result(self, node):
        os.remove(os.path.join(node.cwd, "mock_result"))

    def run_test(self):
        self.skip_if_no_runcommand(self.nodes[0])
        self.log.info('-signer=%s' % self.mock_signer_path())
        assert_equal(self.nodes[0].getbalance(), 1250)
        assert_equal(self.nodes[1].getbalance(), 1250)
        assert_raises_rpc_error(-4, 'Error: restart bitcoind with -signer=<cmd>',
            self.nodes[0].enumeratesigners
        )

        # Handle script missing:
        assert_raises_rpc_error(-1, 'execve failed: No such file or directory',
            self.nodes[4].enumeratesigners
        )

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            self.nodes[1].enumeratesigners
        )
        self.clear_mock_result(self.nodes[1])

        # Create new wallets with private keys disabled:
        self.nodes[1].createwallet('hww1', True)
        hww1 = self.nodes[1].get_wallet_rpc('hww1')
        self.nodes[2].createwallet('hww2', True)
        hww2 = self.nodes[2].get_wallet_rpc('hww2')
        self.nodes[3].createwallet('hww3', True)
        hww3 = self.nodes[3].get_wallet_rpc('hww3')

        result = hww1.enumeratesigners()
        assert_equal(len(result['signers']), 2)
        hww2.enumeratesigners()
        # Delay enumerate on third wallet to test error handling
        # hww3.enumeratesigners()

        self.log.info('Test signerfetchkeys with bech32, p2sh-segwit and legacy')

        result = hww1.signerfetchkeys(0, "00000001")
        assert_equal(result, [{'success': True}, {'success': True}])
        assert_equal(hww1.getwalletinfo()["keypoolsize"], 1)

        assert_raises_rpc_error(-4, "First call enumeratesigners", hww3.signerfetchkeys)
        hww3.enumeratesigners()

        assert_raises_rpc_error(-4, "Multiple signers found, please specify which to use", hww3.signerfetchkeys)

        # Handle error thrown by script
        self.set_mock_result(self.nodes[1], "2")
        assert_raises_rpc_error(-1, 'Unable to parse JSON',
            hww1.signerfetchkeys, 0, "00000001"
        )
        self.clear_mock_result(self.nodes[1])

        address1 = hww1.getnewaddress()
        assert_equal(address1, "bcrt1qm90ugl4d48jv8n6e5t9ln6t9zlpm5th68x4f8g")
        address_info = hww1.getaddressinfo(address1)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], False)
        assert_equal(address_info['hdkeypath'], "m/84'/1'/0'/0/0")

        result = hww2.signerfetchkeys(0, "00000001")
        assert_equal(result, [{'success': True}, {'success': True}])
        assert_equal(hww2.getwalletinfo()["keypoolsize"], 1)

        address2 = hww2.getnewaddress()
        assert_equal(address2, "2N2gQKzjUe47gM8p1JZxaAkTcoHPXV6YyVp")
        address_info = hww2.getaddressinfo(address2)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], False)
        assert_equal(address_info['hdkeypath'], "m/49'/1'/0'/0/0")

        result = hww3.signerfetchkeys(0, "00000001")
        assert_equal(result, [{'success': True}, {'success': True}])
        assert_equal(hww3.getwalletinfo()["keypoolsize"], 1)

        address3 = hww3.getnewaddress("00000001")
        assert_equal(address3, "n1LKejAadN6hg2FrBXoU1KrwX4uK16mco9")
        address_info = hww3.getaddressinfo(address3)
        assert_equal(address_info['solvable'], True)
        assert_equal(address_info['ismine'], False)
        assert_equal(address_info['hdkeypath'], "m/44'/1'/0'/0/0")

if __name__ == '__main__':
    SignerTest().main()
