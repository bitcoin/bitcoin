#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getreceivedbyaddress RPC call."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import wait_until, assert_equal, assert_raises_rpc_error

class GetReceivedByAddressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):

        address = "mmo5AqgFVkaT8ehMN5wZ1Xec23h8Pu4T9q"
        address_priv_key = "cMjqJ6RbM3FGoCaVu7x6dmYKu9ARcCGeBE5qtoKvZEaPeiucaNkg"

        # Prepared transaction:
        # bitcoin-tx -create -json -regtest outpubkey=10:02352974bf0da1e6c4652888f014446572669cf7f9fd54d2c3b3d26ef60f15bd4c

        """
        {
            "txid": "24de15cbe041ca8a448bb10635d9dc678bfcfa94b3a2500f69ed98818b9f1de3",
            "hash": "24de15cbe041ca8a448bb10635d9dc678bfcfa94b3a2500f69ed98818b9f1de3",
            "version": 2,
            "size": 54,
            "vsize": 54,
            "weight": 216,
            "locktime": 0,
            "vin": [
            ],
            "vout": [
                {
                    "value": 10.00000000,
                    "n": 0,
                    "scriptPubKey": {
                        "asm": "02352974bf0da1e6c4652888f014446572669cf7f9fd54d2c3b3d26ef60f15bd4c OP_CHECKSIG",
                        "hex": "2102352974bf0da1e6c4652888f014446572669cf7f9fd54d2c3b3d26ef60f15bd4cac",
                        "reqSigs": 1,
                        "type": "pubkey",
                        "addresses": [
                            "mmo5AqgFVkaT8ehMN5wZ1Xec23h8Pu4T9q"
                        ]
                    }
                }
            ],
            "hex": "02000000000100ca9a3b00000000232102352974bf0da1e6c4652888f014446572669cf7f9fd54d2c3b3d26ef60f15bd4cac00000000"
        }
        """
        tx_vout = "02000000000100ca9a3b00000000232102352974bf0da1e6c4652888f014446572669cf7f9fd54d2c3b3d26ef60f15bd4cac00000000"

        self.log.info("Mining blocks...")
        self.nodes[0].generate(101)

        self.log.info("Sending BTCs...")
        raw_tx = self.nodes[0].fundrawtransaction(tx_vout)
        raw_tx_signed = self.nodes[0].signrawtransactionwithwallet(raw_tx['hex'])
        self.nodes[0].sendrawtransaction(raw_tx_signed['hex'])

        self.log.info("Mining transaction...")
        self.nodes[0].generate(7)

        self.log.info("Test getreceivedbyaddress")
        self.nodes[0].importprivkey(address_priv_key)
        received=self.nodes[0].getreceivedbyaddress(address)
        assert_equal(received, 10)

if __name__ == '__main__':
    GetReceivedByAddressTest().main()
