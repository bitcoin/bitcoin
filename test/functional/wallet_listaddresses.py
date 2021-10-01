#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listaddresses RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal
)
from random import randrange

class ListAddressesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_addresses(self, wallet, address_type = None):

        new_address_list = []
        count_addr = 0

        for i in range (10, randrange(15,20)):
            new_address_list.append(wallet.getnewaddress('', address_type))

        list_addresses = wallet.listaddresses(address_type)

        for i in range(0, len(list_addresses["receive_addresses"])):
            rec_addr_item = list_addresses['receive_addresses'][i]
            rec_addr = rec_addr_item['address']
            if rec_addr in new_address_list:
                count_addr = count_addr + 1

                rec_addr_info = wallet.getaddressinfo(rec_addr)

                assert_equal(rec_addr_item['hdkeypath'], rec_addr_info['hdkeypath'])
                assert_equal(rec_addr_item['desc'], rec_addr_info['desc'])
                assert_equal(rec_addr_item['internal'], rec_addr_info['ischange'])

        assert_equal(count_addr, len(new_address_list))

        for i in range(0, len(list_addresses["change_addresses"])):
            chg_addr_item = list_addresses['change_addresses'][i]
            chg_addr = chg_addr_item['address']

            chg_addr_info = wallet.getaddressinfo(chg_addr)

            assert_equal(chg_addr_item['hdkeypath'], chg_addr_info['hdkeypath'])
            assert_equal(chg_addr_item['desc'], chg_addr_info['desc'])
            assert_equal(chg_addr_item['internal'], chg_addr_info['ischange'])

    def run_test(self):

        wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.test_addresses(wallet)
        self.test_addresses(wallet, "legacy")
        self.test_addresses(wallet, "p2sh-segwit")
        self.test_addresses(wallet, "bech32")

if __name__ == '__main__':
    ListAddressesTest().main()
