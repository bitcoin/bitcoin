#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test ports handling for I2P hosts
"""


from test_framework.test_framework import TortoisecoinTestFramework

PROXY = "127.0.0.1:60000"

class I2PPorts(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # The test assumes that an I2P SAM proxy is not listening here.
        self.extra_args = [[f"-i2psam={PROXY}"]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Ensure we don't try to connect if port!=0")
        addr = "zsxwyo6qcn3chqzwxnseusqgsnuw3maqnztkiypyfxtya4snkoka.b32.i2p:8333"
        with node.assert_debug_log(expected_msgs=[f"Error connecting to {addr}, connection refused due to arbitrary port 8333"]):
            node.addnode(node=addr, command="onetry")

        self.log.info("Ensure we try to connect if port=0 and get an error due to missing I2P proxy")
        addr = "h3r6bkn46qxftwja53pxiykntegfyfjqtnzbm6iv6r5mungmqgmq.b32.i2p:0"
        with node.assert_debug_log(expected_msgs=[f"Error connecting to {addr}: Cannot connect to {PROXY}"]):
            node.addnode(node=addr, command="onetry")


if __name__ == '__main__':
    I2PPorts(__file__).main()
