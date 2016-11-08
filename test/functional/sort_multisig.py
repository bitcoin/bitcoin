#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the createmultisig API

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class SortMultisigTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]
        self.setup_clean_chain = True

    def run_test(self):
        pub1 = "022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da"
        pub2 = "03e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e9"
        pub3 = "021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc18"

        pubs = [pub1,pub2,pub3]

        default = self.nodes[0].createmultisig(2, pubs)
        unsorted = self.nodes[0].createmultisig(2, pubs, False)

        assert_equal("2N2BchzwfyuqJep7sKmFfBucfopHZQuPnpt", unsorted["address"])
        assert_equal("5221022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da2103e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e921021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc1853ae", unsorted["redeemScript"])
        assert_equal(default["address"], unsorted["address"])
        assert_equal(default["redeemScript"], unsorted["redeemScript"])

        sorted = self.nodes[0].createmultisig(2, pubs, True)
        assert_equal("2NFd5JqpwmQNz3gevZJ3rz9ofuHvqaP9Cye", sorted["address"])
        assert_equal("5221021f2f6e1e50cb6a953935c3601284925decd3fd21bc445712576873fb8c6ebc1821022df8750480ad5b26950b25c7ba79d3e37d75f640f8e5d9bcd5b150a0f85014da2103e3818b65bcc73a7d64064106a859cc1a5a728c4345ff0b641209fba0d90de6e953ae", sorted["redeemScript"])

if __name__ == '__main__':
    SortMultisigTest().main()

