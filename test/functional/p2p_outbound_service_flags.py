#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that automatic outbound connections skip addresses lacking desirable service flags.

ThreadOpenConnections filters candidate addresses on
HasAllDesirableServiceFlags before attempting a connection. This covers
that filter: a node whose addrman holds only addresses without those
flags should make no connection attempt, while a node whose addrman
holds usable addresses should.

Two nodes are used rather than two phases on one node, so that
addresses left in the first case cannot influence the second. Both are
given an unreachable proxy, so no attempt can succeed; what is observed
is whether an attempt is made at all.
"""

import time

from test_framework.messages import (
    NODE_NETWORK,
    NODE_NONE,
    NODE_WITNESS,
)
from test_framework.netutil import UNREACHABLE_PROXY_ARG
from test_framework.test_framework import BitcoinTestFramework

# Logged by CConnman::ConnectNode for every attempt, under BCLog::NET.
ATTEMPT_LOG = "trying v1 connection (outbound-full-relay)"

# How long to watch for an attempt that should not happen. An attempt
# against a usable addrman is observed in a few seconds, so this is
# comfortably longer than the behaviour it is ruling out.
QUIET_WINDOW = 20


class P2POutboundServiceFlags(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        # The point of this test is the node's own addrman-driven outbound
        # behaviour, which the framework's default connect=0 disables.
        self.disable_autoconnect = False
        self.extra_args = [["-dnsseed=0", "-fixedseeds=0", "-debug=net",
                            UNREACHABLE_PROXY_ARG]] * 2

    def setup_network(self):
        # Deliberately unconnected: the node's own outbound behaviour is
        # what is under test.
        self.setup_nodes()

    def fill_addrman(self, node, first_octet, services, count=32):
        for i in range(count):
            node.addpeeraddress(address=f"{first_octet}.{i + 1}.0.1", port=8333,
                                tried=True, services=services)

    def run_test(self):
        undesirable, usable = self.nodes

        self.log.info("A node whose addrman holds only addresses without desirable "
                      "service flags makes no outbound attempt")
        self.fill_addrman(undesirable, 10, NODE_NONE)
        with undesirable.assert_debug_log(expected_msgs=[], unexpected_msgs=[ATTEMPT_LOG]):
            time.sleep(QUIET_WINDOW)

        self.log.info("A node whose addrman holds usable addresses does attempt")
        with usable.assert_debug_log(expected_msgs=[ATTEMPT_LOG], timeout=60):
            self.fill_addrman(usable, 11, NODE_NETWORK | NODE_WITNESS)


if __name__ == '__main__':
    P2POutboundServiceFlags(__file__).main()
