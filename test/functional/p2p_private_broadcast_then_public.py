#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test sending a transaction via private broadcast and then, from the same running
node, sending the same transaction again non-privately.

With -privatebroadcast, sendrawtransaction does not add the transaction to the
mempool and only sends it via short-lived private broadcast connections. However,
other code paths add a transaction to the mempool and announce it to all peers
regardless of -privatebroadcast (net_processing's InitiateTxBroadcastToAll()),
for example the submitpackage RPC or the wallet (on commit or on periodic
rebroadcast, via MEMPOOL_AND_BROADCAST_TO_ALL). Here submitpackage is used.

Topology:
* nodes[0]: the transaction originator, runs with -privatebroadcast. All its
  outbound connections go through a SOCKS5 proxy controlled by this test.
* nodes[1]: receives the first private broadcast connection from nodes[0]
  (redirected to it by the proxy). Not connected to anybody else.
* nodes[2]: has a normal (manual) connection from nodes[0]. Not connected to
  anybody else.
"""

import threading

from test_framework.messages import CAddress
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    start_p2p_listener,
)
from test_framework.socks5 import start_socks5_server
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    tor_port,
)
from test_framework.wallet import MiniWallet

NUM_PRIVATE_BROADCAST_PER_TX = 3


class P2PPrivateBroadcastThenPublic(BitcoinTestFramework):
    def set_test_params(self):
        self.disable_autoconnect = False
        self.num_nodes = 3

    def setup_nodes(self):
        self.private_broadcast_to_node1_done = False
        self.lock = threading.Lock()

        def destinations_factory(requested_to_addr, requested_to_port, proxy_client):
            """
            Instruct the SOCKS5 proxy to redirect connections:
            * The first private broadcast connection -> nodes[1]
            * Manual connections -> unchanged (the requested destination)
            * Anything else -> a new P2PInterface
            """
            # SOCKS handlers run in separate threads, so each needs its own RPC connection.
            rpc = self.nodes[0].create_new_rpc_connection()
            conn_type = None

            def connection_type_found():
                nonlocal conn_type
                # The proxy client address equals the node's addrbind for this connection.
                for peer in rpc.getpeerinfo():
                    if peer.get("addrbind") == proxy_client:
                        conn_type = peer["connection_type"]
                        return True
                return False

            self.wait_until(connection_type_found)

            if conn_type == "manual":
                self.log.debug(f"Passing through a manual connection to {requested_to_addr}:{requested_to_port}")
                return {"actual_to_addr": requested_to_addr, "actual_to_port": requested_to_port}

            with self.lock:
                if conn_type == "private-broadcast" and not self.private_broadcast_to_node1_done:
                    self.private_broadcast_to_node1_done = True
                    self.log.debug("Redirecting the first private broadcast connection to nodes[1]")
                    return {"actual_to_addr": "127.0.0.1", "actual_to_port": tor_port(1)}

            listener = P2PInterface()
            listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
            listener.peer_connect_send_version(services=P2P_SERVICES)
            actual_to_addr, actual_to_port = start_p2p_listener(self.network_thread, listener)
            self.log.debug(f"Redirecting a {conn_type} connection to a Python P2PInterface")
            return {"actual_to_addr": actual_to_addr, "actual_to_port": actual_to_port}

        self.socks5_server = start_socks5_server(destinations_factory)

        self.extra_args = [
            [
                "-privatebroadcast",
                "-v2transport=0",
                f"-proxy={self.socks5_server.conf.addr[0]}:{self.socks5_server.conf.addr[1]}",
            ],
            [
                "-connect=0",
                f"-bind=127.0.0.1:{tor_port(1)}=onion",
            ],
            [
                "-connect=0",
                "-listen=1",
            ],
        ]
        super().setup_nodes()

    def setup_network(self):
        # Keep the nodes disconnected from each other.
        self.setup_nodes()

    def run_test(self):
        tx_originator = self.nodes[0]
        private_observer = self.nodes[1]
        public_observer = self.nodes[2]

        self.log.info("Connecting nodes[0] to nodes[2] with a normal connection")
        self.connect_nodes(0, 2)

        self.fill_node_addrman(node_index=0, address_types_to_add=[CAddress.NET_TORV3])

        wallet = MiniWallet(tx_originator)
        tx = wallet.create_self_transfer()
        self.log.info(f"Created txid={tx['txid']} wtxid={tx['wtxid']}")

        self.log.info("Sending the transaction via private broadcast")
        tx_originator.sendrawtransaction(hexstring=tx["hex"], maxfeerate=0.1)

        self.log.info("Observing the private broadcast on nodes[1]")
        self.wait_until(lambda: tx["txid"] in private_observer.getrawmempool())
        assert_equal(private_observer.getmempoolentry(tx["txid"])["wtxid"], tx["wtxid"])

        def all_private_broadcasts_done():
            pending = [t for t in tx_originator.getprivatebroadcastinfo()["transactions"] if t["wtxid"] == tx["wtxid"]]
            assert_equal(len(pending), 1)
            return sum(1 for p in pending[0]["peers"] if "received" in p) >= NUM_PRIVATE_BROADCAST_PER_TX
        self.wait_until(all_private_broadcasts_done)

        self.log.info("Checking that the transaction did not propagate otherwise")
        assert tx["txid"] not in tx_originator.getrawmempool()
        assert tx["txid"] not in public_observer.getrawmempool()

        # Note: we want to make the node to send the TX again non-privately,
        # Currently the `submitpackage` is a good option for that, but if it also gets changes
        # to be privateproadcast-aware, it should be changed to another method -- TODO
        self.log.info("Sending the same transaction again, non-privately, via submitpackage")
        res = tx_originator.submitpackage([tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert tx["txid"] in tx_originator.getrawmempool()

        self.log.info("Observing the non-private broadcast on nodes[2]")
        self.wait_until(lambda: tx["txid"] in public_observer.getrawmempool())
        assert_equal(public_observer.getmempoolentry(tx["txid"])["wtxid"], tx["wtxid"])

        self.stop_node(0)
        self.socks5_server.stop()

        # TODO: Continue the test with another send, in the same way, but with another TX

if __name__ == "__main__":
    P2PPrivateBroadcastThenPublic(__file__).main()
