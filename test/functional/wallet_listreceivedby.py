#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listreceivedbyaddress RPC."""
from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_array_result,
                                 assert_equal,
                                 assert_raises_rpc_error,
                                 )

class ReceivedByTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def run_test(self):
        # Generate block to get out of IBD
        self.nodes[0].generate(1)

        self.log.info("listreceivedbyaddress Test")

        # Send from node 0 to 1
        addr = self.nodes[1].getnewaddress()
        txid = self.nodes[0].sendtoaddress(addr, 0.1)
        self.sync_all()

        # Check not listed in listreceivedbyaddress because has 0 confirmations
        assert_array_result(self.nodes[1].listreceivedbyaddress(),
                            {"address": addr},
                            {},
                            True)
        # Bury Tx under 10 block so it will be returned by listreceivedbyaddress
        self.nodes[1].generate(10)
        self.sync_all()
        assert_array_result(self.nodes[1].listreceivedbyaddress(),
                            {"address": addr},
                            {"address": addr, "account": "", "amount": Decimal("0.1"), "confirmations": 10, "txids": [txid, ]})
        # With min confidence < 10
        assert_array_result(self.nodes[1].listreceivedbyaddress(5),
                            {"address": addr},
                            {"address": addr, "account": "", "amount": Decimal("0.1"), "confirmations": 10, "txids": [txid, ]})
        # With min confidence > 10, should not find Tx
        assert_array_result(self.nodes[1].listreceivedbyaddress(11), {"address": addr}, {}, True)

        # Empty Tx
        addr = self.nodes[1].getnewaddress()
        assert_array_result(self.nodes[1].listreceivedbyaddress(0, True),
                            {"address": addr},
                            {"address": addr, "account": "", "amount": 0, "confirmations": 0, "txids": []})

        self.log.info("getreceivedbyaddress Test")

        # Send from node 0 to 1
        addr = self.nodes[1].getnewaddress()
        txid = self.nodes[0].sendtoaddress(addr, 0.1)
        self.sync_all()

        # Check balance is 0 because of 0 confirmations
        balance = self.nodes[1].getreceivedbyaddress(addr)
        assert_equal(balance, Decimal("0.0"))

        # Check balance is 0.1
        balance = self.nodes[1].getreceivedbyaddress(addr, 0)
        assert_equal(balance, Decimal("0.1"))

        # Bury Tx under 10 block so it will be returned by the default getreceivedbyaddress
        self.nodes[1].generate(10)
        self.sync_all()
        balance = self.nodes[1].getreceivedbyaddress(addr)
        assert_equal(balance, Decimal("0.1"))

        # Trying to getreceivedby for an address the wallet doesn't own should return an error
        assert_raises_rpc_error(-4, "Address not found in wallet", self.nodes[0].getreceivedbyaddress, addr)

        self.log.info("listreceivedbyaccount + getreceivedbyaccount Test")

        # set pre-state
        addrArr = self.nodes[1].getnewaddress()
        account = self.nodes[1].getaccount(addrArr)
        received_by_account_json = [r for r in self.nodes[1].listreceivedbyaccount() if r["account"] == account][0]
        balance_by_account = self.nodes[1].getreceivedbyaccount(account)

        txid = self.nodes[0].sendtoaddress(addr, 0.1)
        self.sync_all()

        # listreceivedbyaccount should return received_by_account_json because of 0 confirmations
        assert_array_result(self.nodes[1].listreceivedbyaccount(),
                            {"account": account},
                            received_by_account_json)

        # getreceivedbyaddress should return same balance because of 0 confirmations
        balance = self.nodes[1].getreceivedbyaccount(account)
        assert_equal(balance, balance_by_account)

        self.nodes[1].generate(10)
        self.sync_all()
        # listreceivedbyaccount should return updated account balance
        assert_array_result(self.nodes[1].listreceivedbyaccount(),
                            {"account": account},
                            {"account": received_by_account_json["account"], "amount": (received_by_account_json["amount"] + Decimal("0.1"))})

        # getreceivedbyaddress should return updates balance
        balance = self.nodes[1].getreceivedbyaccount(account)
        assert_equal(balance, balance_by_account + Decimal("0.1"))

        # Create a new account named "mynewaccount" that has a 0 balance
        self.nodes[1].getaccountaddress("mynewaccount")
        received_by_account_json = [r for r in self.nodes[1].listreceivedbyaccount(0, True) if r["account"] == "mynewaccount"][0]

        # Test includeempty of listreceivedbyaccount
        assert_equal(received_by_account_json["amount"], Decimal("0.0"))

        # Test getreceivedbyaccount for 0 amount accounts
        balance = self.nodes[1].getreceivedbyaccount("mynewaccount")
        assert_equal(balance, Decimal("0.0"))

if __name__ == '__main__':
    ReceivedByTest().main()
