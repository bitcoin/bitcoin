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
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
import time

# Keep this with length <= 10. Addresses from larger messages are not relayed.
ADDRS = []
num_ipv4_addrs = 10

for i in range(num_ipv4_addrs):
    addr = CAddress()
    addr.time = int(time.time()) + i
    addr.nServices = NODE_NETWORK | NODE_WITNESS
    addr.ip = "123.123.123.{}".format(i % 256)
    addr.port = 8333 + i
    ADDRS.append(addr)


class AddrReceiver(P2PInterface):
    num_ipv4_received = 0

    def on_addr(self, message):
        for addr in message.addrs:
            assert_equal(addr.nServices, 9)
            if not 8333 <= addr.port < 8343:
                raise AssertionError("Invalid addr.port of {} (8333-8342 expected)".format(addr.port))
            assert addr.ip.startswith('123.123.123.')
            self.num_ipv4_received += 1


class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info('Create connection that sends addr messages')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        msg = msg_addr()

        self.log.info('Send too-large addr message')
        msg.addrs = ADDRS * 101  # more than 1000 addresses in one message
        with self.nodes[0].assert_debug_log(['addr message size = 1010']):
            addr_source.send_and_ping(msg)

        self.log.info('Check that addr message content is relayed and added to addrman')
        num_receivers = 7
        receivers = []
        for _ in range(num_receivers):
            receivers.append(self.nodes[0].add_p2p_connection(AddrReceiver()))
        msg.addrs = ADDRS
        with self.nodes[0].assert_debug_log(
            [
                'Added {} addresses from 127.0.0.1: 0 tried'.format(num_ipv4_addrs),
                'received: addr (301 bytes) peer=0',
            ]
        ):
            addr_source.send_and_ping(msg)
            self.nodes[0].setmocktime(int(time.time()) + 30 * 60)
            for receiver in receivers:
                receiver.sync_with_ping()

        total_ipv4_received = sum(r.num_ipv4_received for r in receivers)

        # Every IPv4 address must be relayed to two peers, other than the
        # originating node (addr_source).
        ipv4_branching_factor = 2
        assert_equal(total_ipv4_received, num_ipv4_addrs * ipv4_branching_factor)


if __name__ == '__main__':
    AddrTest().main()
