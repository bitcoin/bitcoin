#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test P2P behaviour during the handshake phase.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import p2p_port


class P2PHandshakeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Check that connecting to ourself leads to immediate disconnect")
        with node.assert_debug_log(["connected to self", "disconnecting"]):
            node_listen_addr = f"127.0.0.1:{p2p_port(0)}"
            node.addconnection(node_listen_addr, "outbound-full-relay", self.options.v2transport)
            self.wait_until(lambda: len(node.getpeerinfo()) == 0)


if __name__ == '__main__':
    P2PHandshakeTest().main()
