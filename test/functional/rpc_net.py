#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC calls related to net.

Tests correspond to code in rpc/net.cpp.
"""

from decimal import Decimal
from itertools import product
import platform
import random
import time

import test_framework.messages
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    p2p_port,
)
from test_framework.wallet import MiniWallet


def assert_net_servicesnames(servicesflag, servicenames):
    """Utility that checks if all flags are correctly decoded in
    `getpeerinfo` and `getnetworkinfo`.

    :param servicesflag: The services as an integer.
    :param servicenames: The list of decoded services names, as strings.
    """
    servicesflag_generated = 0
    for servicename in servicenames:
        servicesflag_generated |= getattr(test_framework.messages, 'NODE_' + servicename)
    assert servicesflag_generated == servicesflag


def seed_addrman(node):
    """ Populate the addrman with addresses from different networks.
    Here 2 ipv4, 2 ipv6, 1 cjdns, 2 onion and 1 i2p addresses are added.
    """
    # These addresses currently don't collide with a deterministic addrman.
    # If the addrman positioning/bucketing is changed, these might collide
    # and adding them fails.
    success = { "success": True }
    assert_equal(node.addpeeraddress(address="1.2.3.4", tried=True, port=8333), success)
    assert_equal(node.addpeeraddress(address="2.0.0.0", port=8333), success)
    assert_equal(node.addpeeraddress(address="1233:3432:2434:2343:3234:2345:6546:4534", tried=True, port=8333), success)
    assert_equal(node.addpeeraddress(address="2803:0:1234:abcd::1", port=45324), success)
    assert_equal(node.addpeeraddress(address="fc00:1:2:3:4:5:6:7", port=8333), success)
    assert_equal(node.addpeeraddress(address="pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion", tried=True, port=8333), success)
    assert_equal(node.addpeeraddress(address="nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion", port=45324, tried=True), success)
    assert_equal(node.addpeeraddress(address="c4gfnttsuwqomiygupdqqqyy5y5emnk5c73hrfvatri67prd7vyq.b32.i2p", port=8333), success)


class NetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-minrelaytxfee=0.00001000"], ["-minrelaytxfee=0.00000500"]]
        self.supports_cli = False

    def run_test(self):
        # We need miniwallet to make a transaction
        self.wallet = MiniWallet(self.nodes[0])

        # By default, the test framework sets up an addnode connection from
        # node 1 --> node0. By connecting node0 --> node 1, we're left with
        # the two nodes being connected both ways.
        # Topology will look like: node0 <--> node1
        self.connect_nodes(0, 1)
        self.sync_all()

        self.test_connection_count()
        self.test_getpeerinfo()
        self.test_getnettotals()
        self.test_getnetworkinfo()
        self.test_addnode_getaddednodeinfo()
        self.test_service_flags()
        self.test_getnodeaddresses()
        self.test_addpeeraddress()
        self.test_sendmsgtopeer()
        self.test_getaddrmaninfo()
        self.test_getrawaddrman()
        self.test_getnetmsgstats()

    def test_connection_count(self):
        self.log.info("Test getconnectioncount")
        # After using `connect_nodes` to connect nodes 0 and 1 to each other.
        assert_equal(self.nodes[0].getconnectioncount(), 2)

    def test_getpeerinfo(self):
        self.log.info("Test getpeerinfo")
        # Create a few getpeerinfo last_block/last_transaction values.
        self.wallet.send_self_transfer(from_node=self.nodes[0]) # Make a transaction so we can see it in the getpeerinfo results
        self.generate(self.nodes[1], 1)
        time_now = int(time.time())
        peer_info = [x.getpeerinfo() for x in self.nodes]
        # Verify last_block and last_transaction keys/values.
        for node, peer, field in product(range(self.num_nodes), range(2), ['last_block', 'last_transaction']):
            assert field in peer_info[node][peer].keys()
            if peer_info[node][peer][field] != 0:
                assert_approx(peer_info[node][peer][field], time_now, vspan=60)
        # check both sides of bidirectional connection between nodes
        # the address bound to on one side will be the source address for the other node
        assert_equal(peer_info[0][0]['addrbind'], peer_info[1][0]['addr'])
        assert_equal(peer_info[1][0]['addrbind'], peer_info[0][0]['addr'])
        assert_equal(peer_info[0][0]['minfeefilter'], Decimal("0.00000500"))
        assert_equal(peer_info[1][0]['minfeefilter'], Decimal("0.00001000"))
        # check the `servicesnames` field
        for info in peer_info:
            assert_net_servicesnames(int(info[0]["services"], 0x10), info[0]["servicesnames"])

        assert_equal(peer_info[0][0]['connection_type'], 'inbound')
        assert_equal(peer_info[0][1]['connection_type'], 'manual')

        assert_equal(peer_info[1][0]['connection_type'], 'manual')
        assert_equal(peer_info[1][1]['connection_type'], 'inbound')

        # Check dynamically generated networks list in getpeerinfo help output.
        assert "(ipv4, ipv6, onion, i2p, cjdns, not_publicly_routable)" in self.nodes[0].help("getpeerinfo")

        self.log.info("Check getpeerinfo output before a version message was sent")
        no_version_peer_id = 2
        no_version_peer_conntime = int(time.time())
        self.nodes[0].setmocktime(no_version_peer_conntime)
        with self.nodes[0].wait_for_new_peer():
            no_version_peer = self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        if self.options.v2transport:
            self.wait_until(lambda: self.nodes[0].getpeerinfo()[no_version_peer_id]["transport_protocol_type"] == "v2")
        self.nodes[0].setmocktime(0)
        peer_info = self.nodes[0].getpeerinfo()[no_version_peer_id]
        peer_info.pop("addr")
        peer_info.pop("addrbind")
        # The next two fields will vary for v2 connections because we send a rng-based number of decoy messages
        peer_info.pop("bytesrecv")
        peer_info.pop("bytessent")
        assert_equal(
            peer_info,
            {
                "addr_processed": 0,
                "addr_rate_limited": 0,
                "addr_relay_enabled": False,
                "bip152_hb_from": False,
                "bip152_hb_to": False,
                "bytesrecv_per_msg": {},
                "bytessent_per_msg": {},
                "connection_type": "inbound",
                "conntime": no_version_peer_conntime,
                "id": no_version_peer_id,
                "inbound": True,
                "inflight": [],
                "last_block": 0,
                "last_transaction": 0,
                "lastrecv": 0 if not self.options.v2transport else no_version_peer_conntime,
                "lastsend": 0 if not self.options.v2transport else no_version_peer_conntime,
                "minfeefilter": Decimal("0E-8"),
                "network": "not_publicly_routable",
                "permissions": [],
                "presynced_headers": -1,
                "relaytxes": False,
                "services": "0000000000000000",
                "servicesnames": [],
                "session_id": "" if not self.options.v2transport else no_version_peer.v2_state.peer['session_id'].hex(),
                "startingheight": -1,
                "subver": "",
                "synced_blocks": -1,
                "synced_headers": -1,
                "timeoffset": 0,
                "transport_protocol_type": "v1" if not self.options.v2transport else "v2",
                "version": 0,
            },
        )
        no_version_peer.peer_disconnect()
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 2)

    def test_getnettotals(self):
        self.log.info("Test getnettotals")
        # Test getnettotals and getpeerinfo by doing a ping. The bytes
        # sent/received should increase by at least the size of one ping
        # and one pong. Both have a payload size of 8 bytes, but the total
        # size depends on the used p2p version:
        #   - p2p v1: 24 bytes (header) + 8 bytes (payload) = 32 bytes
        #   - p2p v2: 21 bytes (header/tag with short-id) + 8 bytes (payload) = 29 bytes
        ping_size = 32 if not self.options.v2transport else 29
        net_totals_before = self.nodes[0].getnettotals()
        peer_info_before = self.nodes[0].getpeerinfo()

        self.nodes[0].ping()
        self.wait_until(lambda: (self.nodes[0].getnettotals()['totalbytessent'] >= net_totals_before['totalbytessent'] + ping_size * 2), timeout=1)
        self.wait_until(lambda: (self.nodes[0].getnettotals()['totalbytesrecv'] >= net_totals_before['totalbytesrecv'] + ping_size * 2), timeout=1)

        for peer_before in peer_info_before:
            peer_after = lambda: next(p for p in self.nodes[0].getpeerinfo() if p['id'] == peer_before['id'])
            self.wait_until(lambda: peer_after()['bytesrecv_per_msg'].get('pong', 0) >= peer_before['bytesrecv_per_msg'].get('pong', 0) + ping_size, timeout=1)
            self.wait_until(lambda: peer_after()['bytessent_per_msg'].get('ping', 0) >= peer_before['bytessent_per_msg'].get('ping', 0) + ping_size, timeout=1)

    def test_getnetmsgstats(self):
        self.log.info("Test getnetmsgstats")

        self.restart_node(0)
        node0 = self.nodes[0]
        self.connect_nodes(0, 1) # Generate some traffic.
        # Wait for the initial messages to be sent/received (don't disconnect too early). "sendheaders" is the last one.
        self.wait_until(lambda: "sendheaders" in node0.getnetmsgstats()["recv"]["not_publicly_routable"]["manual"])
        self.wait_until(lambda: "sendheaders" in node0.getnetmsgstats()["sent"]["not_publicly_routable"]["manual"])
        self.disconnect_nodes(0, 1) # Avoid random/unpredictable packets (e.g. ping) messing with the tests below.
        assert_equal(len(node0.getpeerinfo()), 0)

        # In v2 getnettotals counts also bytes that are not accounted at any message (the v2 handshake).
        # Also the v2 handshake's size could vary.
        if not self.options.v2transport:
            self.log.info("Compare byte count getnetmsgstats vs getnettotals")
            nettotals = self.nodes[0].getnettotals()
            stats_net_con_msg = self.nodes[0].getnetmsgstats(aggregate_by=["network", "connection_type", "message_type"])
            assert_equal(nettotals["totalbytessent"], stats_net_con_msg["sent"]["bytes"])
            assert_equal(nettotals["totalbytesrecv"], stats_net_con_msg["recv"]["bytes"])

        self.log.info(f"Test full (un-aggregated) output is as expected")
        stats_full = node0.getnetmsgstats()
        assert_equal(
            stats_full,
            {
                "recv": {
                    "not_publicly_routable": {
                        "manual": {
                            "addrv2": {"bytes": 63 if self.options.v2transport else 66, "count": 1},
                            "feefilter": {"bytes": 29 if self.options.v2transport else 32, "count": 1},
                            "getheaders": {"bytes": 666 if self.options.v2transport else 669, "count": 1},
                            "headers": {"bytes": 103 if self.options.v2transport else 106, "count": 1},
                            "ping": {"bytes": 29 if self.options.v2transport else 32, "count": 1},
                            "pong": {"bytes": 29 if self.options.v2transport else 32, "count": 1},
                            "sendaddrv2": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "sendcmpct": {"bytes": 30 if self.options.v2transport else 33, "count": 1},
                            "sendheaders": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "verack": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "version": {"bytes": 147 if self.options.v2transport else 138, "count": 1},
                            "wtxidrelay": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                        }
                    }
                },
                "sent": {
                    "not_publicly_routable": {
                        "manual": {
                            "feefilter": {"bytes": 29 if self.options.v2transport else 32, "count": 1},
                            "getaddr": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "getheaders": {"bytes": 666 if self.options.v2transport else 669, "count": 1},
                            "headers": {"bytes": 103 if self.options.v2transport else 106, "count": 1},
                            "ping": {"bytes": 29 if self.options.v2transport else 32, "count": 1},
                            "pong": {"bytes": 29 if self.options.v2transport else 32, "count": 1},
                            "sendaddrv2": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "sendcmpct": {"bytes": 30 if self.options.v2transport else 33, "count": 1},
                            "sendheaders": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "verack": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                            "version": {"bytes": 147 if self.options.v2transport else 138, "count": 1},
                            "wtxidrelay": {"bytes": 33 if self.options.v2transport else 24, "count": 1},
                        }
                    }
                }
            }
        )

        self.log.info("Check that aggregation works correctly")

        def sum_all(json):
            if "bytes" in json:
                return dict(bytes=json["bytes"], count=json["count"])

            s = dict(bytes=0, count=0)
            #print(f'S json={json}')
            for k, v in json.items():
                #print(f'S k={k}, v={v}')
                #print(f'S diving into {v}')
                sub = sum_all(v)
                s["bytes"] += sub["bytes"]
                s["count"] += sub["count"]
            return s

        stats_aggregated_by_bitcoind = node0.getnetmsgstats(aggregate_by=["direction", "network", "connection_type", "message_type"])
        stats_aggregated_by_test = sum_all(stats_full)
        assert_equal(stats_aggregated_by_bitcoind, stats_aggregated_by_test)
        if not self.options.v2transport:
            assert_equal(nettotals["totalbytessent"] + nettotals["totalbytesrecv"], stats_aggregated_by_test["bytes"])

        for i in range(1, 16):
            keywords = []
            if i & 1:
                keywords.append("direction")
            if i & 2:
                keywords.append("network")
            if i & 4:
                keywords.append("connection_type")
            if i & 8:
                keywords.append("message_type")
            random.shuffle(keywords)
            self.log.info(f"Test values add up correctly when aggregated by {keywords}")
            assert_equal(stats_aggregated_by_test, sum_all(node0.getnetmsgstats(aggregate_by=keywords)))

        def get_stats(node):
            return node.getnetmsgstats(aggregate_by=["network", "connection_type"])

        self.log.info("Test that message count and total number of bytes increment when a ping message is sent")
        stats_before_connect = get_stats(node0)
        node2 = node0.add_p2p_connection(P2PInterface())
        assert_equal(len(node0.getpeerinfo()), 1)
        # Wait for the initial PING (that is sent immediately after the connection is estabilshed) to go through.
        self.wait_until(lambda: get_stats(node0)["sent"]["ping"]["count"] > stats_before_connect["sent"]["ping"]["count"])
        stats_before_ping = get_stats(node0)
        node0.ping()
        self.wait_until(lambda: get_stats(node0)["sent"]["ping"]["count"] > stats_before_ping["sent"]["ping"]["count"])
        self.wait_until(lambda: get_stats(node0)["sent"]["ping"]["bytes"] > stats_before_ping["sent"]["ping"]["bytes"])

        self.log.info("Test that when a message is broken in two, the stats only update once the full message has been received")
        ping_msg = node2.build_message(test_framework.messages.msg_ping(nonce=12345))
        stats_before_ping = get_stats(node0)
        # Send the message in two pieces.
        cut_pos = 7 # Chosen at an arbitrary position within the header.
        node2.send_raw_message(ping_msg[:cut_pos])
        assert_equal(get_stats(node0)["recv"]["ping"], stats_before_ping["recv"]["ping"])
        # Send the rest of the ping.
        node2.send_raw_message(ping_msg[cut_pos:])
        self.wait_until(lambda: get_stats(node0)["recv"]["ping"]["count"] == stats_before_ping["recv"]["ping"]["count"] + 1)

        node2.peer_disconnect()

    def test_getnetworkinfo(self):
        self.log.info("Test getnetworkinfo")
        info = self.nodes[0].getnetworkinfo()
        assert_equal(info['networkactive'], True)
        assert_equal(info['connections'], 2)
        assert_equal(info['connections_in'], 1)
        assert_equal(info['connections_out'], 1)

        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: false\n']):
            self.nodes[0].setnetworkactive(state=False)
        assert_equal(self.nodes[0].getnetworkinfo()['networkactive'], False)
        # Wait a bit for all sockets to close
        for n in self.nodes:
            self.wait_until(lambda: n.getnetworkinfo()['connections'] == 0, timeout=3)

        with self.nodes[0].assert_debug_log(expected_msgs=['SetNetworkActive: true\n']):
            self.nodes[0].setnetworkactive(state=True)
        # Connect nodes both ways.
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 0)

        info = self.nodes[0].getnetworkinfo()
        assert_equal(info['networkactive'], True)
        assert_equal(info['connections'], 2)
        assert_equal(info['connections_in'], 1)
        assert_equal(info['connections_out'], 1)

        # check the `servicesnames` field
        network_info = [node.getnetworkinfo() for node in self.nodes]
        for info in network_info:
            assert_net_servicesnames(int(info["localservices"], 0x10), info["localservicesnames"])

        # Check dynamically generated networks list in getnetworkinfo help output.
        assert "(ipv4, ipv6, onion, i2p, cjdns)" in self.nodes[0].help("getnetworkinfo")

    def test_addnode_getaddednodeinfo(self):
        self.log.info("Test addnode and getaddednodeinfo")
        assert_equal(self.nodes[0].getaddednodeinfo(), [])
        # add a node (node2) to node0
        ip_port = "127.0.0.1:{}".format(p2p_port(2))
        self.nodes[0].addnode(node=ip_port, command='add')
        # try to add an equivalent ip
        # (note that OpenBSD doesn't support the IPv4 shorthand notation with omitted zero-bytes)
        if platform.system() != "OpenBSD":
            ip_port2 = "127.1:{}".format(p2p_port(2))
            assert_raises_rpc_error(-23, "Node already added", self.nodes[0].addnode, node=ip_port2, command='add')
        # check that the node has indeed been added
        added_nodes = self.nodes[0].getaddednodeinfo()
        assert_equal(len(added_nodes), 1)
        assert_equal(added_nodes[0]['addednode'], ip_port)
        # check that node cannot be added again
        assert_raises_rpc_error(-23, "Node already added", self.nodes[0].addnode, node=ip_port, command='add')
        # check that node can be removed
        self.nodes[0].addnode(node=ip_port, command='remove')
        assert_equal(self.nodes[0].getaddednodeinfo(), [])
        # check that an invalid command returns an error
        assert_raises_rpc_error(-1, 'addnode "node" "command"', self.nodes[0].addnode, node=ip_port, command='abc')
        # check that trying to remove the node again returns an error
        assert_raises_rpc_error(-24, "Node could not be removed", self.nodes[0].addnode, node=ip_port, command='remove')
        # check that a non-existent node returns an error
        assert_raises_rpc_error(-24, "Node has not been added", self.nodes[0].getaddednodeinfo, '1.1.1.1')

    def test_service_flags(self):
        self.log.info("Test service flags")
        self.nodes[0].add_p2p_connection(P2PInterface(), services=(1 << 4) | (1 << 63))
        if self.options.v2transport:
            assert_equal(['UNKNOWN[2^4]', 'P2P_V2', 'UNKNOWN[2^63]'], self.nodes[0].getpeerinfo()[-1]['servicesnames'])
        else:
            assert_equal(['UNKNOWN[2^4]', 'UNKNOWN[2^63]'], self.nodes[0].getpeerinfo()[-1]['servicesnames'])
        self.nodes[0].disconnect_p2ps()

    def test_getnodeaddresses(self):
        self.log.info("Test getnodeaddresses")
        self.nodes[0].add_p2p_connection(P2PInterface())

        # Add an IPv6 address to the address manager.
        ipv6_addr = "1233:3432:2434:2343:3234:2345:6546:4534"
        self.nodes[0].addpeeraddress(address=ipv6_addr, port=8333)

        # Add 10,000 IPv4 addresses to the address manager. Due to the way bucket
        # and bucket positions are calculated, some of these addresses will collide.
        imported_addrs = []
        for i in range(10000):
            first_octet = i >> 8
            second_octet = i % 256
            a = f"{first_octet}.{second_octet}.1.1"
            imported_addrs.append(a)
            self.nodes[0].addpeeraddress(a, 8333)

        # Fetch the addresses via the RPC and test the results.
        assert_equal(len(self.nodes[0].getnodeaddresses()), 1)  # default count is 1
        assert_equal(len(self.nodes[0].getnodeaddresses(count=2)), 2)
        assert_equal(len(self.nodes[0].getnodeaddresses(network="ipv4", count=8)), 8)

        # Maximum possible addresses in AddrMan is 10000. The actual number will
        # usually be less due to bucket and bucket position collisions.
        node_addresses = self.nodes[0].getnodeaddresses(0, "ipv4")
        assert_greater_than(len(node_addresses), 5000)
        assert_greater_than(10000, len(node_addresses))
        for a in node_addresses:
            assert_greater_than(a["time"], 1527811200)  # 1st June 2018
            assert_equal(a["services"], P2P_SERVICES)
            assert a["address"] in imported_addrs
            assert_equal(a["port"], 8333)
            assert_equal(a["network"], "ipv4")

        # Test the IPv6 address.
        res = self.nodes[0].getnodeaddresses(0, "ipv6")
        assert_equal(len(res), 1)
        assert_equal(res[0]["address"], ipv6_addr)
        assert_equal(res[0]["network"], "ipv6")
        assert_equal(res[0]["port"], 8333)
        assert_equal(res[0]["services"], P2P_SERVICES)

        # Test for the absence of onion, I2P and CJDNS addresses.
        for network in ["onion", "i2p", "cjdns"]:
            assert_equal(self.nodes[0].getnodeaddresses(0, network), [])

        # Test invalid arguments.
        assert_raises_rpc_error(-8, "Address count out of range", self.nodes[0].getnodeaddresses, -1)
        assert_raises_rpc_error(-8, "Network not recognized: Foo", self.nodes[0].getnodeaddresses, 1, "Foo")

    def test_addpeeraddress(self):
        self.log.info("Test addpeeraddress")
        # The node has an existing, non-deterministic addrman from a previous test.
        # Clear it to have a deterministic addrman.
        self.restart_node(1, ["-checkaddrman=1", "-test=addrman"], clear_addrman=True)
        node = self.nodes[1]

        self.log.debug("Test that addpeeraddress is a hidden RPC")
        # It is hidden from general help, but its detailed help may be called directly.
        assert "addpeeraddress" not in node.help()
        assert "unknown command: addpeeraddress" not in node.help("addpeeraddress")

        self.log.debug("Test that adding an empty address fails")
        assert_equal(node.addpeeraddress(address="", port=8333), {"success": False})
        assert_equal(node.getnodeaddresses(count=0), [])

        self.log.debug("Test that non-bool tried fails")
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type bool", self.nodes[0].addpeeraddress, address="1.2.3.4", tried="True", port=1234)

        self.log.debug("Test that adding an address with invalid port fails")
        assert_raises_rpc_error(-1, "JSON integer out of range", self.nodes[0].addpeeraddress, address="1.2.3.4", port=-1)
        assert_raises_rpc_error(-1, "JSON integer out of range", self.nodes[0].addpeeraddress, address="1.2.3.4", port=65536)

        self.log.debug("Test that adding a valid address to the new table succeeds")
        assert_equal(node.addpeeraddress(address="1.0.0.0", tried=False, port=8333), {"success": True})
        addrman = node.getrawaddrman()
        assert_equal(len(addrman["tried"]), 0)
        new_table = list(addrman["new"].values())
        assert_equal(len(new_table), 1)
        assert_equal(new_table[0]["address"], "1.0.0.0")
        assert_equal(new_table[0]["port"], 8333)

        self.log.debug("Test that adding an already-present new address to the new and tried tables fails")
        for value in [True, False]:
            assert_equal(node.addpeeraddress(address="1.0.0.0", tried=value, port=8333), {"success": False, "error": "failed-adding-to-new"})
        assert_equal(len(node.getnodeaddresses(count=0)), 1)

        self.log.debug("Test that adding a valid address to the tried table succeeds")
        assert_equal(node.addpeeraddress(address="1.2.3.4", tried=True, port=8333), {"success": True})
        addrman = node.getrawaddrman()
        assert_equal(len(addrman["new"]), 1)
        tried_table = list(addrman["tried"].values())
        assert_equal(len(tried_table), 1)
        assert_equal(tried_table[0]["address"], "1.2.3.4")
        assert_equal(tried_table[0]["port"], 8333)
        node.getnodeaddresses(count=0)  # getnodeaddresses re-runs the addrman checks

        self.log.debug("Test that adding an already-present tried address to the new and tried tables fails")
        for value in [True, False]:
            assert_equal(node.addpeeraddress(address="1.2.3.4", tried=value, port=8333), {"success": False, "error": "failed-adding-to-new"})
        assert_equal(len(node.getnodeaddresses(count=0)), 2)

        self.log.debug("Test that adding an address, which collides with the address in tried table, fails")
        colliding_address = "1.2.5.45"  # grinded address that produces a tried-table collision
        assert_equal(node.addpeeraddress(address=colliding_address, tried=True, port=8333), {"success": False, "error": "failed-adding-to-tried"})
        # When adding an address to the tried table, it's first added to the new table.
        # As we fail to move it to the tried table, it remains in the new table.
        addrman_info = node.getaddrmaninfo()
        assert_equal(addrman_info["all_networks"]["tried"], 1)
        assert_equal(addrman_info["all_networks"]["new"], 2)

        self.log.debug("Test that adding an another address to the new table succeeds")
        assert_equal(node.addpeeraddress(address="2.0.0.0", port=8333), {"success": True})
        addrman_info = node.getaddrmaninfo()
        assert_equal(addrman_info["all_networks"]["tried"], 1)
        assert_equal(addrman_info["all_networks"]["new"], 3)
        node.getnodeaddresses(count=0)  # getnodeaddresses re-runs the addrman checks

    def test_sendmsgtopeer(self):
        node = self.nodes[0]

        self.restart_node(0)
        # we want to use a p2p v1 connection here in order to ensure
        # a peer id of zero (a downgrade from v2 to v1 would lead
        # to an increase of the peer id)
        self.connect_nodes(0, 1, peer_advertises_v2=False)

        self.log.info("Test sendmsgtopeer")
        self.log.debug("Send a valid message")
        with self.nodes[1].assert_debug_log(expected_msgs=["received: addr"]):
            node.sendmsgtopeer(peer_id=0, msg_type="addr", msg="FFFFFF")

        self.log.debug("Test error for sending to non-existing peer")
        assert_raises_rpc_error(-1, "Error: Could not send message to peer", node.sendmsgtopeer, peer_id=100, msg_type="addr", msg="FF")

        self.log.debug("Test that zero-length msg_type is allowed")
        node.sendmsgtopeer(peer_id=0, msg_type="addr", msg="")

        self.log.debug("Test error for msg_type that is too long")
        assert_raises_rpc_error(-8, "Error: msg_type too long, max length is 12", node.sendmsgtopeer, peer_id=0, msg_type="long_msg_type", msg="FF")

        self.log.debug("Test that unknown msg_type is allowed")
        node.sendmsgtopeer(peer_id=0, msg_type="unknown", msg="FF")

        self.log.debug("Test that empty msg is allowed")
        node.sendmsgtopeer(peer_id=0, msg_type="addr", msg="FF")

        self.log.debug("Test that oversized messages are allowed, but get us disconnected")
        zero_byte_string = b'\x00' * 4000001
        node.sendmsgtopeer(peer_id=0, msg_type="addr", msg=zero_byte_string.hex())
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 0, timeout=10)

    def test_getaddrmaninfo(self):
        self.log.info("Test getaddrmaninfo")
        self.restart_node(1, extra_args=["-cjdnsreachable", "-test=addrman"], clear_addrman=True)
        node = self.nodes[1]
        seed_addrman(node)

        expected_network_count = {
            'all_networks': {'new': 4, 'tried': 4, 'total': 8},
            'ipv4': {'new': 1, 'tried': 1, 'total': 2},
            'ipv6': {'new': 1, 'tried': 1, 'total': 2},
            'onion': {'new': 0, 'tried': 2, 'total': 2},
            'i2p': {'new': 1, 'tried': 0, 'total': 1},
            'cjdns': {'new': 1, 'tried': 0, 'total': 1},
        }

        self.log.debug("Test that count of addresses in addrman match expected values")
        res = node.getaddrmaninfo()
        for network, count in expected_network_count.items():
            assert_equal(res[network]['new'], count['new'])
            assert_equal(res[network]['tried'], count['tried'])
            assert_equal(res[network]['total'], count['total'])

    def test_getrawaddrman(self):
        self.log.info("Test getrawaddrman")
        self.restart_node(1, extra_args=["-cjdnsreachable", "-test=addrman"], clear_addrman=True)
        node = self.nodes[1]
        self.addr_time = int(time.time())
        node.setmocktime(self.addr_time)
        seed_addrman(node)

        self.log.debug("Test that getrawaddrman is a hidden RPC")
        # It is hidden from general help, but its detailed help may be called directly.
        assert "getrawaddrman" not in node.help()
        assert "unknown command: getrawaddrman" not in node.help("getrawaddrman")

        def check_addr_information(result, expected):
            """Utility to compare a getrawaddrman result entry with an expected entry"""
            assert_equal(result["address"], expected["address"])
            assert_equal(result["port"], expected["port"])
            assert_equal(result["services"], expected["services"])
            assert_equal(result["network"], expected["network"])
            assert_equal(result["source"], expected["source"])
            assert_equal(result["source_network"], expected["source_network"])
            assert_equal(result["time"], self.addr_time)

        def check_getrawaddrman_entries(expected):
            """Utility to compare a getrawaddrman result with expected addrman contents"""
            getrawaddrman = node.getrawaddrman()
            getaddrmaninfo = node.getaddrmaninfo()
            for (table_name, table_info) in expected.items():
                assert_equal(len(getrawaddrman[table_name]), len(table_info))
                assert_equal(len(getrawaddrman[table_name]), getaddrmaninfo["all_networks"][table_name])

                for bucket_position in getrawaddrman[table_name].keys():
                    entry = getrawaddrman[table_name][bucket_position]
                    expected_entry = list(filter(lambda e: e["address"] == entry["address"], table_info))[0]
                    assert bucket_position == expected_entry["bucket_position"]
                    check_addr_information(entry, expected_entry)

        # we expect 4 new and 4 tried table entries in the addrman which were added using seed_addrman()
        expected = {
            "new": [
                    {
                        "bucket_position": "82/8",
                        "address": "2.0.0.0",
                        "port": 8333,
                        "services": 9,
                        "network": "ipv4",
                        "source": "2.0.0.0",
                        "source_network": "ipv4",
                    },
                    {
                        "bucket_position": "336/24",
                        "address": "fc00:1:2:3:4:5:6:7",
                        "port": 8333,
                        "services": 9,
                        "network": "cjdns",
                        "source": "fc00:1:2:3:4:5:6:7",
                        "source_network": "cjdns",
                    },
                    {
                        "bucket_position": "963/46",
                        "address": "c4gfnttsuwqomiygupdqqqyy5y5emnk5c73hrfvatri67prd7vyq.b32.i2p",
                        "port": 8333,
                        "services": 9,
                        "network": "i2p",
                        "source": "c4gfnttsuwqomiygupdqqqyy5y5emnk5c73hrfvatri67prd7vyq.b32.i2p",
                        "source_network": "i2p",
                    },
                    {
                        "bucket_position": "613/6",
                        "address": "2803:0:1234:abcd::1",
                        "services": 9,
                        "network": "ipv6",
                        "source": "2803:0:1234:abcd::1",
                        "source_network": "ipv6",
                        "port": 45324,
                    }
            ],
            "tried": [
                    {
                        "bucket_position": "6/33",
                        "address": "1.2.3.4",
                        "port": 8333,
                        "services": 9,
                        "network": "ipv4",
                        "source": "1.2.3.4",
                        "source_network": "ipv4",
                    },
                    {
                        "bucket_position": "197/34",
                        "address": "1233:3432:2434:2343:3234:2345:6546:4534",
                        "port": 8333,
                        "services": 9,
                        "network": "ipv6",
                        "source": "1233:3432:2434:2343:3234:2345:6546:4534",
                        "source_network": "ipv6",
                    },
                    {
                        "bucket_position": "72/61",
                        "address": "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion",
                        "port": 8333,
                        "services": 9,
                        "network": "onion",
                        "source": "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion",
                        "source_network": "onion"
                    },
                    {
                        "bucket_position": "139/46",
                        "address": "nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion",
                        "services": 9,
                        "network": "onion",
                        "source": "nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion",
                        "source_network": "onion",
                        "port": 45324,
                    }
            ]
        }

        self.log.debug("Test that getrawaddrman contains information about newly added addresses in each addrman table")
        check_getrawaddrman_entries(expected)


if __name__ == '__main__':
    NetTest().main()
