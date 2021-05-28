#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test ThreadDNSAddressSeed logic for querying DNS seeds."""

from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework


class P2PDNSSeeds(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-dnsseed=1"]]

    def run_test(self):
        self.existing_outbound_connections_test()

    def existing_outbound_connections_test(self):
        # Make sure addrman is populated to enter the conditional where we
        # delay and potentially skip DNS seeding.
        self.nodes[0].addpeeraddress("192.0.0.8", 8333)

        self.log.info("Check that we *do not* query DNS seeds if we have 2 outbound connections")

        self.restart_node(0)
        with self.nodes[0].assert_debug_log(expected_msgs=["P2P peers available. Skipped DNS seeding."], timeout=12):
            for i in range(2):
                self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i, connection_type="outbound-full-relay")


if __name__ == '__main__':
    P2PDNSSeeds().main()
