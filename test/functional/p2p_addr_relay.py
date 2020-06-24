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
    msg_addr,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
import time

ADDRS = []
for i in range(10):
    addr = CAddress()
    addr.time = int(time.time()) + i
    addr.nServices = NODE_NETWORK
    addr.ip = "123.123.123.{}".format(i % 256)
    addr.port = 8333 + i
    ADDRS.append(addr)


class AddrReceiver(P2PInterface):
    def on_addr(self, message):
        for addr in message.addrs:
            assert_equal(addr.nServices, 9)
            assert addr.ip.startswith('123.123.123.')
            assert (8333 <= addr.port < 8343)


class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info('Create connection that sends addr messages')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        msg = msg_addr()

        self.log.info('Send too-large addr message')
        msg.addrs = ADDRS * 101
        with self.nodes[0].assert_debug_log(['addr message size = 1010']):
            addr_source.send_and_ping(msg)

        self.log.info('Check that addr message content is relayed and added to addrman')
        addr_receiver = self.nodes[0].add_p2p_connection(AddrReceiver())
        msg.addrs = ADDRS
        with self.nodes[0].assert_debug_log([
                'Added 10 addresses from 127.0.0.1: 0 tried',
                'received: addr (301 bytes) peer=0',
        ]):
            addr_source.send_and_ping(msg)
            self.nodes[0].setmocktime(int(time.time()) + 30 * 60)
            addr_receiver.sync_with_ping()


if __name__ == '__main__':
    AddrTest().main()
