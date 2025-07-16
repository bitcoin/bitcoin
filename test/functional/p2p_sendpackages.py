#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test package relay messages"""

import time

from test_framework.messages import (
    msg_sendpackages,
    msg_verack,
    msg_version,
    msg_wtxidrelay,
    PKG_RELAY_PKGTXNS,
)
from test_framework.p2p import (
    p2p_lock,
    P2PInterface,
    P2P_SERVICES,
    P2P_SUBVERSION,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

def cleanup(func):
    def wrapper(self):
        assert len(self.nodes[0].getpeerinfo()) == 0
        assert len(self.nodes[0].getrawmempool()) == 0
        try:
            func(self)
        finally:
            # Clear mempool
            self.generate(self.nodes[0], 1)
            self.nodes[0].disconnect_p2ps()
    return wrapper

class PackageRelayer(P2PInterface):
    def __init__(self, send_sendpackages=True, send_wtxidrelay=True):
        super().__init__()
        # List versions of each sendpackages received
        self._sendpackages_received = []
        self._send_sendpackages = send_sendpackages
        self._send_wtxidrelay = send_wtxidrelay

    @property
    def sendpackages_received(self):
        with p2p_lock:
            return self._sendpackages_received

    def on_version(self, message):
        if self._send_wtxidrelay:
            self.send_without_ping(msg_wtxidrelay())
        if self._send_sendpackages:
            sendpackages_message = msg_sendpackages()
            sendpackages_message.versions = PKG_RELAY_PKGTXNS
            self.send_without_ping(sendpackages_message)
        self.send_without_ping(msg_verack())
        self.nServices = message.nServices

    def on_sendpackages(self, message):
        self._sendpackages_received.append(message.versions)

class PackageRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-packagerelay=1"]]

    @cleanup
    def test_sendpackages(self):
        node = self.nodes[0]

        self.log.info("Test sendpackages during version handshake")
        peer_normal = node.add_p2p_connection(PackageRelayer())
        assert peer_normal._sendpackages_received
        assert_equal(peer_normal._sendpackages_received, [PKG_RELAY_PKGTXNS])
        assert node.getpeerinfo()[0]["relaytxpackages"]
        node.disconnect_p2ps()

        self.log.info("Test sendpackages without wtxid relay")
        peer_no_wtxidrelay = node.add_p2p_connection(PackageRelayer(send_wtxidrelay=False))
        assert_equal(peer_no_wtxidrelay.sendpackages_received, [PKG_RELAY_PKGTXNS])
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        node.disconnect_p2ps()

        self.log.info("Test sendpackages when fRelay is false")
        node = self.nodes[0]
        peer_no_frelay = node.add_p2p_connection(PackageRelayer(), send_version=False, wait_for_verack=False)
        version_message = msg_version()
        version_message.nVersion = 70015
        version_message.strSubVer = P2P_SUBVERSION
        version_message.nServices = P2P_SERVICES
        version_message.relay = 1
        peer_no_frelay.send_without_ping(version_message)
        peer_no_frelay.wait_for_verack()
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        self.nodes[0].disconnect_p2ps()

        self.log.info("Test sendpackages is received even if we don't send it")
        node = self.nodes[0]
        peer_no_sendpackages = node.add_p2p_connection(PackageRelayer(send_sendpackages=False))
        # Sendpackages should still be sent
        assert_equal(node.getpeerinfo()[0]["bytessent_per_msg"]["sendpackages"], 32)
        assert_equal(peer_no_sendpackages.sendpackages_received, [PKG_RELAY_PKGTXNS])
        assert "sendpackages" not in node.getpeerinfo()[0]["bytesrecv_per_msg"]
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        node.disconnect_p2ps()

        self.log.info("Test disconnection if sendpackages is sent after version handshake")
        peer_sendpackages_after_verack = node.add_p2p_connection(PackageRelayer(), send_version=True, wait_for_verack=True)
        sendpackages_message = msg_sendpackages()
        sendpackages_message.versions = PKG_RELAY_PKGTXNS
        peer_sendpackages_after_verack.send_without_ping(sendpackages_message)
        peer_sendpackages_after_verack.wait_for_disconnect()

        self.log.info("Test sendpackages on a blocksonly node")

        self.restart_node(0, extra_args=["-blocksonly"])
        node.add_p2p_connection(PackageRelayer(), wait_for_verack=True)
        assert not node.getpeerinfo()[0]["relaytxpackages"]
        self.nodes[0].disconnect_p2ps()

    def run_test(self):
        self.test_sendpackages()

if __name__ == '__main__':
    PackageRelayTest(__file__).main()
