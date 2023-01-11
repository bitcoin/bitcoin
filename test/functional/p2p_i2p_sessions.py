#!/usr/bin/env python3
# Copyright (c) 2022-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test whether persistent or transient I2P sessions are being used, based on `-i2pacceptincoming`.
"""

from test_framework.test_framework import SyscoinTestFramework


class I2PSessions(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # The test assumes that an I2P SAM proxy is not listening here.
        self.extra_args = [
            ["-i2psam=127.0.0.1:60000", "-i2pacceptincoming=1"],
            ["-i2psam=127.0.0.1:60000", "-i2pacceptincoming=0"],
        ]

    def run_test(self):
        addr = "zsxwyo6qcn3chqzwxnseusqgsnuw3maqnztkiypyfxtya4snkoka.b32.i2p"

        self.log.info("Ensure we create a persistent session when -i2pacceptincoming=1")
        node0 = self.nodes[0]
        with node0.assert_debug_log(expected_msgs=["Creating persistent SAM session"]):
            node0.addnode(node=addr, command="onetry")

        self.log.info("Ensure we create a transient session when -i2pacceptincoming=0")
        node1 = self.nodes[1]
        with node1.assert_debug_log(expected_msgs=["Creating transient SAM session"]):
            node1.addnode(node=addr, command="onetry")


if __name__ == '__main__':
    I2PSessions().main()
