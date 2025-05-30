#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the setban rpc call."""

from contextlib import ExitStack
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    p2p_port,
    assert_equal,
)

class SetBanTests(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[],[]]

    def is_banned(self, node, addr):
        return any(e['address'] == addr for e in node.listbanned())

    def run_test(self):
        # Node 0 connects to Node 1, check that the noban permission is not granted
        self.connect_nodes(0, 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert not "noban" in peerinfo["permissions"]

        # Node 0 get banned by Node 1
        self.nodes[1].setban("127.0.0.1", "add")

        # Node 0 should not be able to reconnect
        context = ExitStack()
        context.enter_context(self.nodes[1].assert_debug_log(expected_msgs=['dropped (banned)\n'], timeout=50))
        # When disconnected right after connecting, a v2 node will attempt to reconnect with v1.
        # Wait for that to happen so that it cannot mess with later tests.
        if self.options.v2transport:
            context.enter_context(self.nodes[0].assert_debug_log(expected_msgs=['trying v1 connection'], timeout=50))
        with context:
            self.restart_node(1, [])
            self.nodes[0].addnode("127.0.0.1:" + str(p2p_port(1)), "onetry")

        # However, node 0 should be able to reconnect if it has noban permission
        self.restart_node(1, ['-whitelist=127.0.0.1'])
        self.connect_nodes(0, 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert "noban" in peerinfo["permissions"]

        # If we remove the ban, Node 0 should be able to reconnect even without noban permission
        self.nodes[1].setban("127.0.0.1", "remove")
        self.restart_node(1, [])
        self.connect_nodes(0, 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert not "noban" in peerinfo["permissions"]

        self.log.info("Test that a non-IP address can be banned/unbanned")
        node = self.nodes[1]
        tor_addr = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"
        ip_addr = "1.2.3.4"
        assert not self.is_banned(node, tor_addr)
        assert not self.is_banned(node, ip_addr)

        node.setban(tor_addr, "add")
        assert self.is_banned(node, tor_addr)
        assert not self.is_banned(node, ip_addr)

        node.setban(tor_addr, "remove")
        assert not self.is_banned(self.nodes[1], tor_addr)
        assert not self.is_banned(node, ip_addr)

        self.log.info("Test -bantime")
        self.restart_node(1, ["-bantime=1234"])
        self.nodes[1].setban("127.0.0.1", "add")
        banned = self.nodes[1].listbanned()[0]
        assert_equal(banned['ban_duration'], 1234)

if __name__ == '__main__':
    SetBanTests(__file__).main()
