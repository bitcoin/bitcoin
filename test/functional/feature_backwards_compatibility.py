#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Backwards compatibility functional test

Test various backwards compatibility scenarios. Download the previous node binaries:

contrib/devtools/previous_release.sh -b v0.18.1 v0.17.1

Due to RPC changes introduced in various versions the below tests
won't work for older versions without some patches or workarounds.

Use only the latest patch version of each release, unless a test specifically
needs an older patch version.
"""

import os

from test_framework.test_framework import BitcoinTestFramework, SkipTest

from test_framework.util import (
    assert_equal,
    sync_blocks
)

class BackwardsCompatibilityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        # Add new version after each release:
        self.extra_args = [
            [], # Pre-release: use to mine blocks
            [], # Pre-release: use to receive coins, swap wallets, etc
            [], # v0.18.1
            [] # v0.17.1
        ]

    def setup_nodes(self):
        if os.getenv("TEST_PREVIOUS_RELEASES") == "false":
            raise SkipTest("backwards compatibility tests")

        releases_path = os.getenv("PREVIOUS_RELEASES_DIR") or os.getcwd() + "/releases"
        if not os.path.isdir(releases_path):
            if os.getenv("TEST_PREVIOUS_RELEASES") == "true":
                raise AssertionError("TEST_PREVIOUS_RELEASES=1 but releases missing: " + releases_path)
            raise SkipTest("This test requires binaries for previous releases")

        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            None,
            180100,
            170100
        ], binary=[
            self.options.bitcoind,
            self.options.bitcoind,
            releases_path + "/v0.18.1/bin/bitcoind",
            releases_path + "/v0.17.1/bin/bitcoind"
        ], binary_cli=[
            self.options.bitcoincli,
            self.options.bitcoincli,
            releases_path + "/v0.18.1/bin/bitcoin-cli",
            releases_path + "/v0.17.1/bin/bitcoin-cli"
        ])

        self.start_nodes()

    def run_test(self):
        self.nodes[0].generate(101)

        sync_blocks(self.nodes)

        # Sanity check the test framework:
        res = self.nodes[self.num_nodes - 1].getblockchaininfo()
        assert_equal(res['blocks'], 101)

if __name__ == '__main__':
    BackwardsCompatibilityTest().main()
