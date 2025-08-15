#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getstakinginfo RPC.

Ensure getstakinginfo and stakerstatus return identical output.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class GetStakingInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info("Compare stakerstatus and getstakinginfo outputs")
        info_status = self.nodes[0].stakerstatus()
        info_info = self.nodes[0].getstakinginfo()
        assert_equal(info_status, info_info)


if __name__ == '__main__':
    GetStakingInfoTest(__file__).main()
