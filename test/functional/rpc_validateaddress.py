#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the deriveaddresses rpc call."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class ValidateaddressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        # Valid address
        address = "bcrt1q049ldschfnwystcqnsvyfpj23mpsg3jcedq9xv"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], True)
        assert 'error' not in res
        assert 'error_locations' not in res

        # Valid capitalised address
        address = "BCRT1QPLMTZKC2XHARPPZDLNPAQL78RSHJ68U33RAH7R"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], True)
        assert 'error' not in res
        assert 'error_locations' not in res

        # Address with no '1' separator
        address = "bcrtq049ldschfnwystcqnsvyfpj23mpsg3jcedq9xv"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error'], "Missing separator")

        # Address with no HRP
        address = "1q049ldschfnwystcqnsvyfpj23mpsg3jcedq9xv"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error'], "Invalid HRP or Base58 character in address")

        # Address with an invalid bech32 encoding character
        address = "bcrt1q04oldschfnwystcqnsvyfpj23mpsg3jcedq9xv"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error'], "Invalid Base 32 character")
        assert_equal(res['error_locations'], [8])

        # Address with one error
        address = "bcrt1q049edschfnwystcqnsvyfpj23mpsg3jcedq9xv"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error_locations'], [9])
        assert_equal(res['error'], "Invalid checksum")

        # Capitalised address with one error
        address = "BCRT1QPLMTZKC2XHARPPZDLNPAQL78RSHJ68U32RAH7R"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error_locations'], [38])
        assert_equal(res['error'], "Invalid checksum")

        # Valid multisig address
        address = "bcrt1qdg3myrgvzw7ml9q0ejxhlkyxm7vl9r56yzkfgvzclrf4hkpx9yfqhpsuks"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], True)

        # Multisig address with 2 errors
        address = "bcrt1qdg3myrgvzw7ml8q0ejxhlkyxn7vl9r56yzkfgvzclrf4hkpx9yfqhpsuks"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error_locations'], [19, 30])
        assert_equal(res['error'], "Invalid checksum")

        # Address with 2 errors (should be 'bcrt1qax9suht3qv95sw33wavx8crpxduefdrsvgsklx')
        address = "bcrt1qax9suht3qv95sw33xavx8crpxduefdrsvgsklu"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error_locations'], [22, 43])
        assert_equal(res['error'], "Invalid checksum")

        # Invalid but also too long
        address = "bcrt1q049edschfnwystcqnsvyfpj23mpsg3jcedq9xv049edschfnwystcqnsvyfpj23mpsg3jcedq9xv049edschfnwystcqnsvyfpj23m"
        res = self.nodes[0].validateaddress(address)
        assert_equal(res['isvalid'], False)
        assert_equal(res['error_locations'], list(range(90, 108)))
        assert_equal(res['error'], "Bech32 string too long")

if __name__ == '__main__':
    ValidateaddressTest().main()
