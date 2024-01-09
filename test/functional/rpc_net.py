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
import time

import test_framework.messages
from test_framework.netutil import ADDRMAN_NEW_BUCKET_COUNT, ADDRMAN_TRIED_BUCKET_COUNT, ADDRMAN_BUCKET_SIZE
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
        self.nodes[0].setmocktime(0)
        peer_info = self.nodes[0].getpeerinfo()[no_version_peer_id]
        peer_info.pop("addr")
        peer_info.pop("addrbind")
        assert_equal(
            peer_info,
            {
                "addr_processed": 0,
                "addr_rate_limited": 0,
                "addr_relay_enabled": False,
                "bip152_hb_from": False,
                "bip152_hb_to": False,
                "bytesrecv": 0,
                "bytesrecv_per_msg": {},
                "bytessent": 0,
                "bytessent_per_msg": {},
                "connection_type": "inbound",
                "conntime": no_version_peer_conntime,
                "id": no_version_peer_id,
                "inbound": True,
                "inflight": [],
                "last_block": 0,
                "last_transaction": 0,
                "lastrecv": 0,
                "lastsend": 0,
                "minfeefilter": Decimal("0E-8"),
                "network": "not_publicly_routable",
                "permissions": [],
                "presynced_headers": -1,
                "relaytxes": False,
                "services": "0000000000000000",
                "servicesnames": [],
                "session_id": "",
                "startingheight": -1,
                "subver": "",
                "synced_blocks": -1,
                "synced_headers": -1,
                "timeoffset": 0,
                "transport_protocol_type": "v1" if not self.options.v2transport else "detecting",
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
        """RPC addpeeraddress sets the source address equal to the destination address.
        If an address with the same /16 as an existing new entry is passed, it will be
        placed in the same new bucket and have a 1/64 chance of the bucket positions
        colliding (depending on the value of nKey in the addrman), in which case the
        new address won't be added.  The probability of collision can be reduced to
        1/2^16 = 1/65536 by using an address from a different /16.  We avoid this here
        by first testing adding a tried table entry before testing adding a new table one.
        """
        self.log.info("Test addpeeraddress")
        self.restart_node(1, ["-checkaddrman=1"])
        node = self.nodes[1]

        self.log.debug("Test that addpeerinfo is a hidden RPC")
        # It is hidden from general help, but its detailed help may be called directly.
        assert "addpeerinfo" not in node.help()
        assert "addpeerinfo" in node.help("addpeerinfo")

        self.log.debug("Test that adding an empty address fails")
        assert_equal(node.addpeeraddress(address="", port=8333), {"success": False})
        assert_equal(node.getnodeaddresses(count=0), [])

        self.log.debug("Test that non-bool tried fails")
        assert_raises_rpc_error(-3, "JSON value of type string is not of expected type bool", self.nodes[0].addpeeraddress, address="1.2.3.4", tried="True", port=1234)

        self.log.debug("Test that adding an address with invalid port fails")
        assert_raises_rpc_error(-1, "JSON integer out of range", self.nodes[0].addpeeraddress, address="1.2.3.4", port=-1)
        assert_raises_rpc_error(-1, "JSON integer out of range", self.nodes[0].addpeeraddress, address="1.2.3.4", port=65536)

        self.log.debug("Test that adding a valid address to the tried table succeeds")
        self.addr_time = int(time.time())
        node.setmocktime(self.addr_time)
        assert_equal(node.addpeeraddress(address="1.2.3.4", tried=True, port=8333), {"success": True})
        with node.assert_debug_log(expected_msgs=["CheckAddrman: new 0, tried 1, total 1 started"]):
            addrs = node.getnodeaddresses(count=0)  # getnodeaddresses re-runs the addrman checks
            assert_equal(len(addrs), 1)
            assert_equal(addrs[0]["address"], "1.2.3.4")
            assert_equal(addrs[0]["port"], 8333)

        self.log.debug("Test that adding an already-present tried address to the new and tried tables fails")
        for value in [True, False]:
            assert_equal(node.addpeeraddress(address="1.2.3.4", tried=value, port=8333), {"success": False})
        assert_equal(len(node.getnodeaddresses(count=0)), 1)

        self.log.debug("Test that adding a second address, this time to the new table, succeeds")
        assert_equal(node.addpeeraddress(address="2.0.0.0", port=8333), {"success": True})
        with node.assert_debug_log(expected_msgs=["CheckAddrman: new 1, tried 1, total 2 started"]):
            addrs = node.getnodeaddresses(count=0)  # getnodeaddresses re-runs the addrman checks
            assert_equal(len(addrs), 2)

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
        node = self.nodes[1]

        # current count of ipv4 addresses in addrman is {'new':1, 'tried':1}
        self.log.info("Test that count of addresses in addrman match expected values")
        res = node.getaddrmaninfo()
        assert_equal(res["ipv4"]["new"], 1)
        assert_equal(res["ipv4"]["tried"], 1)
        assert_equal(res["ipv4"]["total"], 2)
        assert_equal(res["all_networks"]["new"], 1)
        assert_equal(res["all_networks"]["tried"], 1)
        assert_equal(res["all_networks"]["total"], 2)
        for net in ["ipv6", "onion", "i2p", "cjdns"]:
            assert_equal(res[net]["new"], 0)
            assert_equal(res[net]["tried"], 0)
            assert_equal(res[net]["total"], 0)

    def test_getrawaddrman(self):
        self.log.info("Test getrawaddrman")
        node = self.nodes[1]

        self.log.debug("Test that getrawaddrman is a hidden RPC")
        # It is hidden from general help, but its detailed help may be called directly.
        assert "getrawaddrman" not in node.help()
        assert "getrawaddrman" in node.help("getrawaddrman")

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
                assert_equal(len(getrawaddrman[table_name]), len(table_info["entries"]))
                assert_equal(len(getrawaddrman[table_name]), getaddrmaninfo["all_networks"][table_name])

                for bucket_position in getrawaddrman[table_name].keys():
                    bucket = int(bucket_position.split("/")[0])
                    position = int(bucket_position.split("/")[1])

                    # bucket and position only be sanity checked here as the
                    # test-addrman isn't deterministic
                    assert 0 <= int(bucket) < table_info["bucket_count"]
                    assert 0 <= int(position) < ADDRMAN_BUCKET_SIZE

                    entry = getrawaddrman[table_name][bucket_position]
                    expected_entry = list(filter(lambda e: e["address"] == entry["address"], table_info["entries"]))[0]
                    check_addr_information(entry, expected_entry)

        # we expect one addrman new and tried table entry, which were added in a previous test
        expected = {
            "new": {
                "bucket_count": ADDRMAN_NEW_BUCKET_COUNT,
                "entries": [
                    {
                        "address": "2.0.0.0",
                        "port": 8333,
                        "services": 9,
                        "network": "ipv4",
                        "source": "2.0.0.0",
                        "source_network": "ipv4",
                    }
                ]
            },
            "tried": {
                "bucket_count": ADDRMAN_TRIED_BUCKET_COUNT,
                "entries": [
                    {
                        "address": "1.2.3.4",
                        "port": 8333,
                        "services": 9,
                        "network": "ipv4",
                        "source": "1.2.3.4",
                        "source_network": "ipv4",
                    }
                ]
            }
        }

        self.log.debug("Test that the getrawaddrman contains information about the addresses added in a previous test")
        check_getrawaddrman_entries(expected)

        self.log.debug("Add one new address to each addrman table")
        expected["new"]["entries"].append({
            "address": "2803:0:1234:abcd::1",
            "services": 9,
            "network": "ipv6",
            "source": "2803:0:1234:abcd::1",
            "source_network": "ipv6",
            "port": -1,  # set once addpeeraddress is successful
        })
        expected["tried"]["entries"].append({
            "address": "nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion",
            "services": 9,
            "network": "onion",
            "source": "nrfj6inpyf73gpkyool35hcmne5zwfmse3jl3aw23vk7chdemalyaqad.onion",
            "source_network": "onion",
            "port": -1,  # set once addpeeraddress is successful
        })

        port = 0
        for (table_name, table_info) in expected.items():
            # There's a slight chance that the to-be-added address collides with an already
            # present table entry. To avoid this, we increment the port until an address has been
            # added. Incrementing the port changes the position in the new table bucket (bucket
            # stays the same) and changes both the bucket and the position in the tried table.
            while True:
                if node.addpeeraddress(address=table_info["entries"][1]["address"], port=port, tried=table_name == "tried")["success"]:
                    table_info["entries"][1]["port"] = port
                    self.log.debug(f"Added {table_info['entries'][1]['address']} to {table_name} table")
                    break
                else:
                    port += 1

        self.log.debug("Test that the newly added addresses appear in getrawaddrman")
        check_getrawaddrman_entries(expected)


if __name__ == '__main__':
    NetTest().main()
