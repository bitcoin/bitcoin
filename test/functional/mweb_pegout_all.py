#!/usr/bin/env python3
# Copyright (c) 2021 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify that we can pegout all coins in the MWEB"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.ltc_util import get_hog_addr_txout, setup_mweb_chain

class MWEBPegoutAllTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Setup MWEB chain")
        setup_mweb_chain(self.nodes[0])
        
        total_balance = self.nodes[0].getbalance()
        pegout_txid = self.nodes[0].sendtoaddress(address=self.nodes[0].getnewaddress(), amount=total_balance, subtractfeefromamount=True)
        assert_equal(len(self.nodes[0].getrawmempool()), 1)
        self.nodes[0].generate(1)
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        self.log.info("Check that pegged in amount is 0")
        hog_addr_txout = get_hog_addr_txout(self.nodes[0])
        assert_equal(hog_addr_txout.nValue, 0.0)

        self.log.info("Ensure we can mine the next block")
        self.nodes[0].generate(1)

if __name__ == '__main__':
    MWEBPegoutAllTest().main()
