#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test non-fungible tokens."""

from test_framework.test_framework import BitcoinTestFramework

from test_framework.authproxy import JSONRPCException
from test_framework.util import assert_equal

class OmniNonFungibleTokensTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def run_test(self):
        # Get address for mining, issuance, granting and sending tokens to.
        token_address = self.nodes[0].getnewaddress()
        grant_address = self.nodes[0].getnewaddress()
        destination_address = self.nodes[0].getnewaddress()

        # Fund issuance address
        self.nodes[0].generatetoaddress(110, token_address)

        # Create test token
        txid = self.nodes[0].omni_sendissuancemanaged(token_address, 2, 5, 0, "", "", "TESTTOKEN", "", "")
        self.nodes[0].generatetoaddress(1, token_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        property_id = result["propertyid"]

        # Send tokens we do not have yet
        try:
            self.nodes[0].omni_sendnonfungible(token_address, destination_address, property_id, 1, 1)
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Sender has insufficient balance" in errorString)
        errorString = ""

        # Grant tokens to creator
        txid = self.nodes[0].omni_sendgrant(token_address, "", property_id, "100", "")
        self.nodes[0].generatetoaddress(1, token_address)

        # Send tokens out of range start
        try:
            self.nodes[0].omni_sendnonfungible(token_address, destination_address, property_id, 0, 10)
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Sender does not own the range" in errorString)
        errorString = ""

        # Send tokens out of range end
        try:
            self.nodes[0].omni_sendnonfungible(token_address, destination_address, property_id, 100, 101)
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Sender does not own the range" in errorString)
        errorString = ""

        # Get property and check fields
        result = self.nodes[0].omni_getproperty(property_id)
        assert_equal(result['fixedissuance'], False)
        assert_equal(result['managedissuance'], True)
        assert_equal(result['non-fungibletoken'], True)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        assert_equal(result['tokenstart'], '1')
        assert_equal(result['tokenend'], '100')
        assert_equal(result['grantdata'], "")

        result = self.nodes[0].omni_getnonfungibletokens(token_address, property_id)
        assert_equal(result[0]['propertyid'], property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[0]['tokens'][0]['tokenend'], 100)
        assert_equal(result[0]['tokens'][0]['amount'], 100)

        result = self.nodes[0].omni_getnonfungibletokenranges(property_id)
        assert_equal(result[0]['address'], token_address)
        assert_equal(result[0]['tokenstart'], 1)
        assert_equal(result[0]['tokenend'], 100)
        assert_equal(result[0]['amount'], 100)

        # Check data blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 1)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], '')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Grant 1 tokens with data
        txid = self.nodes[0].omni_sendgrant(token_address, "", property_id, "1", "Test grantdata")
        self.nodes[0].generatetoaddress(1, token_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        assert_equal(result['tokenstart'], '101')
        assert_equal(result['tokenend'], '101')
        assert_equal(result['grantdata'], "Test grantdata")

        result = self.nodes[0].omni_getnonfungibletokens(token_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[0]['tokens'][0]['tokenend'], 101)
        assert_equal(result[0]['tokens'][0]['amount'], 101)

        result = self.nodes[0].omni_getnonfungibletokenranges(property_id)
        assert_equal(result[0]['address'], token_address)
        assert_equal(result[0]['tokenstart'], 1)
        assert_equal(result[0]['tokenend'], 101)
        assert_equal(result[0]['amount'], 101)

        # Make sure original token data unchanged
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 1)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], '')

        # Check tokens have expected data set
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')

        # Grant 99 tokens with different data
        txid = self.nodes[0].omni_sendgrant(token_address, "", property_id, "99", "Different grantdata")
        self.nodes[0].generatetoaddress(1, token_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        assert_equal(result['tokenstart'], '102')
        assert_equal(result['tokenend'], '200')
        assert_equal(result['grantdata'], "Different grantdata")

        result = self.nodes[0].omni_getnonfungibletokens(token_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[0]['tokens'][0]['tokenend'], 200)
        assert_equal(result[0]['tokens'][0]['amount'], 200)

        result = self.nodes[0].omni_getnonfungibletokenranges(property_id)
        assert_equal(result[0]['address'], token_address)
        assert_equal(result[0]['tokenstart'], 1)
        assert_equal(result[0]['tokenend'], 200)
        assert_equal(result[0]['amount'], 200)

        # Make sure previous token data unchanged
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 1)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], '')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Make sure previous token data unchanged
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 200)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Grant 100 tokens to different address
        txid = self.nodes[0].omni_sendgrant(token_address, grant_address, property_id, "100", "Multiple grantdata")
        self.nodes[0].generatetoaddress(1, token_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        assert_equal(result['tokenstart'], '201')
        assert_equal(result['tokenend'], '300')
        assert_equal(result['grantdata'], "Multiple grantdata")

        # No change here
        result = self.nodes[0].omni_getnonfungibletokens(token_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[0]['tokens'][0]['tokenend'], 200)
        assert_equal(result[0]['tokens'][0]['amount'], 200)

        # New tokens appear on this address
        result = self.nodes[0].omni_getnonfungibletokens(grant_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 201)
        assert_equal(result[0]['tokens'][0]['tokenend'], 300)
        assert_equal(result[0]['tokens'][0]['amount'], 100)

        # Two addresses now show for holding this token
        result = self.nodes[0].omni_getnonfungibletokenranges(property_id)
        assert_equal(result[0]['address'], token_address)
        assert_equal(result[0]['tokenstart'], 1)
        assert_equal(result[0]['tokenend'], 200)
        assert_equal(result[0]['amount'], 200)
        assert_equal(result[1]['address'], grant_address)
        assert_equal(result[1]['tokenstart'], 201)
        assert_equal(result[1]['tokenend'], 300)
        assert_equal(result[1]['amount'], 100)

        # Check data fields
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 300)
        assert_equal(result[0]['owner'], grant_address)
        assert_equal(result[0]['grantdata'], 'Multiple grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Test sending tokens to a new address
        self.nodes[0].sendtoaddress(destination_address, 1)
        self.nodes[0].sendtoaddress(grant_address, 1)
        self.nodes[0].generatetoaddress(1, token_address)

        # Send token range without data
        self.nodes[0].omni_sendnonfungible(token_address, destination_address, property_id, 1, 10)

        # Send single token with "Test grantdata"
        self.nodes[0].omni_sendnonfungible(token_address, destination_address, property_id, 101, 101)

        # Send token range from different address with "Multiple grantdata"
        self.nodes[0].omni_sendnonfungible(grant_address, destination_address, property_id, 201, 210)
        self.nodes[0].generatetoaddress(1, token_address)

        # Send tokens that has already been sent from source address
        try:
            self.nodes[0].omni_sendnonfungible(token_address, destination_address, property_id, 101, 101)
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Sender does not own the range" in errorString)
        errorString = ""

        # Check range has changed with gap in the middle
        result = self.nodes[0].omni_getnonfungibletokens(token_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 11)
        assert_equal(result[0]['tokens'][0]['tokenend'], 100)
        assert_equal(result[0]['tokens'][0]['amount'], 90)
        assert_equal(result[0]['tokens'][1]['tokenstart'], 102)
        assert_equal(result[0]['tokens'][1]['tokenend'], 200)
        assert_equal(result[0]['tokens'][1]['amount'], 99)

        # Check range has changed
        result = self.nodes[0].omni_getnonfungibletokens(grant_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 211)
        assert_equal(result[0]['tokens'][0]['tokenend'], 300)
        assert_equal(result[0]['tokens'][0]['amount'], 90)

        # New tokens should be present in dest address
        result = self.nodes[0].omni_getnonfungibletokens(destination_address, property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[0]['tokens'][0]['tokenend'], 10)
        assert_equal(result[0]['tokens'][0]['amount'], 10)
        assert_equal(result[0]['tokens'][1]['tokenstart'], 101)
        assert_equal(result[0]['tokens'][1]['tokenend'], 101)
        assert_equal(result[0]['tokens'][1]['amount'], 1)
        assert_equal(result[0]['tokens'][2]['tokenstart'], 201)
        assert_equal(result[0]['tokens'][2]['tokenend'], 210)
        assert_equal(result[0]['tokens'][2]['amount'], 10)

        # Three addresses now show for holding this token
        result = self.nodes[0].omni_getnonfungibletokenranges(property_id)
        assert_equal(result[0]['address'], destination_address)
        assert_equal(result[0]['tokenstart'], 1)
        assert_equal(result[0]['tokenend'], 10)
        assert_equal(result[0]['amount'], 10)
        assert_equal(result[1]['address'], token_address)
        assert_equal(result[1]['tokenstart'], 11)
        assert_equal(result[1]['tokenend'], 100)
        assert_equal(result[1]['amount'], 90)
        assert_equal(result[2]['address'], destination_address)
        assert_equal(result[2]['tokenstart'], 101)
        assert_equal(result[2]['tokenend'], 101)
        assert_equal(result[2]['amount'], 1)
        assert_equal(result[3]['address'], token_address)
        assert_equal(result[3]['tokenstart'], 102)
        assert_equal(result[3]['tokenend'], 200)
        assert_equal(result[3]['amount'], 99)
        assert_equal(result[4]['address'], destination_address)
        assert_equal(result[4]['tokenstart'], 201)
        assert_equal(result[4]['tokenend'], 210)
        assert_equal(result[4]['amount'], 10)
        assert_equal(result[5]['address'], grant_address)
        assert_equal(result[5]['tokenstart'], 211)
        assert_equal(result[5]['tokenend'], 300)
        assert_equal(result[5]['amount'], 90)

        # Check owner and data fields on transfered tokens
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 1)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], '')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 201)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Multiple grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Set data on a non-issuer/holder address
        non_issuer_address = self.nodes[1].getnewaddress()
        self.nodes[0].sendtoaddress(non_issuer_address, 1)
        self.nodes[0].generatetoaddress(1, token_address)

        # Fail to set data on token we did not issue
        try:
            self.nodes[1].omni_setnonfungibledata(property_id, 102, 102, True, "Test issuerdata")
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Error with selected inputs for the send transaction" in errorString)
        errorString = ""

        # Fail to set data on token we do not own
        try:
            self.nodes[1].omni_setnonfungibledata(property_id, 102, 102, False, "Test holderdata")
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Error with selected inputs for the send transaction" in errorString)
        errorString = ""

        # Send token to new address
        self.nodes[0].omni_sendnonfungible(token_address, non_issuer_address, property_id, 102, 111)
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_all()

        # Fail to set data on token we did not issue but do own
        try:
            self.nodes[1].omni_setnonfungibledata(property_id, 102, 102, True, "Test issuerdata")
        except JSONRPCException as e:
            errorString = e.error['message']
        assert("Error with selected inputs for the send transaction" in errorString)
        errorString = ""

        # Set holder data
        self.nodes[1].omni_setnonfungibledata(property_id, 102, 102, False, "Test holderdata")
        self.sync_mempools()
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_blocks()

        # Before blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 102)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'Test holderdata')

        # End new data
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 103)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Set holder data range and overwrite just set entry
        self.nodes[1].omni_setnonfungibledata(property_id, 102, 111, False, "New holderdata")
        self.sync_mempools()
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_blocks()

        # Before blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Start new data
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 102)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # End new data
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 111)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # After blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 112)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Set holder data in the middle of the range
        self.nodes[1].omni_setnonfungibledata(property_id, 106, 106, False, "Even newer holderdata")
        self.sync_mempools()
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_blocks()

        # Before blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Before range start
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 102)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # Before range end
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 105)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # Data changed?
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 106)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'Even newer holderdata')

        # After range
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 107)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # End after range data
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 111)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # After blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 112)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Test issuer data
        self.nodes[0].omni_setnonfungibledata(property_id, 106, 106, True, "Test issuerdata")
        self.sync_mempools()
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_blocks()

        # Before
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 105)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # Changed?
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 106)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Test issuerdata')
        assert_equal(result[0]['holderdata'], 'Even newer holderdata')

        # After
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 107)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # Set issuer data across multiple ranges owned by different addresses
        self.nodes[0].omni_setnonfungibledata(property_id, 101, 112, True, "Different issuerdata")
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_blocks()

        # Before blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 100)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], '')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Before range start
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], '')

        # Changed previously set record?
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 106)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], 'Even newer holderdata')

        # End after range data
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 112)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], '')

        # After blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 113)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Send tokens and chekd data is the same
        self.nodes[0].sendtoaddress(non_issuer_address, 1)
        self.nodes[0].generatetoaddress(1, token_address)
        self.sync_blocks()
        self.nodes[1].omni_sendnonfungible(non_issuer_address, destination_address, property_id, 106, 111)
        self.sync_mempools()
        self.nodes[0].generatetoaddress(1, token_address)

        # Before range start
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 105)
        assert_equal(result[0]['owner'], non_issuer_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # Changed previously set record?
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 106)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], 'Even newer holderdata')

        # End after range data
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 111)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], 'New holderdata')

        # After blank
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 112)
        assert_equal(result[0]['owner'], token_address)
        assert_equal(result[0]['grantdata'], 'Different grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], '')

        # Test omni_getnonfungibletokendata ranges
        result = self.nodes[0].omni_getnonfungibletokendata(property_id)
        assert_equal(len(result), 300)

        # Check first in range
        assert_equal(result[0]['index'], 1)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], '')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Check last in range
        assert_equal(result[299]['index'], 300)
        assert_equal(result[299]['owner'], grant_address)
        assert_equal(result[299]['grantdata'], 'Multiple grantdata')
        assert_equal(result[299]['issuerdata'], '')
        assert_equal(result[299]['holderdata'], '')

        # Below range will return first in range, only one result expected.
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 0)
        assert_equal(len(result), 1)

        assert_equal(result[0]['index'], 1)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], '')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Above range will return last in range
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 301)
        assert_equal(len(result), 1)

        assert_equal(result[0]['index'], 300)
        assert_equal(result[0]['owner'], grant_address)
        assert_equal(result[0]['grantdata'], 'Multiple grantdata')
        assert_equal(result[0]['issuerdata'], '')
        assert_equal(result[0]['holderdata'], '')

        # Test sub range
        result = self.nodes[0].omni_getnonfungibletokendata(property_id, 101, 200)
        assert_equal(len(result), 100)

        assert_equal(result[0]['index'], 101)
        assert_equal(result[0]['owner'], destination_address)
        assert_equal(result[0]['grantdata'], 'Test grantdata')
        assert_equal(result[0]['issuerdata'], 'Different issuerdata')
        assert_equal(result[0]['holderdata'], '')

        assert_equal(result[99]['index'], 200)
        assert_equal(result[99]['owner'], token_address)
        assert_equal(result[99]['grantdata'], 'Different grantdata')
        assert_equal(result[99]['issuerdata'], '')
        assert_equal(result[99]['holderdata'], '')

        # Test omni_getnonfungibletokendata with multiple tokens on an address
        txid = self.nodes[0].omni_sendissuancemanaged(token_address, 2, 5, 0, "", "", "TESTTOKEN2", "", "")
        self.nodes[0].generatetoaddress(1, token_address)

        # Checking the transaction was valid...
        result = self.nodes[0].omni_gettransaction(txid)
        assert_equal(result['valid'], True)
        second_property_id = result["propertyid"]

        # Grant tokens to creator
        txid = self.nodes[0].omni_sendgrant(token_address, "", second_property_id, "100", "")
        self.nodes[0].generatetoaddress(1, token_address)

        # Check multiple properties returned
        result = self.nodes[0].omni_getnonfungibletokens(token_address)
        assert_equal(len(result), 2)
        assert_equal(result[0]['propertyid'], property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 11)
        assert_equal(result[0]['tokens'][0]['tokenend'], 100)
        assert_equal(result[0]['tokens'][0]['amount'], 90)
        assert_equal(result[0]['tokens'][1]['tokenstart'], 112)
        assert_equal(result[0]['tokens'][1]['tokenend'], 200)
        assert_equal(result[0]['tokens'][1]['amount'], 89)
        assert_equal(result[1]['propertyid'], second_property_id)
        assert_equal(result[1]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[1]['tokens'][0]['tokenend'], 100)
        assert_equal(result[1]['tokens'][0]['amount'], 100)

        # Filter on first property ID
        result = self.nodes[0].omni_getnonfungibletokens(token_address, property_id)
        assert_equal(len(result), 1)
        assert_equal(result[0]['propertyid'], property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 11)
        assert_equal(result[0]['tokens'][0]['tokenend'], 100)
        assert_equal(result[0]['tokens'][0]['amount'], 90)
        assert_equal(result[0]['tokens'][1]['tokenstart'], 112)
        assert_equal(result[0]['tokens'][1]['tokenend'], 200)
        assert_equal(result[0]['tokens'][1]['amount'], 89)

        # Filter on second property ID
        result = self.nodes[0].omni_getnonfungibletokens(token_address, second_property_id)
        assert_equal(len(result), 1)
        assert_equal(result[0]['propertyid'], second_property_id)
        assert_equal(result[0]['tokens'][0]['tokenstart'], 1)
        assert_equal(result[0]['tokens'][0]['tokenend'], 100)
        assert_equal(result[0]['tokens'][0]['amount'], 100)

if __name__ == '__main__':
    OmniNonFungibleTokensTest().main()
