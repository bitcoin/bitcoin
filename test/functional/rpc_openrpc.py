#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Check that getopenrpcinfo RPC is callable and serializable as valid json."""

import json

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class OpenRPCDocTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info("Calling getopenrpcinfo")
        openrpc = self.nodes[0].getopenrpcinfo()
        json.dumps(openrpc)
        assert_equal(type(openrpc["openrpc"]).__name__, "str")
        assert_equal(type(openrpc["info"]).__name__, "dict")
        assert_equal(type(openrpc["methods"]).__name__, "list")


if __name__ == "__main__":
    OpenRPCDocTest(__file__).main()
