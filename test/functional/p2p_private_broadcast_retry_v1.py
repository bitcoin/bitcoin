#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Ensure that when v2 private broadcast connection to IPv4 fails the v1 retry
will also be made through the Tor proxy.

The test does:
* Add a bunch of IPv4 addresses to the node's addrman (they will be added without P2P_V2 flag).
* Get them to report P2P_V2 in their service flags and connect to each one, so that the flags
  in addrman are updated to contain P2P_V2.
* Get one successful connection to a Tor peer (.onion) so that bitcoind assumes the configured
  Tor proxy works and is indeed a proxy to the Tor network. This will make it open private
  broadcast connections also to IPv4 addresses via that proxy.
* Start some private broadcast connections.
* Remember the destination IPv4 address of the first connection and get it to fail the v2
  transport.
* Wait for a subsequent connection also through the Tor proxy to the same IPv4 and expect
  it to be v1, i.e. the v2->v1 downgrade retry.
"""

from test_framework.netutil import (
    format_addr_port
)
from test_framework.p2p import (
    P2PConnection,
    P2PInterface,
    P2P_SERVICES,
    start_p2p_listener,
)
from test_framework.messages import (
    CAddress,
    NODE_P2P_V2,
)
from test_framework.socks5 import (
    start_socks5_server,
)
from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.v2_p2p import (
    EncryptedP2PState,
)
from test_framework.wallet import (
    MiniWallet,
)

class P2PDetermineV2or1AndClose(P2PConnection):
    def __init__(self, on_v2or1_determined):
        super().__init__()
        self.on_v2or1_determined = on_v2or1_determined

    # https://docs.python.org/3/library/asyncio-protocol.html#asyncio.Protocol.data_received
    def data_received(self, data):
        self.recvbuf += data
        if len(self.recvbuf) >= 4:
            self.on_v2or1_determined(1 if self.recvbuf[0:4] == self.magic_bytes else 2)
            self.peer_disconnect()

    def on_open(self):
        pass

    def on_close(self):
        pass

class P2PPrivateBroadcastRetryV1(BitcoinTestFramework):
    def set_test_params(self):
        self.disable_autoconnect = False
        self.num_nodes = 1

    def ipv4_via_tor_proxy_conn_versions_append(self, v2or1):
        """
        Add to the transport versions (v2 or v1) tried towards the first IPv4 which
        nodes[0] tries to connect to via the Tor proxy.
        """
        self.ipv4_via_tor_proxy_conn_versions.append(v2or1)

    def setup_nodes(self):
        def destinations_factory_all_proxy(requested_to_addr, requested_to_port):
            """
            Instruct the SOCKS5 proxy to redirect all connections to newly created P2PInterface
            objects that claim support for P2P_V2.
            """
            listener = P2PInterface()
            listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
            listener.peer_connect_send_version(services=P2P_SERVICES | NODE_P2P_V2)

            actual_to_addr, actual_to_port = start_p2p_listener(self.network_thread, listener)

            self.log.debug("Instructing the common proxy to redirect connection for "
                           f"{format_addr_port(requested_to_addr, requested_to_port)} to "
                           f"{format_addr_port(actual_to_addr, actual_to_port)} (Python {type(listener).__name__})")

            return {
                "actual_to_addr": actual_to_addr,
                "actual_to_port": actual_to_port,
            }

        self.all_proxy = start_socks5_server(destinations_factory_all_proxy)

        self.ipv4_via_tor_proxy_addr_port = None # Remember the first IPv4 address connected to via the Tor proxy.
        self.ipv4_via_tor_proxy_conn_versions = [] # Transport versions tried on that address.

        def destinations_factory_tor_proxy(requested_to_addr, requested_to_port):
            """
            Instruct the SOCKS5 proxy to redirect all connections to newly created P2PInterface,
            except the first connection to an IPv4 address and all subsequent connections to that
            address which are redirected to P2PDetermineV2or1AndClose.
            """
            requested_to = format_addr_port(requested_to_addr, requested_to_port)

            if not requested_to_addr.endswith(".onion") and self.ipv4_via_tor_proxy_addr_port is None: # First IPv4
                self.ipv4_via_tor_proxy_addr_port = requested_to

            if self.ipv4_via_tor_proxy_addr_port == requested_to:
                # This is either the first (v2) or the second (the expected v1 retry) connection to requested_to.
                listener = P2PDetermineV2or1AndClose(self.ipv4_via_tor_proxy_conn_versions_append)
                listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
            else:
                listener = P2PInterface()
                listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
                listener.peer_connect_send_version(services=P2P_SERVICES | NODE_P2P_V2)
                if not requested_to_addr.endswith(".onion"):
                    listener.v2_state = EncryptedP2PState(initiating=False, net=self.chain)

            actual_to_addr, actual_to_port = start_p2p_listener(self.network_thread, listener)

            self.log.debug(f"Instructing the Tor proxy to redirect connection for {requested_to} to "
                           f"{format_addr_port(actual_to_addr, actual_to_port)} (Python {type(listener).__name__})")

            return {
                "actual_to_addr": actual_to_addr,
                "actual_to_port": actual_to_port,
            }

        self.tor_proxy = start_socks5_server(destinations_factory_tor_proxy)

        self.extra_args = [
            [
                "-privatebroadcast=1",
                f"-proxy={self.all_proxy.conf.addr[0]}:{self.all_proxy.conf.addr[1]}",
                f"-onion={self.tor_proxy.conf.addr[0]}:{self.tor_proxy.conf.addr[1]}",
                "-test=addrman",
                "-v2transport=0",
            ],
        ]

        super().setup_nodes()

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        node0 = self.nodes[0]

        self.log.info("Filling node0's addrman with addresses")
        self.fill_node_addrman(node_index=0, address_types_to_add=[CAddress.NET_IPV4])

        self.log.info("Opening manual connections to all IPv4 addresses to add P2P_V2 flag to addrman entries")
        for a in node0.getnodeaddresses(count=0, network="ipv4"):
            node0.addnode(node=format_addr_port(a["address"], a["port"]), command="onetry", v2transport=False)

        self.log.info("Waiting for all IPv4 addresses to get P2P_V2 as a result of peers advertising support")
        self.wait_until(lambda: all(a["services"] & NODE_P2P_V2 != 0 for a in node0.getnodeaddresses(count=0, network="ipv4")))

        # The destinations behind the -proxy= don't actually support v2. When bitcoind runs with -v2transport=1
        # and tries v2 on them they would print benign "magic byte mismatch" warnings.
        # Disable those since none of them are needed anymore.
        self.all_proxy.conf.destinations_factory = None

        self.restart_node(0, extra_args=self.extra_args[0] + ["-v2transport=1"])

        self.log.info("Opening a connection to a Tor addresses, so bitcoind considers -onion= is a real Tor proxy")
        node0.addnode(node="testonlyad777777777777777777777777777777777777777775b6qd.onion:1234", command="onetry", v2transport=False)

        self.log.info("Waiting for at least one Tor connection")
        self.wait_until(lambda: any(p["network"] == "onion" for p in node0.getpeerinfo()))

        self.log.info("Starting private broadcast connections")
        wallet = MiniWallet(node0)
        tx = wallet.create_self_transfer()
        node0.sendrawtransaction(hexstring=tx["hex"])

        self.log.info("Tor proxy: waiting for connection to an IPv4 address")
        self.wait_until(lambda: self.ipv4_via_tor_proxy_addr_port is not None)
        self.log.info(f"Tor proxy: got {self.ipv4_via_tor_proxy_addr_port}, waiting for v2")
        self.wait_until(lambda: 2 in self.ipv4_via_tor_proxy_conn_versions)
        self.log.info(f"Tor proxy: got {self.ipv4_via_tor_proxy_addr_port} v2, waiting for v1")
        self.wait_until(lambda: 1 in self.ipv4_via_tor_proxy_conn_versions)
        self.log.info(f"Tor proxy: got {self.ipv4_via_tor_proxy_addr_port} v2, v1")

        self.stop_node(0)
        self.all_proxy.stop()
        self.tor_proxy.stop()


if __name__ == "__main__":
    P2PPrivateBroadcastRetryV1(__file__).main()
