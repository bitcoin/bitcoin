#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the sendmany RPC command."""

from test_framework.test_framework import BitcoinTestFramework

class SendmanyTest(BitcoinTestFramework):
    # Setup and helpers
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
        self.def_wallet.sendmany(dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[addr_1, addr_1, addr_1])
        self.log.info("Test using address not present in tx.vout in SFFO argument")
        self.def_wallet.sendmany(dummy='', amounts={addr_1: 1, addr_2: 1}, subtractfeefrom=[addr_3])

    def run_test(self):
        self.nodes[0].createwallet("activewallet")
        self.wallet = self.nodes[0].get_wallet_rpc("activewallet")
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 101)

        self.test_sffo_repeated_address()


if __name__ == '__main__':
    SendmanyTest().main()
