#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the createmultisig API

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class SortMultisigTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]
        self.setup_clean_chain = True

    def run_simple_test(self):
        pub1 = "022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da"
        pub2 = "03e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e9"
        pub3 = "021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc18"

        pubs = [pub1,pub2,pub3]

        default = self.nodes[0].createmultisig(2, pubs)
        unsorted_ms = self.nodes[0].createmultisig(2, pubs, {"sort": False})
        assert_equal(unsorted_ms, self.nodes[0].createmultisig(2, pubs, options={"sort": False}))
        assert_equal(unsorted_ms, self.nodes[0].createmultisig(2, pubs, sort=False))

        assert_equal("2N2BchzwfyuqJep7sKmFfBucfopHZQuPnpt", unsorted_ms["address"])
        assert_equal("5221022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da2103e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e921021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc1853ae", unsorted_ms["redeemScript"])
        assert_equal(default["address"], unsorted_ms["address"])
        assert_equal(default["redeemScript"], unsorted_ms["redeemScript"])

        sorted_ms = self.nodes[0].createmultisig(2, pubs, {"sort": True})
        assert_equal(sorted_ms, self.nodes[0].createmultisig(2, pubs, options={"sort": True}))
        assert_equal(sorted_ms, self.nodes[0].createmultisig(2, pubs, sort=True))
        assert_equal("2NFd5JqpwmQNz3gevZJ3rz9ofuHvqaP9Cye", sorted_ms["address"])
        assert_equal("5221021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc1821022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da2103e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e953ae", sorted_ms["redeemScript"])

    def run_demonstrate_sorting(self):
        pub1 = "022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da"
        pub2 = "03e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e9"
        pub3 = "021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc18"

        sorted_ms = self.nodes[0].createmultisig(2, [pub3,pub1,pub2,])

        self.test_if_result_matches(2, [pub1,pub2,pub3], True, sorted_ms["address"])
        self.test_if_result_matches(2, [pub1,pub3,pub2], True, sorted_ms["address"])
        self.test_if_result_matches(2, [pub2,pub3,pub1], True, sorted_ms["address"])
        self.test_if_result_matches(2, [pub2,pub1,pub3], True, sorted_ms["address"])
        self.test_if_result_matches(2, [pub3,pub1,pub2], True, sorted_ms["address"])
        self.test_if_result_matches(2, [pub3,pub2,pub1], True, sorted_ms["address"])

        self.test_if_result_matches(2, [pub1,pub2,pub3], False, sorted_ms["address"])
        self.test_if_result_matches(2, [pub1,pub3,pub2], False, sorted_ms["address"])
        self.test_if_result_matches(2, [pub2,pub3,pub1], False, sorted_ms["address"])
        self.test_if_result_matches(2, [pub2,pub1,pub3], False, sorted_ms["address"])
        self.test_if_result_matches(2, [pub3,pub2,pub1], False, sorted_ms["address"])

    def test_if_result_matches(self, m, keys, sort, against):
        result = self.nodes[0].createmultisig(m, keys, {"sort": sort})
        assert_equal(sort, result["address"] == against)

    def test_compressed_keys_forbidden(self):
        pub1 = "02fdf7e1b65a477a7815effde996a03a7d94cbc46f7d14c05ef38425156fc92e22"
        pub2 = "04823336da95f0b4cf745839dff26992cef239ad2f08f494e5b57c209e4f3602d5526bc251d480e3284d129f736441560e17f3a7eb7ed665fdf0158f44550b926c"
        rs = "522102fdf7e1b65a477a7815effde996a03a7d94cbc46f7d14c05ef38425156fc92e224104823336da95f0b4cf745839dff26992cef239ad2f08f494e5b57c209e4f3602d5526bc251d480e3284d129f736441560e17f3a7eb7ed665fdf0158f44550b926c52ae"
        pubs = [pub1,pub2]

        default = self.nodes[0].createmultisig(2, pubs)
        assert_equal(rs, default["redeemScript"])

        unsorted_ms = self.nodes[0].createmultisig(2, pubs, {"sort": False})
        assert_equal(rs, unsorted_ms["redeemScript"])
        assert_equal(default["address"], unsorted_ms["address"])
        assert_equal(default["redeemScript"], unsorted_ms["redeemScript"])

        assert_raises_rpc_error(-1, "Compressed key required for BIP67: 04823336da95f0b4cf745839dff26992cef239ad2f08f494e5b57c209e4f3602d5526bc251d480e3284d129f736441560e17f3a7eb7ed665fdf0158f44550b926c", self.nodes[0].createmultisig, 2, pubs, {"sort": True})

    def run_test(self):
        self.run_simple_test()
        self.run_demonstrate_sorting()
        self.test_compressed_keys_forbidden()

if __name__ == '__main__':
    SortMultisigTest(__file__).main()

