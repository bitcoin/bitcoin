#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getversion RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class GetVersionTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        version = self.nodes[0].getversion()
        # The functional tests don't have access to the version number and it
        # can't be hardcoded. Only doing sanity and consistency checks in this
        # test.

        # numerical: e.g. 279900
        assert_equal(type(version["numeric"]), int)
        major = int(version["numeric"] / 10_000)
        minor = int(version["numeric"] / 100) % 100
        build = version["numeric"] % 100

        # short: e.g. "27.99.0"
        assert_equal(version["short"], f"{major}.{minor}.{build}")

        # long: e.g. "v27.99.0-b10ca895160a-dirty"
        assert version["long"].startswith(f"v{major}.{minor}.{build}")

        # client: e.g. "Satoshi"
        assert_equal(type(version["client"]), str)

        # release_candidate: e.g. 0
        assert_equal(type(version["release_candidate"]), int)

        # is_release: e.g. false
        assert_equal(type(version["is_release"]), bool)


if __name__ == '__main__':
    GetVersionTest().main()
