#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC commands for signing and verifying messages."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class SignMessagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-addresstype=legacy"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        message = 'This is just a test message'

        self.log.info('test signing with priv_key')
        priv_key = 'cUeKHd5orzT3mz8P9pxyREHfsWtVfgsfDjiZZBcjUBAaGk1BTj7N'
        address = 'mpLQjfK79b7CCV4VMJWEWAj5Mpx8Up5zxB'
        expected_signature = 'INbVnW4e6PeRmsv2Qgu8NuopvrVjkcxob+sX8OcZG0SALhWybUjzMLPdAsXI46YZGb0KQTRii+wWIQzRpG/U+S0='
        signature = self.nodes[0].signmessagewithprivkey(priv_key, message)
        assert_equal(expected_signature, signature)
        assert self.nodes[0].verifymessage(address, signature, message)

        self.log.info('test signing with an address with wallet')
        address = self.nodes[0].getnewaddress()
        signature = self.nodes[0].signmessage(address, message)
        assert self.nodes[0].verifymessage(address, signature, message)

        self.log.info('test verifying with another address should not work')
        other_address = self.nodes[0].getnewaddress()
        other_signature = self.nodes[0].signmessage(other_address, message)
        assert not self.nodes[0].verifymessage(other_address, signature, message)
        assert not self.nodes[0].verifymessage(address, other_signature, message)

        self.log.info('test parameter validity and error codes')
        # signmessage(withprivkey) have two required parameters
        for num_params in [0, 1, 3, 4, 5]:
            param_list = ["dummy"]*num_params
            assert_raises_rpc_error(-1, "signmessagewithprivkey", self.nodes[0].signmessagewithprivkey, *param_list)
            assert_raises_rpc_error(-1, "signmessage", self.nodes[0].signmessage, *param_list)
        # verifymessage has three required parameters
        for num_params in [0, 1, 2, 4, 5]:
            param_list = ["dummy"]*num_params
            assert_raises_rpc_error(-1, "verifymessage", self.nodes[0].verifymessage, *param_list)
        # invalid key or address provided
        assert_raises_rpc_error(-5, "Invalid private key", self.nodes[0].signmessagewithprivkey, "invalid_key", message)
        assert_raises_rpc_error(-5, "Invalid address", self.nodes[0].signmessage, "invalid_addr", message)
        assert_raises_rpc_error(-5, "Invalid address", self.nodes[0].verifymessage, "invalid_addr", signature, message)
        # malformed signature provided
        assert_raises_rpc_error(-3, "Malformed base64 encoding", self.nodes[0].verifymessage, self.nodes[0].getnewaddress(), "invalid_sig", message)

if __name__ == '__main__':
    SignMessagesTest().main()
