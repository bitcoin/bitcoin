#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the behavior of RPC importprivkey on set and unset labels of
addresses.

It tests different cases in which an address is imported with importaddress
with or without a label and then its private key is imported with importprivkey
with and without a label.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet_util import test_address


class ImportWithLabel(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        """Main test logic"""

        self.log.info(
            "Test importaddress with label and importprivkey without label."
        )
        self.log.info("Import a watch-only address with a label.")
        address = self.nodes[0].getnewaddress()
        label = "Test Label"
        self.nodes[1].importaddress(address, label)
        test_address(self.nodes[1],
                     address,
                     iswatchonly=True,
                     ismine=False,
                     label=label)

        self.log.info(
            "Import the watch-only address's private key without a "
            "label and the address should keep its label."
        )
        priv_key = self.nodes[0].dumpprivkey(address)
        self.nodes[1].importprivkey(priv_key)

        test_address(self.nodes[1],
                     address,
                     label=label)

        self.log.info(
            "Test importaddress without label and importprivkey with label."
        )
        self.log.info("Import a watch-only address without a label.")
        address2 = self.nodes[0].getnewaddress()
        self.nodes[1].importaddress(address2)
        test_address(self.nodes[1],
                     address2,
                     iswatchonly=True,
                     ismine=False,
                     label="")

        self.log.info(
            "Import the watch-only address's private key with a "
            "label and the address should have its label updated."
        )
        priv_key2 = self.nodes[0].dumpprivkey(address2)
        label2 = "Test Label 2"
        self.nodes[1].importprivkey(priv_key2, label2)

        test_address(self.nodes[1],
                     address2,
                     label=label2)

        self.log.info("Test importaddress with label and importprivkey with label.")
        self.log.info("Import a watch-only address with a label.")
        address3 = self.nodes[0].getnewaddress()
        label3_addr = "Test Label 3 for importaddress"
        self.nodes[1].importaddress(address3, label3_addr)
        test_address(self.nodes[1],
                     address3,
                     iswatchonly=True,
                     ismine=False,
                     label=label3_addr)

        self.log.info(
            "Import the watch-only address's private key with a "
            "label and the address should have its label updated."
        )
        priv_key3 = self.nodes[0].dumpprivkey(address3)
        label3_priv = "Test Label 3 for importprivkey"
        self.nodes[1].importprivkey(priv_key3, label3_priv)

        test_address(self.nodes[1],
                     address3,
                     label=label3_priv)

        self.log.info(
            "Test importprivkey won't label new dests with the same "
            "label as others labeled dests for the same key."
        )
        self.log.info("Import a watch-only legacy address with a label.")
        address4 = self.nodes[0].getnewaddress()
        label4_addr = "Test Label 4 for importaddress"
        self.nodes[1].importaddress(address4, label4_addr)
        test_address(self.nodes[1],
                     address4,
                     iswatchonly=True,
                     ismine=False,
                     label=label4_addr,
                     embedded=None)

        self.log.info(
            "Import the watch-only address's private key without a "
            "label and new destinations for the key should have an "
            "empty label while the 'old' destination should keep "
            "its label."
        )
        priv_key4 = self.nodes[0].dumpprivkey(address4)
        self.nodes[1].importprivkey(priv_key4)
        embedded_addr = self.nodes[1].getaddressinfo(address4)['embedded']['address']

        test_address(self.nodes[1],
                     embedded_addr,
                     label="")
        test_address(self.nodes[1],
                     address4,
                     label=label4_addr)

        self.stop_nodes()


if __name__ == "__main__":
    ImportWithLabel().main()
