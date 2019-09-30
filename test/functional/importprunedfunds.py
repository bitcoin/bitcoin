#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importprunedfunds and removeprunedfunds RPCs."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class ImportPrunedFundsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(101)

        self.sync_all()
        
        # address
        address1 = self.nodes[0].getnewaddress()
        # pubkey
        address2 = self.nodes[0].getnewaddress()
        address2_pubkey = self.nodes[0].validateaddress(address2)['pubkey']                 # Using pubkey
        # privkey
        address3 = self.nodes[0].getnewaddress()
        address3_privkey = self.nodes[0].dumpprivkey(address3)                              # Using privkey

        #Check only one address
        address_info = self.nodes[0].validateaddress(address1)
        assert_equal(address_info['ismine'], True)

        self.sync_all()

        #Node 1 sync test
        assert_equal(self.nodes[1].getblockcount(),101)

        #Address Test - before import
        address_info = self.nodes[1].validateaddress(address1)
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)

        address_info = self.nodes[1].validateaddress(address2)
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)

        address_info = self.nodes[1].validateaddress(address3)
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)

        #Send funds to self
        txnid1 = self.nodes[0].sendtoaddress(address1, 0.1)
        self.nodes[0].generate(1)
        rawtxn1 = self.nodes[0].gettransaction(txnid1)['hex']
        proof1 = self.nodes[0].gettxoutproof([txnid1])

        txnid2 = self.nodes[0].sendtoaddress(address2, 0.05)
        self.nodes[0].generate(1)
        rawtxn2 = self.nodes[0].gettransaction(txnid2)['hex']
        proof2 = self.nodes[0].gettxoutproof([txnid2])

        txnid3 = self.nodes[0].sendtoaddress(address3, 0.025)
        self.nodes[0].generate(1)
        rawtxn3 = self.nodes[0].gettransaction(txnid3)['hex']
        proof3 = self.nodes[0].gettxoutproof([txnid3])

        self.sync_all()

        #Import with no affiliated address
        assert_raises_rpc_error(-5, "No addresses", self.nodes[1].importprunedfunds, rawtxn1, proof1)

        balance1 = self.nodes[1].getbalance("", 0, False, True)
        assert_equal(balance1, Decimal(0))

        #Import with affiliated address with no rescan
        self.nodes[1].importaddress(address2, "add2", False)
        result2 = self.nodes[1].importprunedfunds(rawtxn2, proof2)
        balance2 = self.nodes[1].getbalance("add2", 0, False, True)
        assert_equal(balance2, Decimal('0.05'))

        #Import with private key with no rescan
        self.nodes[1].importprivkey(privkey=address3_privkey, label="add3", rescan=False)
        result3 = self.nodes[1].importprunedfunds(rawtxn3, proof3)
        balance3 = self.nodes[1].getbalance("add3", 0, False, False)
        assert_equal(balance3, Decimal('0.025'))
        balance3 = self.nodes[1].getbalance("*", 0, False, True)
        assert_equal(balance3, Decimal('0.075'))

        #Addresses Test - after import
        address_info = self.nodes[1].validateaddress(address1)
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)
        address_info = self.nodes[1].validateaddress(address2)
        assert_equal(address_info['iswatchonly'], True)
        assert_equal(address_info['ismine'], False)
        address_info = self.nodes[1].validateaddress(address3)
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], True)

        #Remove transactions
        assert_raises_rpc_error(-8, "Transaction does not exist in wallet.", self.nodes[1].removeprunedfunds, txnid1)

        balance1 = self.nodes[1].getbalance("*", 0, False, True)
        assert_equal(balance1, Decimal('0.075'))

        self.nodes[1].removeprunedfunds(txnid2)
        balance2 = self.nodes[1].getbalance("*", 0, False, True)
        assert_equal(balance2, Decimal('0.025'))

        self.nodes[1].removeprunedfunds(txnid3)
        balance3 = self.nodes[1].getbalance("*", 0, False, True)
        assert_equal(balance3, Decimal('0.0'))

if __name__ == '__main__':
    ImportPrunedFundsTest().main()
