#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the sendmany RPC command."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class SendmanyTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_sffo_repeated_address(self):
        addr_1 = self.wallet.getnewaddress()
        addr_2 = self.wallet.getnewaddress()
        addr_3 = self.wallet.getnewaddress()

        self.log.info("Test using duplicate address in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', duplicated position: 0", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[addr_1, addr_1, addr_1])
        self.log.info("Test using address not present in tx.vout in SFFO argument")
        assert_raises_rpc_error(-8, f"Invalid parameter 'subtract fee from output', destination {addr_3} not found in tx outputs", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[addr_3])
        self.log.info("Test using negative index in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', negative position: -5", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[-5])
        self.log.info("Test using an out of bounds index in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', position too large: 5", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[5])
        self.log.info("Test using an unexpected type in SFFO argument")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', invalid value type: bool", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[False])
        self.log.info("Test duplicates in SFFO argument, mix string destinations with numeric indexes")
        assert_raises_rpc_error(-8, "Invalid parameter 'subtract fee from output', duplicated position: 0", self.def_wallet.sendmany, dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[0, addr_1])
        self.log.info("Test valid mixing of string destinations with numeric indexes in SFFO argument")
        self.def_wallet.sendmany(dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[0, addr_2])

    def run_test(self):
        self.nodes[0].createwallet("activewallet")
        self.wallet = self.nodes[0].get_wallet_rpc("activewallet")
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 101)

        self.test_sffo_repeated_address()


if __name__ == '__main__':
    SendmanyTest(__file__).main()
