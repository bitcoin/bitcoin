#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test P2P handling of TCP urgent bytes."""

import socket

from test_framework.messages import MAGIC_BYTES, msg_version
from test_framework.p2p import P2PInterface, P2P_VERSION
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, p2p_port


class SocketErrorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.message_builder = P2PInterface()
        self.message_builder.magic_bytes = MAGIC_BYTES[self.chain]
        self.version = msg_version()
        self.version.nVersion = P2P_VERSION
        self.check_urgent_bytes()

    def check_probe(self, send_probe, expected_bytes):
        node = self.nodes[0]
        with socket.create_connection(("127.0.0.1", p2p_port(0)), timeout=10) as peer:
            self.wait_until(lambda: len(node.getpeerinfo()) == 1)
            send_probe(peer)
            # The node ignores the urgent byte: the connection stays open,
            # the ordinary version message is processed and the urgent byte
            # is not counted as received.
            def received_version():
                peers = node.getpeerinfo()
                assert_equal(len(peers), 1)
                return peers[0]["version"] == self.version.nVersion

            self.wait_until(received_version)
            assert_equal(node.getpeerinfo()[0]["bytesrecv"], expected_bytes)
        self.wait_until(lambda: len(node.getpeerinfo()) == 0)

    def check_urgent_bytes(self):
        self.log.info("Check handling of individual urgent bytes")
        version = self.message_builder.build_message(self.version)
        # Use a fresh connection for each byte so earlier urgent indications
        # cannot be overwritten before they are observed by the node.
        for byte in b"URG":
            def send_probe(peer):
                peer.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                assert_equal(peer.send(bytes([byte]), socket.MSG_OOB), 1)
                peer.sendall(version)

            self.check_probe(send_probe, len(version))


if __name__ == '__main__':
    SocketErrorsTest(__file__).main()
