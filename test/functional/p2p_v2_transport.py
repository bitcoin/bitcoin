#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test v2 transport
"""

from test_framework.messages import NODE_P2P_V2
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class V2TransportTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain=True
        self.num_nodes = 1
        self.extra_args = [["-v2transport=0"]]

    def run_test(self):
        network_info = self.nodes[0].getnetworkinfo()
        assert_equal(int(network_info["localservices"], 16) & NODE_P2P_V2, 0)
        if "P2P_V2" in network_info["localservicesnames"]:
            raise AssertionError("Did not expect P2P_V2 to be signaled for a v1 node")

        self.restart_node(0, ["-v2transport=1"])
        network_info = self.nodes[0].getnetworkinfo()
        assert_equal(int(network_info["localservices"], 16) & NODE_P2P_V2, NODE_P2P_V2)
        if "P2P_V2" not in network_info["localservicesnames"]:
            raise AssertionError("Expected P2P_V2 to be signaled for a v2 node")

if __name__ == '__main__':
    V2TransportTest().main()
