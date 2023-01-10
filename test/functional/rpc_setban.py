#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the setban rpc call."""
import time
from pathlib import Path

from contextlib import ExitStack
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    p2p_port
)

class SetBanTests(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[],[]]

    def run_test(self):
        self.log.info("Connect nodes both way")
        # By default, the test framework sets up an addnode connection from
        # node 1 --> node0. By connecting node0 --> node 1, we're left with
        # the two nodes being connected both ways.
        # Topology will look like: node0 <--> node1
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

        self.log.info("clearbanned: successfully clear ban list")
        self.nodes[1].clearbanned()
        assert_equal(len(self.nodes[1].listbanned()), 0)

        self.log.info('Test banlist database recreation')
        self.stop_node(1)
        target_file = self.nodes[1].chain_path / "banlist.json"
        Path.unlink(target_file)
        with self.nodes[1].assert_debug_log(["Recreating the banlist database"]):
            self.start_node(1)

        assert Path.exists(target_file)
        assert_equal(self.nodes[1].listbanned(), [])

        self.nodes[1].setban("127.0.0.0/24", "add")

        self.log.info("setban: fail to ban an already banned subnet")
        assert_equal(len(self.nodes[1].listbanned()), 1)
        assert_raises_rpc_error(-23, "IP/Subnet already banned", self.nodes[1].setban, "127.0.0.1", "add")

        self.log.info("setban: fail to ban an invalid subnet")
        assert_raises_rpc_error(-30, "Error: Invalid IP/Subnet", self.nodes[1].setban, "127.0.0.1/42", "add")
        assert_equal(len(self.nodes[1].listbanned()), 1)  # still only one banned ip because 127.0.0.1 is within the range of 127.0.0.0/24

        self.log.info("setban: fail to ban with past absolute timestamp")
        assert_raises_rpc_error(-8, "Error: Absolute timestamp is in the past", self.nodes[1].setban, "127.27.0.1", "add", 123, True)

        self.log.info("setban remove: fail to unban a non-banned subnet")
        assert_raises_rpc_error(-30, "Error: Unban failed", self.nodes[1].setban, "127.0.0.1", "remove")
        assert_equal(len(self.nodes[1].listbanned()), 1)

        self.log.info("setban remove: successfully unban subnet")
        self.nodes[1].setban("127.0.0.0/24", "remove")
        assert_equal(len(self.nodes[1].listbanned()), 0)
        self.nodes[1].clearbanned()
        assert_equal(len(self.nodes[1].listbanned()), 0)

        self.log.info("setban: test persistence across node restart")
        # Set the mocktime so we can control when bans expire
        old_time = int(time.time())
        self.nodes[1].setmocktime(old_time)
        self.nodes[1].setban("127.0.0.0/32", "add")
        self.nodes[1].setban("127.0.0.0/24", "add")
        self.nodes[1].setban("192.168.0.1", "add", 1)  # ban for 1 seconds
        self.nodes[1].setban("2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/19", "add", 1000)  # ban for 1000 seconds
        assert any(e['address'] == "192.168.0.1/32" for e in self.nodes[1].listbanned())

        self.log.info("setban: test banning with absolute timestamp")
        self.nodes[1].setban("192.168.0.2", "add", old_time + 120, True)

        # Move time forward by 3 seconds so the third ban has expired
        self.nodes[1].setmocktime(old_time + 3)
        assert_equal(len(self.nodes[1].listbanned()), 4)

        self.log.info("Test ban_duration and time_remaining")
        for ban in self.nodes[1].listbanned():
            if ban["address"] in ["127.0.0.0/32", "127.0.0.0/24"]:
                assert_equal(ban["ban_duration"], 86400)
                assert_equal(ban["time_remaining"], 86397)
            elif ban["address"] == "2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/19":
                assert_equal(ban["ban_duration"], 1000)
                assert_equal(ban["time_remaining"], 997)
            elif ban["address"] == "192.168.0.2/32":
                assert_equal(ban["ban_duration"], 120)
                assert_equal(ban["time_remaining"], 117)

        self.restart_node(1)

        listAfterShutdown = self.nodes[1].listbanned()
        assert_equal("127.0.0.0/24", listAfterShutdown[0]['address'])
        assert_equal("127.0.0.0/32", listAfterShutdown[1]['address'])
        assert_equal("192.168.0.2/32", listAfterShutdown[2]['address'])
        assert_equal("/19" in listAfterShutdown[3]['address'], True)

        self.log.info("Test that a non-IP address can be banned/unbanned")
        tor_addr = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"
        ip_addr = "1.2.3.4"
        list_banned = [addr["address"] for addr in self.nodes[1].listbanned()]
        assert not tor_addr in list_banned
        assert not ip_addr in list_banned

        self.nodes[1].setban(tor_addr, "add")
        list_banned = [addr["address"] for addr in self.nodes[1].listbanned()]
        assert tor_addr in list_banned
        assert not ip_addr in list_banned

        self.log.info("Test -bantime")
        self.restart_node(1, ["-bantime=1234"])
        self.nodes[1].setban("1.2.3.5", "add")
        banned = self.nodes[1].listbanned()[0]
        assert_equal(banned['ban_duration'], 1234)

if __name__ == '__main__':
    SetBanTests().main()
