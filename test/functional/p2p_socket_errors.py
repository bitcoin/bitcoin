#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test P2P handling of TCP urgent data and resets while sending."""

import socket

from test_framework.messages import (
    CInv,
    MAGIC_BYTES,
    MSG_BLOCK,
    MSG_WITNESS_FLAG,
    msg_generic,
    msg_getdata,
    msg_ping,
    msg_version,
)
from test_framework.netutil import get_tcp_urgent_padding, pack_linger, send_tcp_urgent_data
from test_framework.p2p import NetworkThread, P2PInterface, P2P_VERSION
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, mine_large_block, p2p_port
from test_framework.wallet import MiniWallet


def reset_on_close(sock):
    """Make a subsequent close send a TCP reset and discard unsent data."""
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, pack_linger(enabled=True, timeout=0))


class SocketErrorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-maxsendbuffer=1", "-maxreceivebuffer=1"]]

    def run_test(self):
        self.message_builder = P2PInterface()
        self.message_builder.magic_bytes = MAGIC_BYTES[self.chain]
        self.version = msg_version()
        self.version.nVersion = P2P_VERSION
        self.check_urgent_bytes()
        self.check_urgent_pointer_ahead()
        self.check_reset_while_sending()

    def check_probe(self, send_probe, expected_bytes):
        node = self.nodes[0]
        with socket.create_connection(("127.0.0.1", p2p_port(0)), timeout=10) as peer:
            self.wait_until(lambda: len(node.getpeerinfo()) == 1)
            if self.windows_target:
                with node.assert_debug_log(["socket exceptional condition"]):
                    try:
                        send_probe(peer)
                    except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
                        # The node can disconnect before the bulk send finishes.
                        pass
                    self.wait_until(lambda: len(node.getpeerinfo()) == 0)
            else:
                send_probe(peer)
                # Processing the ordinary version message demonstrates progress
                # despite the urgent indication. The urgent byte is excluded.
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
                # Windows should disconnect immediately on the urgent indication.
                if not self.windows_target:
                    peer.sendall(version)

            self.check_probe(send_probe, len(version))

    def check_urgent_pointer_ahead(self):
        self.log.info("Check an urgent pointer ahead of ordinary P2P data")
        padding = get_tcp_urgent_padding(linux_host=self.linux_host)
        payload = self.version.serialize() + b"\x00" * padding
        version = self.message_builder.build_message(msg_generic(b"version", payload))

        def send_probe(peer):
            send_tcp_urgent_data(peer, version, linux_host=self.linux_host)

        self.check_probe(send_probe, len(version))

    def check_reset_while_sending(self):
        self.log.info("Check reset while the node has unsent block data")
        node = self.nodes[0]
        wallet = MiniWallet(node)
        mine_large_block(self, wallet, node)
        block_hash = node.getbestblockhash()
        block_size = len(node.getblock(block_hash, 0)) // 2 + 24
        peer = node.add_p2p_connection(P2PInterface(), supports_v2_p2p=False)
        peer.sync_with_ping()
        NetworkThread.network_event_loop.call_soon_threadsafe(peer._transport.pause_reading)
        requests = 16
        peer.send_without_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, int(block_hash, 16))] * requests))
        sent_bytes = self.wait_for_send_stall(node)
        assert_greater_than(requests * block_size, sent_bytes)
        # Exceed the receive-queue limit while processing is blocked by the
        # outgoing data. Wait until the node has read that first batch.
        pings = peer.build_message(msg_ping(nonce=123)) * 100
        received = node.getnettotals()["totalbytesrecv"] + len(pings)
        peer.send_raw_message(pings)
        self.wait_until(lambda: node.getnettotals()["totalbytesrecv"] == received)
        # More input is now queued in the kernel while receiving is paused.
        peer.send_raw_message(pings)
        reset_on_close(peer._transport.get_extra_info("socket"))
        expected_msgs = ["socket error or hangup"] if self.linux_host and not self.windows_target else []
        with node.assert_debug_log(expected_msgs):
            peer.peer_disconnect()
            self.wait_until(lambda: len(node.getpeerinfo()) == 0, timeout=10)
        assert_equal(node.getnettotals()["totalbytesrecv"], received)
        node.disconnect_p2ps()


if __name__ == '__main__':
    SocketErrorsTest(__file__).main()
