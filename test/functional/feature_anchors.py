#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block-relay-only anchors functionality"""

import os

from test_framework.p2p import P2PInterface, P2P_SERVICES
from test_framework.socks5 import Socks5Configuration, Socks5Server
from test_framework.messages import CAddress, hash256
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import check_node_connections, assert_equal, p2p_port

INBOUND_CONNECTIONS = 5
BLOCK_RELAY_CONNECTIONS = 2
ONION_ADDR = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:8333"


class AnchorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.disable_autoconnect = False

    def run_test(self):
        node_anchors_path = self.nodes[0].chain_path / "anchors.dat"

        self.log.info("When node starts, check if anchors.dat doesn't exist")
        assert not os.path.exists(node_anchors_path)

        self.log.info(f"Add {BLOCK_RELAY_CONNECTIONS} block-relay-only connections to node")
        for i in range(BLOCK_RELAY_CONNECTIONS):
            self.log.debug(f"block-relay-only: {i}")
            self.nodes[0].add_outbound_p2p_connection(
                P2PInterface(), p2p_idx=i, connection_type="block-relay-only"
            )

        self.log.info(f"Add {INBOUND_CONNECTIONS} inbound connections to node")
        for i in range(INBOUND_CONNECTIONS):
            self.log.debug(f"inbound: {i}")
            self.nodes[0].add_p2p_connection(P2PInterface())

        self.log.info("Check node connections")
        check_node_connections(node=self.nodes[0], num_in=5, num_out=2)

        # 127.0.0.1
        ip = "7f000001"

        # Since the ip is always 127.0.0.1 for this case,
        # we store only the port to identify the peers
        block_relay_nodes_port = []
        inbound_nodes_port = []
        for p in self.nodes[0].getpeerinfo():
            addr_split = p["addr"].split(":")
            if p["connection_type"] == "block-relay-only":
                block_relay_nodes_port.append(hex(int(addr_split[1]))[2:])
            else:
                inbound_nodes_port.append(hex(int(addr_split[1]))[2:])

        self.log.debug("Stop node")
        self.stop_node(0)

        # It should contain only the block-relay-only addresses
        self.log.info("Check the addresses in anchors.dat")

        with open(node_anchors_path, "rb") as file_handler:
            anchors = file_handler.read()

        anchors_hex = anchors.hex()
        for port in block_relay_nodes_port:
            ip_port = ip + port
            assert ip_port in anchors_hex
        for port in inbound_nodes_port:
            ip_port = ip + port
            assert ip_port not in anchors_hex

        self.log.info("Perturb anchors.dat to test it doesn't throw an error during initialization")
        with self.nodes[0].assert_debug_log(["0 block-relay-only anchors will be tried for connections."]):
            with open(node_anchors_path, "wb") as out_file_handler:
                tweaked_contents = bytearray(anchors)
                tweaked_contents[20:20] = b'1'
                out_file_handler.write(bytes(tweaked_contents))

            self.log.debug("Start node")
            self.start_node(0)

        self.log.info("When node starts, check if anchors.dat doesn't exist anymore")
        assert not os.path.exists(node_anchors_path)

        self.log.info("Ensure addrv2 support")
        # Use proxies to catch outbound connections to networks with 256-bit addresses
        onion_conf = Socks5Configuration()
        onion_conf.auth = True
        onion_conf.unauth = True
        onion_conf.addr = ('127.0.0.1', p2p_port(self.num_nodes))
        onion_conf.keep_alive = True
        onion_proxy = Socks5Server(onion_conf)
        onion_proxy.start()
        self.restart_node(0, extra_args=[f"-onion={onion_conf.addr[0]}:{onion_conf.addr[1]}"])

        self.log.info("Add 256-bit-address block-relay-only connections to node")
        self.nodes[0].addconnection(ONION_ADDR, 'block-relay-only', v2transport=False)

        self.log.debug("Stop node")
        with self.nodes[0].assert_debug_log([f"DumpAnchors: Flush 1 outbound block-relay-only peer addresses to anchors.dat"]):
            self.stop_node(0)
        # Manually close keep_alive proxy connection
        onion_proxy.stop()

        self.log.info("Check for addrv2 addresses in anchors.dat")
        caddr = CAddress()
        caddr.net = CAddress.NET_TORV3
        caddr.ip, port_str = ONION_ADDR.split(":")
        caddr.port = int(port_str)
        # TorV3 addrv2 serialization:
        # time(4) | services(1) | networkID(1) | address length(1) | address(32)
        expected_pubkey = caddr.serialize_v2()[7:39].hex()

        # position of services byte of first addr in anchors.dat
        # network magic, vector length, version, nTime
        services_index = 4 + 1 + 4 + 4
        data = bytes()
        with open(node_anchors_path, "rb") as file_handler:
            data = file_handler.read()
            assert_equal(data[services_index], 0x00)  # services == NONE
            anchors2 = data.hex()
            assert expected_pubkey in anchors2

        with open(node_anchors_path, "wb") as file_handler:
            # Modify service flags for this address even though we never connected to it.
            # This is necessary because on restart we will not attempt an anchor connection
            # to a host without our required services, even if its address is in the anchors.dat file
            new_data = bytearray(data)[:-32]
            new_data[services_index] = P2P_SERVICES
            new_data_hash = hash256(new_data)
            file_handler.write(new_data + new_data_hash)

        self.log.info("Restarting node attempts to reconnect to anchors")
        with self.nodes[0].assert_debug_log([f"Trying to make an anchor connection to {ONION_ADDR}"]):
            self.start_node(0, extra_args=[f"-onion={onion_conf.addr[0]}:{onion_conf.addr[1]}"])


if __name__ == "__main__":
    AnchorsTest(__file__).main()
