#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test meta dex phase 3."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class OmniMetaDexPhase3(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omniactivationallowsender=any']]

    def run_test(self):
        self.log.info("test meta dex phase 3")

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

        # Testing a trade against self that uses divisible / divisible (10.0 OMNI for 100.0 #4)
        txid = self.nodes[0].omni_sendtrade(address, 3, "20", 1, "1.1")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the trade was valid.
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Testing a trade against self that doesn't use Omni to confirm non-Omni pairs are disabled (4.45 #4 for 20 #3)
        txid = self.nodes[0].omni_sendtrade(address, 3, "20", 4, "4.45")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the trade was invalid..."
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], False)

        # Activating trading on all pairs
        activation_block = self.nodes[0].getblockcount() + 8
        txid = self.nodes[0].omni_sendactivation(address, 8, activation_block, 999)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the activation transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Mining 10 blocks to forward past the activation block
        self.nodes[0].generatetoaddress(10, coinbase_address)

        # Checking the activation went live as expected...
        featureid = self.nodes[0].omni_getactivations()['completedactivations']
        allpairs = False
        for ids in featureid:
            if ids['featureid'] == 8:
                allpairs = True
        assert_equal(allpairs, True)

        # Testing a trade against self that doesn't use Omni to confirm non-Omni pairs are now enabled (4.45 #4 for 20 #3)
        txid = self.nodes[0].omni_sendtrade(address, 3, "20", 4, "4.45")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the trade was valid..."
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

if __name__ == '__main__':
    OmniMetaDexPhase3().main()
