#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Copyright (c) 2017-2018 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test listmyassets RPC command."""

from test_framework.test_framework import RavenTestFramework
from test_framework.util import *
from test_framework.mininode import *
from io import BytesIO

class ListMyAssetsTest(RavenTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def listmyassets_filter_zeros(self):
        """Sometimes the asset cache will contain zero-quantity holdings for some assets (until they're flushed).
           These shouldn't be returned by listmyassets.
        """

        # activate assets
        self.nodes[0].generate(500)
        self.sync_all()

        assert_equal(0, len(self.nodes[0].listmyassets()))
        assert_equal(0, len(self.nodes[1].listmyassets()))

        self.nodes[0].issue("FOO", 1000)
        self.nodes[0].generate(10)
        self.sync_all()

        result = self.nodes[0].listmyassets()
        assert_equal(2, len(result))
        assert_contains_pair("FOO", 1000, result)
        assert_contains_pair("FOO!", 1, result)

        address_to = self.nodes[1].getnewaddress()
        self.nodes[0].transfer("FOO", 1000, address_to)
        self.nodes[0].generate(10)
        self.sync_all()

        result = self.nodes[0].listmyassets()
        assert_equal(1, len(result))
        assert_contains_pair("FOO!", 1, result)

        result = self.nodes[1].listmyassets()
        assert_equal(1, len(result))
        assert_contains_pair("FOO", 1000, result)


    def run_test(self):
        self.listmyassets_filter_zeros()

if __name__ == '__main__':
    ListMyAssetsTest().main()
