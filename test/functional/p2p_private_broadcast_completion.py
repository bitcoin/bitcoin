#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that all three initial private sends complete even after receiving the transaction back."""

from test_framework.messages import CAddress, msg_tx
from test_framework.p2p import P2PInterface, P2P_SERVICES, p2p_lock, start_p2p_listener
from test_framework.socks5 import Socks5Server, start_socks5_server
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


class DelayedPrivateBroadcastPeer(P2PInterface):
    def __init__(self, private_peers):
        super().__init__()
        self.private_peers = private_peers

    def on_version(self, message):
        # Private broadcast connections have zero services. Hold them at verack.
        if message.nServices == 0:
            self.private_peers.append(self)
        else:
            super().on_version(message)


class PrivateBroadcastCompletionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.disable_autoconnect = False

    def setup_nodes(self):
        self.private_peers = []  # Guarded by p2p_lock

        def destinations_factory(_requested_to_addr, _requested_to_port, _proxy_client):
            peer = DelayedPrivateBroadcastPeer(self.private_peers)
            peer.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
            peer.peer_connect_send_version(services=P2P_SERVICES)
            addr, port = start_p2p_listener(self.network_thread, peer)
            return {"actual_to_addr": addr, "actual_to_port": port}

        self.proxy = start_socks5_server(destinations_factory)
        self.extra_args = [[
            "-privatebroadcast",
            f"-proxy={self.proxy.conf.addr[0]}:{self.proxy.conf.addr[1]}",
        ]]
        super().setup_nodes()

    def run_test(self):
        node = self.nodes[0]
        returner = node.add_p2p_connection(P2PInterface())
        self.fill_node_addrman(node_index=0, address_types_to_add=[CAddress.NET_TORV3])
        wallet = MiniWallet(node)
        tx = wallet.create_self_transfer()
        node.sendrawtransaction(tx["hex"])

        def three_private_peers():
            with p2p_lock:
                return len(self.private_peers) == 3
        self.wait_until(three_private_peers)
        with p2p_lock:
            peers = list(self.private_peers)
            self.private_peers.clear()

        self.log.info("Ensure each peer receives the transaction before disconnecting")
        for i, peer in enumerate(peers):
            P2PInterface.on_version(peer, peer.last_message["version"])
            peer.wait_for_disconnect()
            assert_equal(peer.last_message["tx"].tx.txid_hex, tx["txid"])
            if i == 0:
                self.log.info("Return the transaction while the other two peers still wait on handshake")
                returner.send_and_ping(msg_tx(tx["tx"]))

        self.log.info("Receive another transaction while the proxy is unavailable")
        self.proxy.stop()
        self.proxy.s.close()
        next_tx = wallet.create_self_transfer()
        with node.assert_debug_log(["will retry to a different address"], timeout=10):
            node.sendrawtransaction(next_tx["hex"])
        returner.send_and_ping(msg_tx(next_tx["tx"]))

        self.log.info("Restoring the proxy opens all three connections for the received transaction")
        self.proxy = Socks5Server(self.proxy.conf)
        self.proxy.start()
        self.wait_until(three_private_peers)
        with p2p_lock:
            peers = list(self.private_peers)
        for peer in peers:
            P2PInterface.on_version(peer, peer.last_message["version"])
            peer.wait_for_disconnect()
            assert_equal(peer.last_message["tx"].tx.txid_hex, next_tx["txid"])

        self.stop_node(0)
        self.proxy.stop()


if __name__ == "__main__":
    PrivateBroadcastCompletionTest(__file__).main()
