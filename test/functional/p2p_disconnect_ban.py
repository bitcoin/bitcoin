#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node disconnect and ban behavior"""
import time
from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class DisconnectBanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def run_test(self):
        self.log.info("Connect nodes both ways")
        # By default, the test framework sets up an addnode connection from
        # node 1 --> node0. By connecting node0 --> node 1, we're left with
        # the two nodes being connected both ways.
        # Topology will look like: node0 <--> node1
        self.connect_nodes(0, 1)

        self.log.info("Test setban and listbanned RPCs")

        self.log.info("setban: successfully ban single IP address")
        assert_equal(len(self.nodes[1].getpeerinfo()), 2)  # node1 should have 2 connections to node0 at this point
        self.nodes[1].setban(subnet="127.0.0.1", command="add")
        self.wait_until(lambda: len(self.nodes[1].getpeerinfo()) == 0, timeout=10)
        assert_equal(len(self.nodes[1].getpeerinfo()), 0)  # all nodes must be disconnected at this point
        assert_equal(len(self.nodes[1].listbanned()), 1)

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
        self.nodes[1].setban("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion", "add")
        self.nodes[1].setban("192.168.0.1", "add", 1)  # ban for 1 seconds
        self.nodes[1].setban("2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/19", "add", 1000)  # ban for 1000 seconds
        listBeforeShutdown = self.nodes[1].listbanned()
        assert_equal("192.168.0.1/32", listBeforeShutdown[2]['address'])

        self.log.info("setban: test banning with absolute timestamp")
        self.nodes[1].setban("192.168.0.2", "add", old_time + 120, absolute=True)

        # Move time forward by 3 seconds so the fourth ban has expired
        self.nodes[1].setmocktime(old_time + 3)
        assert_equal(len(self.nodes[1].listbanned()), 5)

        self.log.info("Test ban_duration and time_remaining")
        for ban in self.nodes[1].listbanned():
            if ban["address"] in ["127.0.0.0/32", "127.0.0.0/24", "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"]:
                assert_equal(ban["ban_duration"], 86400)
                assert_equal(ban["time_remaining"], 86397)
            elif ban["address"] == "2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/19":
                assert_equal(ban["ban_duration"], 1000)
                assert_equal(ban["time_remaining"], 997)
            elif ban["address"] == "192.168.0.2/32":
                assert_equal(ban["ban_duration"], 120)
                assert_equal(ban["time_remaining"], 117)

        # Keep mocktime, to avoid ban expiry when restart takes longer than
        # time_remaining
        self.restart_node(1, extra_args=[f"-mocktime={old_time+4}"])

        listAfterShutdown = self.nodes[1].listbanned()
        assert_equal("127.0.0.0/24", listAfterShutdown[0]['address'])
        assert_equal("127.0.0.0/32", listAfterShutdown[1]['address'])
        assert_equal("192.168.0.2/32", listAfterShutdown[2]['address'])
        assert_equal("/19" in listAfterShutdown[3]['address'], True)
        assert_equal("pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion", listAfterShutdown[4]['address'])

        # Clear ban lists
        self.nodes[1].clearbanned()
        self.log.info("Connect nodes both ways")
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 0)

        self.log.info("Test disconnectnode RPCs")

        self.log.info("disconnectnode: fail to disconnect when calling with address and nodeid")
        address1 = self.nodes[0].getpeerinfo()[0]['addr']
        node1 = self.nodes[0].getpeerinfo()[0]["id"]
        assert_raises_rpc_error(-32602, "Only one of address and nodeid should be provided.", self.nodes[0].disconnectnode, address=address1, nodeid=node1)

        self.log.info("disconnectnode: fail to disconnect when calling with junk address")
        assert_raises_rpc_error(-29, "Node not found in connected nodes", self.nodes[0].disconnectnode, address="221B Baker Street")

        self.log.info("disconnectnode: successfully disconnect node by address")
        address1 = self.nodes[0].getpeerinfo()[0]['addr']
        self.nodes[0].disconnectnode(address=address1)
        self.wait_until(lambda: len(self.nodes[1].getpeerinfo()) == 1, timeout=10)
        assert not [node for node in self.nodes[0].getpeerinfo() if node['addr'] == address1]

        self.log.info("disconnectnode: successfully reconnect node")
        self.connect_nodes(0, 1)  # reconnect the node
        assert_equal(len(self.nodes[0].getpeerinfo()), 2)
        assert [node for node in self.nodes[0].getpeerinfo() if node['addr'] == address1]

        self.log.info("disconnectnode: successfully disconnect node by node id")
        id1 = self.nodes[0].getpeerinfo()[0]['id']
        self.nodes[0].disconnectnode(nodeid=id1)
        self.wait_until(lambda: len(self.nodes[1].getpeerinfo()) == 1, timeout=10)
        assert not [node for node in self.nodes[0].getpeerinfo() if node['id'] == id1]

if __name__ == '__main__':
    DisconnectBanTest(__file__).main()
