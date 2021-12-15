#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test addr relay
"""

import random
import time

from test_framework.messages import (
    CAddress,
    msg_addr,
    msg_getaddr,
    msg_verack,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
    P2P_SERVICES,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than


class AddrReceiver(P2PInterface):
    num_ipv4_received = 0
    test_addr_contents = False
    _tokens = 1
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

    def on_getaddr(self, message):
        # When the node sends us a getaddr, it increments the addr relay tokens for the connection by 1000
        self._tokens += 1000

    @property
    def tokens(self):
        with p2p_lock:
            return self._tokens

    def increment_tokens(self, n):
        # When we move mocktime forward, the node increments the addr relay tokens for its peers
        with p2p_lock:
            self._tokens += n

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
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

    def run_test(self):
        self.oversized_addr_test()
        self.relay_tests()
        self.inbound_blackhole_tests()

        # This test populates the addrman, which can impact the node's behavior
        # in subsequent tests
        self.getaddr_tests()
        self.blocksonly_mode_tests()
        self.rate_limit_tests()

    def setup_addr_msg(self, num):
        addrs = []
        for i in range(num):
            addr = CAddress()
            addr.time = self.mocktime + i
            addr.nServices = P2P_SERVICES
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
            addr.nServices = P2P_SERVICES
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
        inbound_peer = self.nodes[0].add_p2p_connection(AddrReceiver(test_addr_contents=True, send_getaddr=False))
        full_outbound_peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=0, connection_type="outbound-full-relay")
        msg = self.setup_addr_msg(2)
        self.send_addr_msg(full_outbound_peer, msg, [inbound_peer])
        self.log.info('Check that the first addr message received from an outbound peer is not relayed')
        # Currently, there is a flag that prevents the first addr message received
        # from a new outbound peer to be relayed to others. Originally meant to prevent
        # large GETADDR responses from being relayed, it now typically affects the self-announcement
        # of the outbound peer which is often sent before the GETADDR response.
        assert_equal(inbound_peer.num_ipv4_received, 0)

        # Send an empty ADDR message to initialize address relay on this connection.
        inbound_peer.send_and_ping(msg_addr())

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
        return sum(bytes_received for (msg, bytes_received) in msgs_dict.items() if msg in ['addr', 'addrv2', 'getaddr'])

    def inbound_blackhole_tests(self):
        self.log.info('Check that we only relay addresses to inbound peers who have previously sent us addr related messages')

        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        receiver_peer = self.nodes[0].add_p2p_connection(AddrReceiver())
        blackhole_peer = self.nodes[0].add_p2p_connection(AddrReceiver(send_getaddr=False))
        initial_addrs_received = receiver_peer.num_ipv4_received

        peerinfo = self.nodes[0].getpeerinfo()
        assert_equal(peerinfo[0]['addr_relay_enabled'], True)  # addr_source
        assert_equal(peerinfo[1]['addr_relay_enabled'], True)  # receiver_peer
        assert_equal(peerinfo[2]['addr_relay_enabled'], False)  # blackhole_peer

        # addr_source sends 2 addresses to node0
        msg = self.setup_addr_msg(2)
        addr_source.send_and_ping(msg)
        self.mocktime += 30 * 60
        self.nodes[0].setmocktime(self.mocktime)
        receiver_peer.sync_with_ping()
        blackhole_peer.sync_with_ping()

        peerinfo = self.nodes[0].getpeerinfo()

        # Confirm node received addr-related messages from receiver peer
        assert_greater_than(self.sum_addr_messages(peerinfo[1]['bytesrecv_per_msg']), 0)
        # And that peer received addresses
        assert_equal(receiver_peer.num_ipv4_received - initial_addrs_received, 2)

        # Confirm node has not received addr-related messages from blackhole peer
        assert_equal(self.sum_addr_messages(peerinfo[2]['bytesrecv_per_msg']), 0)
        # And that peer did not receive addresses
        assert_equal(blackhole_peer.num_ipv4_received, 0)

        self.log.info("After blackhole peer sends addr message, it becomes eligible for addr gossip")
        blackhole_peer.send_and_ping(msg_addr())

        # Confirm node has now received addr-related messages from blackhole peer
        assert_greater_than(self.sum_addr_messages(peerinfo[1]['bytesrecv_per_msg']), 0)
        assert_equal(self.nodes[0].getpeerinfo()[2]['addr_relay_enabled'], True)

        msg = self.setup_addr_msg(2)
        self.send_addr_msg(addr_source, msg, [receiver_peer, blackhole_peer])

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

    def send_addrs_and_test_rate_limiting(self, peer, no_relay, *, new_addrs, total_addrs):
        """Send an addr message and check that the number of addresses processed and rate-limited is as expected"""

        peer.send_and_ping(self.setup_rand_addr_msg(new_addrs))

        peerinfo = self.nodes[0].getpeerinfo()[0]
        addrs_processed = peerinfo['addr_processed']
        addrs_rate_limited = peerinfo['addr_rate_limited']
        self.log.debug(f"addrs_processed = {addrs_processed}, addrs_rate_limited = {addrs_rate_limited}")

        if no_relay:
            assert_equal(addrs_processed, 0)
            assert_equal(addrs_rate_limited, 0)
        else:
            assert_equal(addrs_processed, min(total_addrs, peer.tokens))
            assert_equal(addrs_rate_limited, max(0, total_addrs - peer.tokens))

    def rate_limit_tests(self):
        self.mocktime = int(time.time())
        self.restart_node(0, [])
        self.nodes[0].setmocktime(self.mocktime)

        for conn_type, no_relay in [("outbound-full-relay", False), ("block-relay-only", True), ("inbound", False)]:
            self.log.info(f'Test rate limiting of addr processing for {conn_type} peers')
            if conn_type == "inbound":
                peer = self.nodes[0].add_p2p_connection(AddrReceiver())
            else:
                peer = self.nodes[0].add_outbound_p2p_connection(AddrReceiver(), p2p_idx=0, connection_type=conn_type)

            # Send 600 addresses. For all but the block-relay-only peer this should result in addresses being processed.
            self.send_addrs_and_test_rate_limiting(peer, no_relay, new_addrs=600, total_addrs=600)

            # Send 600 more addresses. For the outbound-full-relay peer (which we send a GETADDR, and thus will
            # process up to 1001 incoming addresses), this means more addresses will be processed.
            self.send_addrs_and_test_rate_limiting(peer, no_relay, new_addrs=600, total_addrs=1200)

            # Send 10 more. As we reached the processing limit for all nodes, no more addresses should be procesesd.
            self.send_addrs_and_test_rate_limiting(peer, no_relay, new_addrs=10, total_addrs=1210)

            # Advance the time by 100 seconds, permitting the processing of 10 more addresses.
            # Send 200 and verify that 10 are processed.
            self.mocktime += 100
            self.nodes[0].setmocktime(self.mocktime)
            peer.increment_tokens(10)

            self.send_addrs_and_test_rate_limiting(peer, no_relay, new_addrs=200, total_addrs=1410)

            # Advance the time by 1000 seconds, permitting the processing of 100 more addresses.
            # Send 200 and verify that 100 are processed.
            self.mocktime += 1000
            self.nodes[0].setmocktime(self.mocktime)
            peer.increment_tokens(100)

            self.send_addrs_and_test_rate_limiting(peer, no_relay, new_addrs=200, total_addrs=1610)

            self.nodes[0].disconnect_p2ps()


if __name__ == '__main__':
    AddrTest().main()
