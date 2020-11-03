#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test DEx versions spec using free DEx."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.authproxy import JSONRPCException

# Transaction format tests for traditional DEx offer
#
# Traditional DEx orders with version 0 have an implicit action value,
# based on the global state, whereby transactions with version 1 have an
# explicit action value. Other versions are currently not valid.

class OmniFreeDExVersionsSpec(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omniactivationallowsender=any']]

    def run_test(self):
        self.log.info("test dex versions spec using free dex")

        # Preparing some mature Bitcoins
        coinbase_address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(110, coinbase_address)

        # Obtaining a master address to work with
        address = self.nodes[0].getnewaddress()

        # Funding the address with some testnet BTC for fees
        self.nodes[0].sendtoaddress(address, 10)
        self.nodes[0].sendtoaddress(address, 10)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Activating Free DEx...
        activation_block = self.nodes[0].getblockcount() + 8
        txid = self.nodes[0].omni_sendactivation(address, 15, activation_block, 0)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Mining 10 blocks to forward past the activation block
        self.nodes[0].generatetoaddress(10, coinbase_address)

        # Checking the activation went live as expected...
        featureid = self.nodes[0].omni_getactivations()['completedactivations']
        freeDEx = False
        for ids in featureid:
            if ids['featureid'] == 15:
                freeDEx = True
        assert_equal(freeDEx, True)

        # Create a fixed property
        txid = self.nodes[0].omni_sendissuancefixed(address, 1, 2, 0, "Test Category", "Test Subcategory", "TST", "http://www.omnilayer.org", "This is a test for managed properties", "100000")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)

        # Get currency ID
        currencyOffered = result['propertyid']

        # DEx transactions with version 0 have no explicit action value

        # Created funded address for test
        fundedAddress = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(fundedAddress, "0.1")
        txid = self.nodes[0].omni_send(address, fundedAddress, currencyOffered, "0.5")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        #    {
        #        "version": 0,
        #        "type_int": 20,
        #        "propertyid": 3,
        #        "amount": "0.40000000",currently
        #        "bitcoindesired": "0.80000000",
        #        "timelimit": 15,
        #        "feerequired": "0.00000100"
        #    }

        payload = "00000014000000030000000002625a000000000004c4b4000f0000000000000064"

        offerTxid = self.nodes[0].omni_sendrawtx(fundedAddress, payload)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction...
        offerTx = self.nodes[0].omni_gettransaction(offerTxid)
        assert_equal(offerTx['valid'], True)
        assert_equal(offerTx['action'], "new") # implicit

        # DEx transactions with version 1 have an explicit action value

        # Created funded address for test
        fundedAddress = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(fundedAddress, "0.1")
        txid = self.nodes[0].omni_send(address, fundedAddress, currencyOffered, "0.3")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        #    {
        #        "version": 1,
        #        "type_int": 20,
        #        "propertyid": 3,
        #        "amount": "0.10000000",
        #        "bitcoindesired": "0.10000000",
        #        "timelimit": 20,
        #        "feerequired": "0.00000000",
        #        "action": 1
        #    }

        payload = "00010014000000030000000000989680000000000098968014000000000000000001"

        offerTxid = self.nodes[0].omni_sendrawtx(fundedAddress, payload)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction...
        offerTx = self.nodes[0].omni_gettransaction(offerTxid)
        assert_equal(offerTx['valid'], True)
        assert_equal(offerTx['action'], "new") # explicit

        # DEx transactions with version 1 must be sent with action value

        # Created funded address for test
        fundedAddress = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(fundedAddress, "0.1")
        txid = self.nodes[0].omni_send(address, fundedAddress, currencyOffered, "0.3")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        #    {
        #        "version": 1,
        #        "type_int": 20,
        #        "propertyid": 3,
        #        "amount": "0.30000000",
        #        "bitcoindesired": "0.30000000",
        #        "timelimit": 7,
        #        "feerequired": "0.00000050"
        #    }

        payload = "00010014000000030000000001c9c3800000000001c9c380070000000000000032"

        offerTxid = self.nodes[0].omni_sendrawtx(fundedAddress, payload)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Transaction should be invalid, omni_gettransaction will throw
        thrown = False
        try:
            offerTx = self.nodes[0].omni_gettransaction(offerTxid)
        except JSONRPCException:
            thrown = True
        assert_equal(thrown, True)

        # DEx transactions with versions greater than 1 are currently not valid

        # Created funded address for test
        fundedAddress = self.nodes[0].getnewaddress()
        self.nodes[0].sendtoaddress(fundedAddress, "0.1")
        txid = self.nodes[0].omni_send(address, fundedAddress, currencyOffered, "0.3")
        self.nodes[0].generatetoaddress(1, coinbase_address)

        #    {
        #        "version": 2,
        #        "type_int": 20,
        #        "propertyid": 3,
        #        "amount": "0.00000001",
        #        "bitcoindesired": "0.00000001",
        #        "timelimit": 255,
        #        "feerequired": "0.00000001",
        #        "action": 1
        #    }

        payload = "000200140000000300000000000000010000000000000001ff000000000000000101"

        offerTxid = self.nodes[0].omni_sendrawtx(fundedAddress, payload)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Checking the transaction...
        offerTx = self.nodes[0].omni_gettransaction(offerTxid)
        assert_equal(offerTx['valid'], False)

if __name__ == '__main__':
    OmniFreeDExVersionsSpec().main()
