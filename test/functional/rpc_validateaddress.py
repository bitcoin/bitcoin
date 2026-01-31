#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test validateaddress for main chain"""

import json
import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

TESTSDIR = os.path.dirname(os.path.realpath(__file__))


class ValidateAddressMainTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.chain = "signet"
        self.num_nodes = 1
        self.extra_args = [["-prune=899", "-signet"]]

    def check_valid(self, addr, spk):
        info = self.nodes[0].validateaddress(addr)
        assert_equal(info["isvalid"], True)
        assert_equal(info["scriptPubKey"], spk)
        assert "error" not in info
        assert "error_locations" not in info

    def check_invalid(self, addr, error_str, error_locations):
        res = self.nodes[0].validateaddress(addr)
        assert_equal(res["isvalid"], False)
        assert_equal(res["error"], error_str)
        assert_equal(res["error_locations"], error_locations)

    def load_test_data(self, filename):
        with open(filename, 'r', encoding='utf-8') as f:
            return json.load(f)

    def test_validateaddress_signet(self, test_data):
        for entry in test_data["invalid"]["signet"]:
            self.check_invalid(entry["address"], entry["error"], entry["error_locations"])
        for entry in test_data["valid"]["signet"]:
            self.check_valid(entry["address"], entry["scriptPubKey"])

    def test_validateaddress_mainnet(self, test_data):
        for entry in test_data["invalid"]["mainnet"]:
            self.check_invalid(entry["address"], entry["error"], entry["error_locations"])
        for entry in test_data["valid"]["mainnet"]:
            self.check_valid(entry["address"], entry["scriptPubKey"])

    def run_test(self):
        test_data = self.load_test_data(os.path.join(TESTSDIR, "data", "rpc_validateaddress.json"))

        self.test_validateaddress_signet(test_data)
        self.stop_nodes()
        self.nodes.clear()
        self.chain = ""
        self.extra_args = [["-prune=899"]]
        self.setup_chain()
        self.setup_network()
        self.test_validateaddress_mainnet(test_data)

if __name__ == "__main__":
    ValidateAddressMainTest(__file__).main()
