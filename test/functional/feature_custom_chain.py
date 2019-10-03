#!/usr/bin/env python3
# Copyright (c) 2019-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test running bitcoind with multiple custom chains with different genesis blocks
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

EXPECTED_GENESIS_HASH = {
    'regtest': '0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206',
    'chain_1': '58ebd25d25b128530d4d462c65a7e679b7e053e6f25ffb8ac63bc68832fda201',
    'chain_2': 'e07d79a4f8f1525814e421eb71aa9527fe8a25091fe1b9c5c312939c887aadc7',
    'chain_3': 'de650213b96a541df3bd9ee530cc3da4e9424d3617f95e6b2a0d5452e23412b9',
    'chain_4': '075e818d62bbe049a715856344987294ea2b4ff636b857911966e7fc9fee637c',
    'chain_5': '54a435af6a093f145769b138c20d5f72b35395e7057a89d36a0b8e954031b04c',
}

class CustomChainTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        assert_equal(EXPECTED_GENESIS_HASH[self.chain], self.nodes[0].getblockhash(0))
        self.nodes[0].generatetoaddress(1, self.nodes[0].get_deterministic_priv_key().address)
        self.log.info("Success")

if __name__ == '__main__':
    for chain_name in EXPECTED_GENESIS_HASH.keys():
        CustomChainTest(chain=chain_name).main()
