#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test addr relay
"""

from test_framework.messages import (
    CAddress,
    NODE_NETWORK,
    NODE_WITNESS,
    msg_addr,
    msg_getaddr,
    msg_verack
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
import time


class AddrReceiver(P2PInterface):
    num_ipv4_received = 0
    test_addr_contents = False
    send_getaddr = True

    def __init__(self, test_addr_contents=False, send_getaddr=True):
        super().__init__()
        self.test_addr_contents = test_addr_contents
        self.send_getaddr = send_getaddr

    def on_addr(self, message):
        for addr in message.addrs:
            self.num_ipv4_received += 1
            if(self.test_addr_contents):
                # relay_tests checks the content of the addr messages match
                # expectations based on the message creation in setup_addr_msg
                assert_equal(addr.nServices, 9)
                if not 8333 <= addr.port < 8343:
                    raise AssertionError("Invalid addr.port of {} (8333-8342 expected)".format(addr.port))
                assert addr.ip.startswith('123.123.123.')

    def addr_received(self):
        return self.num_ipv4_received != 0

    def on_version(self, message):
        self.send_message(msg_verack())
        if (self.send_getaddr):
            self.send_message(msg_getaddr())

    def getaddr_received(self):
        return self.message_count['getaddr'] > 0


class AddrTest(BitcoinTestFramework):
    counter = 0
    mocktime = int(time.time())

    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.oversized_addr_test()
        self.relay_tests()
        self.inbound_blackhole_tests()

        # This test populates the addrman, which can impact the node's behavior
        # in subsequent tests
        self.getaddr_tests()
        self.blocksonly_mode_tests()

    def setup_addr_msg(self, num):
        addrs = []
        for i in range(num):
            addr = CAddress()
            addr.time = self.mocktime + i
            addr.nServices = NODE_NETWORK | NODE_WITNESS
            addr.ip = f"123.123.123.{self.counter % 256}"
            addr.port = 8333 + i
            addrs.append(addr)
            self.counter += 1

        msg = msg_addr()
        msg.addrs = addrs
        return msg

    def send_addr_msg(self, source, msg, receivers):
        source.send_and_ping(msg)
        # pop m_next_addr_send timer
        self.mocktime += 10 * 60
        self.nodes[0].setmocktime(self.mocktime)
        for peer in receivers:
            peer.sync_send_with_ping()

    def oversized_addr_test(self):
        self.log.info('Send an addr message that is too large')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())

        msg = self.setup_addr_msg(1010)
        with self.nodes[0].assert_debug_log(['addr message size = 1010']):
            addr_source.send_and_ping(msg)

        self.nodes[0].disconnect_p2ps()

    def relay_tests(self):
        self.log.info('Test address relay')
        self.log.info('Check that addr message content is relayed and added to addrman')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        num_receivers = 7
        receivers = []
        for _ in range(num_receivers):
            receivers.append(self.nodes[0].add_p2p_connection(AddrReceiver(test_addr_contents=True)))

        # Keep this with length <= 10. Addresses from larger messages are not
        # relayed.
        num_ipv4_addrs = 10
        msg = self.setup_addr_msg(num_ipv4_addrs)
        with self.nodes[0].assert_debug_log(
            [
                'Added {} addresses from 127.0.0.1: 0 tried'.format(num_ipv4_addrs),
                'received: addr (301 bytes) peer=1',
            ]
        ):
            self.send_addr_msg(addr_source, msg, receivers)

        total_ipv4_received = sum(r.num_ipv4_received for r in receivers)

        # Every IPv4 address must be relayed to two peers, other than the
        # originating node (addr_source).
        ipv4_branching_factor = 2
        assert_equal(total_ipv4_received, num_ipv4_addrs * ipv4_branching_factor)

        self.nodes[0].disconnect_p2ps()

        self.log.info('Check relay of addresses received from outbound peers')
        inbound_peer = self.nodes[0].add_p2p_connection(AddrReceiver(test_addr_contents=True))
        full_outbound_peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=0, connection_type="outbound-full-relay")
        msg = self.setup_addr_msg(2)
        self.send_addr_msg(full_outbound_peer, msg, [inbound_peer])
        self.log.info('Check that the first addr message received from an outbound peer is not relayed')
        # Currently, there is a flag that prevents the first addr message received
        # from a new outbound peer to be relayed to others. Originally meant to prevent
        # large GETADDR responses from being relayed, it now typically affects the self-announcement
        # of the outbound peer which is often sent before the GETADDR response.
        assert_equal(inbound_peer.num_ipv4_received, 0)

        self.log.info('Check that subsequent addr messages sent from an outbound peer are relayed')
        msg2 = self.setup_addr_msg(2)
        self.send_addr_msg(full_outbound_peer, msg2, [inbound_peer])
        assert_equal(inbound_peer.num_ipv4_received, 2)

        self.log.info('Check address relay to outbound peers')
        block_relay_peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=1, connection_type="block-relay-only")
        msg3 = self.setup_addr_msg(2)
        self.send_addr_msg(inbound_peer, msg3, [full_outbound_peer, block_relay_peer])

        self.log.info('Check that addresses are relayed to full outbound peers')
        assert_equal(full_outbound_peer.num_ipv4_received, 2)
        self.log.info('Check that addresses are not relayed to block-relay-only outbound peers')
        assert_equal(block_relay_peer.num_ipv4_received, 0)

        self.nodes[0].disconnect_p2ps()

    def sum_addr_messages(self, msgs_dict):
        val = 0
        for msg in ['addr', 'addrv2', 'getaddr', 'sendaddrv2']:
            if msg in msgs_dict:
                val += msgs_dict[msg]
        return val

    def inbound_blackhole_tests(self):
        self.log.info('Check that we only relay addresses to inbound peers who have previously sent us addr related messages')

        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        receiver_peer = self.nodes[0].add_p2p_connection(AddrReceiver())
        blackhole_peer = self.nodes[0].add_p2p_connection(AddrReceiver(send_getaddr=False))

        msg = self.setup_addr_msg(2)
        addr_source.send_and_ping(msg)
        self.mocktime += 30 * 60
        self.nodes[0].setmocktime(self.mocktime)
        receiver_peer.sync_with_ping()
        blackhole_peer.sync_with_ping()

        peerinfo = self.nodes[0].getpeerinfo()

        # Confirm node received addr-related messages from receiver peer
        assert self.sum_addr_messages(peerinfo[1]['bytesrecv_per_msg']) > 0
        # And that peer received addresses
        assert_equal(receiver_peer.num_ipv4_received, 2)

        # Confirm node has not received addr-related messages from blackhole peer
        assert self.sum_addr_messages(peerinfo[2]['bytesrecv_per_msg']) == 0
        # And that peer did not receive addresses
        assert_equal(blackhole_peer.num_ipv4_received, 0)

        self.log.info("After blackhole peer sends addr message, it becomes eligible for trickle relay")
        blackhole_peer.send_and_ping(msg_addr())
        msg = self.setup_addr_msg(2)
        self.send_addr_msg(addr_source, msg, [receiver_peer, blackhole_peer])

        # Confirm node has now received addr-related messages from blackhole peer
        assert self.sum_addr_messages(peerinfo[1]['bytesrecv_per_msg']) > 0
        # And that peer received addresses
        assert_equal(blackhole_peer.num_ipv4_received, 2)

        self.nodes[0].disconnect_p2ps()

    def getaddr_tests(self):
        # In the previous tests, the node answered GETADDR requests with an
        # empty addrman. Due to GETADDR response caching (see
        # CConnman::GetAddresses), the node would continue to provide 0 addrs
        # in response until enough time has passed or the node is restarted.
        self.restart_node(0)

        self.log.info('Test getaddr behavior')
        self.log.info('Check that we send a getaddr message upon connecting to an outbound-full-relay peer')
        full_outbound_peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=0, connection_type="outbound-full-relay")
        full_outbound_peer.sync_with_ping()
        assert full_outbound_peer.getaddr_received()

        self.log.info('Check that we do not send a getaddr message upon connecting to a block-relay-only peer')
        block_relay_peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=1, connection_type="block-relay-only")
        block_relay_peer.sync_with_ping()
        assert_equal(block_relay_peer.getaddr_received(), False)

        self.log.info('Check that we answer getaddr messages only from inbound peers')
        inbound_peer = self.nodes[0].add_p2p_connection(AddrReceiver(send_getaddr=False))
        inbound_peer.sync_with_ping()

        # Add some addresses to addrman
        for i in range(1000):
            first_octet = i >> 8
            second_octet = i % 256
            a = f"{first_octet}.{second_octet}.1.1"
            self.nodes[0].addpeeraddress(a, 8333)

        full_outbound_peer.send_and_ping(msg_getaddr())
        block_relay_peer.send_and_ping(msg_getaddr())
        inbound_peer.send_and_ping(msg_getaddr())

        self.mocktime += 5 * 60
        self.nodes[0].setmocktime(self.mocktime)
        inbound_peer.wait_until(lambda: inbound_peer.addr_received() is True)

        assert_equal(full_outbound_peer.num_ipv4_received, 0)
        assert_equal(block_relay_peer.num_ipv4_received, 0)
        assert inbound_peer.num_ipv4_received > 100

        self.nodes[0].disconnect_p2ps()

    def blocksonly_mode_tests(self):
        self.log.info('Test addr relay in -blocksonly mode')
        self.restart_node(0, ["-blocksonly"])
        self.mocktime = int(time.time())

        self.log.info('Check that we send getaddr messages')
        full_outbound_peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=0, connection_type="outbound-full-relay")
        full_outbound_peer.sync_with_ping()
        assert full_outbound_peer.getaddr_received()

        self.log.info('Check that we relay address messages')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        msg = self.setup_addr_msg(2)
        self.send_addr_msg(addr_source, msg, [full_outbound_peer])
        assert_equal(full_outbound_peer.num_ipv4_received, 2)

        self.nodes[0].disconnect_p2ps()


if __name__ == '__main__':
    AddrTest().main()
