#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test meta dex prices."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class OmniMetaDexPrices(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("test meta dex prices")

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

        # Creating another indivisible test property
        self.nodes[0].omni_sendissuancefixed(address, 1, 1, 0, "Z_TestCat", "Z_TestSubCat", "Z_IndivisTestProperty", "Z_TestURL", "Z_TestData", "10000000")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Testing a trade against self that uses divisible / divisible (10.0 OMNI for 100.0 #4)
        txid = self.nodes[0].omni_sendtrade(address, 1, "10.0", 4, "100.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the unit price was 10.0...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['unitprice'], "10.00000000000000000000000000000000000000000000000000")

        # Testing a trade against self that uses divisible / indivisible (10.0 OMNI for 100 #3))
        txid = self.nodes[0].omni_sendtrade(address, 1, "10.0", 3, "100")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the unit price was 10.0...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['unitprice'], "10.00000000000000000000000000000000000000000000000000")

        # Testing a trade against self that uses indivisible / divisible (10 #3 for 100.0 OMNI)
        txid = self.nodes[0].omni_sendtrade(address, 3, "10", 1, "100.0")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the unit price was 10.0...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['unitprice'], "10.00000000000000000000000000000000000000000000000000")

        # Testing a trade against self that uses indivisible / indivisible (10 #5 for 100 #3)
        txid = self.nodes[0].omni_sendtrade(address, 5, "10", 3, "100")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the unit price was 10.0...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['unitprice'], "10.00000000000000000000000000000000000000000000000000")

if __name__ == '__main__':
    OmniMetaDexPrices().main()
