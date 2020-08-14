#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Omni deactivation."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class OmniDeactivation(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omniactivationallowsender=any']]

    def run_test(self):
        self.log.info("test deactivation")

        # Preparing some mature Bitcoins
        coinbase_address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(101, coinbase_address)

        # Obtaining a master address to work with
        address = self.nodes[0].getnewaddress()

        # Funding the address with some testnet BTC for fees
        self.nodes[0].sendtoaddress(address, 20)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Participating in the Exodus crowdsale to obtain some OMNI
        txid = self.nodes[0].sendmany("", {"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP": 10, address: 4})
        self.nodes[0].generatetoaddress(10, coinbase_address)

        # Checking the transaction was valid.
        result = self.nodes[0].gettransaction(txid)
        assert_equal(result['confirmations'], 10)

        # Creating an indivisible test property
        self.nodes[0].omni_sendissuancefixed(address, 1, 1, 0, "Z_TestCat", "Z_TestSubCat", "Z_IndivisTestProperty", "Z_TestURL", "Z_TestData", "10000000")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Creating a divisible test property
        self.nodes[0].omni_sendissuancefixed(address, 1, 2, 0, "Z_TestCat", "Z_TestSubCat", "Z_DivisTestProperty", "Z_TestURL", "Z_TestData", "10000")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Activating the fee system & all pair trading, and testing they went live:
        # Sending the all pair activation & checking it was valid...
        activation_block = self.nodes[0].getblockcount() + 8
        txid = self.nodes[0].omni_sendactivation(address, 8, activation_block, 999)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Mining 10 blocks to forward past the activation block
        self.nodes[0].generatetoaddress(10, coinbase_address)

        # Checking the activation went live as expected...
        featureid = self.nodes[0].omni_getactivations()['completedactivations']
        found = False
        for ids in featureid:
            if ids['featureid'] == 8:
                found = True
        assert_equal(found, True)

        # Trying an all pair trade to make sure it is valid
        txid = self.nodes[0].omni_sendtrade(address, 3, "2000", 4, "1.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction is valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Check that the order book is not empty for an all pair trade
        orders = self.nodes[0].omni_getorderbook(3, 4)
        if not orders:
            raise AssertionError("Orderbook should not be empty.")

        # Deactivating the fee system & testing it's now disabled...
        # Sending the deactivation & checking it was valid...
        txid = self.nodes[0].omni_senddeactivation(address, 8)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Check that the order book is empty for an all pair trade
        orders = self.nodes[0].omni_getorderbook(3, 4)
        if orders:
            raise AssertionError("Orderbook should be empty.")

        # Trying an all pair trade to make sure it fails
        txid = self.nodes[0].omni_sendtrade(address, 3, "2000", 4, "1.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction is now invalid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Deactivating the MetaDEx completely & testing it's now disabled...

        # Sending a trade of #3 for #1 & checking it was valid...
        txid = self.nodes[0].omni_sendtrade(address, 3, "2000", 1, "1.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction is valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Sending a trade of #4 for #1 & checking it was valid...
        txid = self.nodes[0].omni_sendtrade(address, 4, "2000", 1, "1.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction is valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Sending the deactivation & checking it was valid...
        txid = self.nodes[0].omni_senddeactivation(address, 2)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Check that the order book is empty for a Omni trade
        orders = self.nodes[0].omni_getorderbook(3, 1)
        if orders:
            raise AssertionError("Orderbook should be empty.")

        # Check that the order book is empty for a Omni trade
        orders = self.nodes[0].omni_getorderbook(4, 1)
        if orders:
            raise AssertionError("Orderbook should be empty.")

        # Checking the trade for #3/#1 was refunded...
        balance = self.nodes[0].omni_getbalance(address, 3)['balance']
        assert_equal(balance, "10000000")

        # Checking the trade for #4/#1 was refunded...
        balance = self.nodes[0].omni_getbalance(address, 4)['balance']
        assert_equal(balance, "10000.00000000")

        # Sending a trade of #3 for #1 & checking it was invalid...
        txid = self.nodes[0].omni_sendtrade(address, 3, "2000", 1, "1.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction is invalid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Sending a trade of #4 for #1 & checking it was invalid...
        txid = self.nodes[0].omni_sendtrade(address, 1, "1.0", 4, "1.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction is invalid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], False)

if __name__ == '__main__':
    OmniDeactivation().main()
