#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getgeneralinfo RPC."""

import os.path
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class GetGeneralInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.min_starttime = int(time.time())

    def run_test(self):
        """Test getgeneralinfo."""
        max_starttime = int(time.time()) + 1

        """Check if 'getgeneralinfo' is idempotent (multiple requests return same data)."""
        json1 = self.nodes[0].getgeneralinfo()
        json2 = self.nodes[0].getgeneralinfo()
        assert_equal(json1, json2)

        """Check 'getgeneralinfo' result is correct."""
        ver_search = f'version {json1["clientversion"]} ('
        with open(self.nodes[0].debug_log_path, encoding='utf-8') as dl:
            for line in dl:
                if line.find(ver_search) >= 0:
                    break
            else:
                raise AssertionError(f"Failed to find clientversion '{json1['clientversion']}' in debug log")

        assert_equal(json1['useragent'], self.nodes[0].getnetworkinfo()['subversion'])

        net_datadir = str(self.nodes[0].chain_path)
        assert_equal(json1['datadir'], net_datadir)
        assert_equal(json1['blocksdir'], os.path.join(net_datadir, "blocks"))
        # startuptime is set before mocktime param is loaded
        assert json1['startuptime'] >= self.min_starttime and json1['startuptime'] <= max_starttime

if __name__ == '__main__':
    GetGeneralInfoTest(__file__).main()
