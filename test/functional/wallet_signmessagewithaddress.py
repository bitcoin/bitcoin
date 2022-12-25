#!/usr/bin/env python3
# Copyright (c) 2016-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Wallet commands for signing and verifying messages."""

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
)

class SignMessagesWithAddressTest(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-addresstype=legacy"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        message = 'This is just a test message'

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
        # signmessage has two required parameters
        for num_params in [0, 1, 3, 4, 5]:
            param_list = ["dummy"]*num_params
            assert_raises_rpc_error(-1, "signmessage", self.nodes[0].signmessage, *param_list)
        # invalid key or address provided
        assert_raises_rpc_error(-5, "Invalid address", self.nodes[0].signmessage, "invalid_addr", message)


if __name__ == '__main__':
    SignMessagesWithAddressTest().main()
