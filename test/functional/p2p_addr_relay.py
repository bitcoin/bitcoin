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
    msg_getaddr
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)
import os
import random
import time


class AddrReceiver(P2PInterface):
    num_ipv4_received = 0
    test_addr_contents = False

    def __init__(self, test_addr_contents=False):
        super().__init__()
        self.test_addr_contents = test_addr_contents

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

    def getaddr_received(self):
        return self.message_count['getaddr'] > 0


class AddrTest(BitcoinTestFramework):
    counter = 0
    mocktime = int(time.time())

    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

    def run_test(self):
        self.oversized_addr_test()
        self.relay_tests()
        self.getaddr_tests()
        self.blocksonly_mode_tests()
        self.rate_limit_tests()

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

    def setup_rand_addr_msg(self, num):
        addrs = []
        for i in range(num):
            addr = CAddress()
            addr.time = self.mocktime + i
            addr.nServices = NODE_NETWORK | NODE_WITNESS
            addr.ip = f"{random.randrange(128,169)}.{random.randrange(1,255)}.{random.randrange(1,255)}.{random.randrange(1,255)}"
            addr.port = 8333
            addrs.append(addr)
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

    def getaddr_tests(self):
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
        inbound_peer = self.nodes[0].add_p2p_connection(AddrReceiver())
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
        self.restart_node(0, ["-blocksonly", "-whitelist=addr@127.0.0.1"])
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

    def rate_limit_tests(self):

        for contype, tokens, no_relay in [("outbound-full-relay", 1001, False), ("block-relay-only", 0, True), ("inbound", 1, False)]:
            self.log.info(f'Test rate limiting of addr processing for {contype} peers')
            self.stop_node(0)
            os.remove(os.path.join(self.nodes[0].datadir, "regtest", "peers.dat"))
            self.start_node(0, [])
            self.mocktime = int(time.time())
            self.nodes[0].setmocktime(self.mocktime)
            if contype == "inbound":
                peer = self.nodes[0].add_p2p_connection(AddrReceiver())
            else:
                peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=0, connection_type=contype)

            # Check that we start off with empty addrman
            addr_count_0 = len(self.nodes[0].getnodeaddresses(0))
            assert_equal(addr_count_0, 0)

            # Send 600 addresses. For all but the block-relay-only peer this should result in at least 1 address.
            peer.send_and_ping(self.setup_rand_addr_msg(600))
            addr_count_1 = len(self.nodes[0].getnodeaddresses(0))
            assert_greater_than_or_equal(tokens, addr_count_1)
            assert_greater_than_or_equal(addr_count_0 + 600, addr_count_1)
            assert_equal(addr_count_1 > addr_count_0, tokens > 0)

            # Send 600 more addresses. For the outbound-full-relay peer (which we send a GETADDR, and thus will
            # process up to 1001 incoming addresses), this means more entries will appear.
            peer.send_and_ping(self.setup_rand_addr_msg(600))
            addr_count_2 = len(self.nodes[0].getnodeaddresses(0))
            assert_greater_than_or_equal(tokens, addr_count_2)
            assert_greater_than_or_equal(addr_count_1 + 600, addr_count_2)
            assert_equal(addr_count_2 > addr_count_1, tokens > 600)

            # Send 10 more. As we reached the processing limit for all nodes, this should have no effect.
            peer.send_and_ping(self.setup_rand_addr_msg(10))
            addr_count_3 = len(self.nodes[0].getnodeaddresses(0))
            assert_greater_than_or_equal(tokens, addr_count_3)
            assert_equal(addr_count_2, addr_count_3)

            # Advance the time by 100 seconds, permitting the processing of 10 more addresses. Send 200,
            # but verify that no more than 10 are processed.
            self.mocktime += 100
            self.nodes[0].setmocktime(self.mocktime)
            new_tokens = 0 if no_relay else 10
            tokens += new_tokens
            peer.send_and_ping(self.setup_rand_addr_msg(200))
            addr_count_4 = len(self.nodes[0].getnodeaddresses(0))
            assert_greater_than_or_equal(tokens, addr_count_4)
            assert_greater_than_or_equal(addr_count_3 + new_tokens, addr_count_4)

            # Advance the time by 1000 seconds, permitting the processing of 100 more addresses. Send 200,
            # but verify that no more than 100 are processed (and at least some).
            self.mocktime += 1000
            self.nodes[0].setmocktime(self.mocktime)
            new_tokens = 0 if no_relay else 100
            tokens += new_tokens
            peer.send_and_ping(self.setup_rand_addr_msg(200))
            addr_count_5 = len(self.nodes[0].getnodeaddresses(0))
            assert_greater_than_or_equal(tokens, addr_count_5)
            assert_greater_than_or_equal(addr_count_4 + new_tokens, addr_count_5)
            assert_equal(addr_count_5 > addr_count_4, not no_relay)

            self.nodes[0].disconnect_p2ps()

if __name__ == '__main__':
    AddrTest().main()
