#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Test seednode interaction with the AddrMan
"""
import random
import time

from test_framework.test_framework import BitcoinTestFramework

ADD_NEXT_SEEDNODE = 10


class P2PSeedNodes(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.disable_autoconnect = False

    def test_no_seednode(self):
        # Check that if no seednode is provided, the node proceeds as usual (without waiting)
        with self.nodes[0].assert_debug_log(expected_msgs=[], unexpected_msgs=["Empty addrman, adding seednode", f"Couldn't connect to peers from addrman after {ADD_NEXT_SEEDNODE} seconds. Adding seednode"], timeout=ADD_NEXT_SEEDNODE):
            self.restart_node(0)

    def test_seednode_empty_addrman(self):
        seed_node = "0.0.0.1"
        # Check that the seednode is added to m_addr_fetches on bootstrap on an empty addrman
        with self.nodes[0].assert_debug_log(expected_msgs=[f"Empty addrman, adding seednode ({seed_node}) to addrfetch"], timeout=ADD_NEXT_SEEDNODE):
            self.restart_node(0, extra_args=[f'-seednode={seed_node}'])

    def test_seednode_addrman_unreachable_peers(self):
        seed_node = "0.0.0.2"
        node = self.nodes[0]
        # Fill the addrman with unreachable nodes
        for i in range(10):
            ip = f"{random.randrange(128,169)}.{random.randrange(1,255)}.{random.randrange(1,255)}.{random.randrange(1,255)}"
            port = 8333 + i
            node.addpeeraddress(ip, port)

        # Restart the node so seednode is processed again
        with node.assert_debug_log(expected_msgs=[f"Couldn't connect to peers from addrman after {ADD_NEXT_SEEDNODE} seconds. Adding seednode ({seed_node}) to addrfetch"], unexpected_msgs=["Empty addrman, adding seednode"], timeout=ADD_NEXT_SEEDNODE * 1.5):
            self.restart_node(0, extra_args=[f'-seednode={seed_node}'])
            node.setmocktime(int(time.time()) + ADD_NEXT_SEEDNODE + 1)

    def run_test(self):
        self.test_no_seednode()
        self.test_seednode_empty_addrman()
        self.test_seednode_addrman_unreachable_peers()


if __name__ == '__main__':
    P2PSeedNodes().main()

