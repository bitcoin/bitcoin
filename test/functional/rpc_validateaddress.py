#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test validateaddress across networks (signet and mainnet)"""

import json
import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

TEST_DATA_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)), "data", "rpc_validateaddress.json")


class ValidateAddressTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.chain = "signet"
        self.num_nodes = 1
        self.extra_args = [["-prune=899"]]

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

    def test_validateaddress_on_network(self, test_data, network_name):
        self.log.info(f"Testing validateaddress on {network_name}")
        for entry in test_data["invalid"][network_name]:
            self.check_invalid(entry["address"], entry["error"], entry["error_locations"])
        for entry in test_data["valid"][network_name]:
            self.check_valid(entry["address"], entry["scriptPubKey"])

    def run_test(self):
        with open(TEST_DATA_PATH, encoding="utf-8") as f:
            test_data = json.load(f)

        self.test_validateaddress_on_network(test_data, "signet")

        self.log.info("Restarting node on mainnet")
        self.stop_nodes()
        self.nodes.clear()
        self.chain = ""
        self.extra_args = [["-prune=899"]]
        self.setup_chain()
        self.setup_network()
        self.test_validateaddress_on_network(test_data, "mainnet")


if __name__ == "__main__":
    ValidateAddressTest(__file__).main()
