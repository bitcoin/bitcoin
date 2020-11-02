#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test client expiry."""

from test_framework.test_framework import BitcoinTestFramework

class OmniClientExpiry(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-omnialertallowsender=any']]

    def run_test(self):
        self.log.info("test client expiry")

        # Preparing some mature Bitcoins
        coinbase_address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(101, coinbase_address)

        # Obtaining a master address to work with
        address = self.nodes[0].getnewaddress()

        # Funding the address with some testnet BTC for fees
        self.nodes[0].sendtoaddress(address, 20)
        self.nodes[0].generatetoaddress(1, coinbase_address)

        # Sending an alert with client expiry version of 999999999
        self.nodes[0].omni_sendalert(address, 3, 999999999, "Client version out of date test.")

        # Generating a block should make the client shutdown
        self.nodes[0].generatetoaddress(10, coinbase_address)

if __name__ == '__main__':
    # We expect the client to shutdown during this test, this will raise an AssertionError, if no
    # AssertionError is raised then this is in error and we raise one ourselves to fail this test.
    client_shutdown = False
    try:
        OmniClientExpiry().main()
    except:
        client_shutdown = True
    finally:
        if not client_shutdown:
            raise AssertionError("Client failed to shutdown as expected.")
