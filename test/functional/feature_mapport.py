#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that mapport respects networkactive state.

Verifies that NAT-PMP port mapping does not run when network is inactive,
and properly starts/stops when network state is toggled via setnetworkactive RPC.
"""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class MapPortNetworkActiveTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Test that mapport does not run when networkactive=0 at startup")
        self.restart_node(0, extra_args=["-natpmp=1", "-networkactive=0", "-debug=net"])
        assert_equal(node.getnetworkinfo()["networkactive"], False)
        with node.assert_debug_log(
            expected_msgs=[], unexpected_msgs=["portmap:"], timeout=2
        ):
            time.sleep(2)

        self.log.info("Test that mapport starts when network is activated")
        with node.assert_debug_log(expected_msgs=["portmap:"], timeout=5):
            node.setnetworkactive(True)
        assert_equal(node.getnetworkinfo()["networkactive"], True)

        self.log.info("Test that mapport stops when network is deactivated")
        node.setnetworkactive(False)
        assert_equal(node.getnetworkinfo()["networkactive"], False)
        with node.assert_debug_log(
            expected_msgs=[], unexpected_msgs=["portmap:"], timeout=2
        ):
            time.sleep(2)

        self.log.info("Test that mapport resumes when network is reactivated")
        with node.assert_debug_log(expected_msgs=["portmap:"], timeout=5):
            node.setnetworkactive(True)
        assert_equal(node.getnetworkinfo()["networkactive"], True)


if __name__ == "__main__":
    MapPortNetworkActiveTest(__file__).main()
