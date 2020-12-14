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
    assert_greater_than
)
import time

ONE_MINUTE = 60
ONE_HOUR = 60 * ONE_MINUTE
ONE_DAY = 24 * ONE_HOUR

def gen_addrs(n, time):
    addrs = []
    for i in range(n):
        addr = CAddress()
        addr.time = time + i
        addr.nServices = NODE_NETWORK | NODE_WITNESS
        addr.ip = "123.123.123.{}".format(i % 256)
        addr.port = 8333 + i
        addrs.append(addr)
    return addrs


class AddrReceiver(P2PInterface):
    received_addr = False

    def on_addr(self, message):
        for addr in message.addrs:
            assert_equal(addr.nServices, 9)
            assert addr.ip.startswith('123.123.123.')
            assert (8333 <= addr.port < 8343)
        self.received_addr = True


class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        self.log.info('Create connection that sends addr messages')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        msg = msg_addr()
        self.log.info('Send too-large addr message')
        msg.addrs = gen_addrs(1010, int(time.time()))
        with self.nodes[0].assert_debug_log(['addr message size = 1010']):
            addr_source.send_and_ping(msg)

        self.log.info('Check that addr message content is relayed and added to addrman')
        addr_receiver = self.nodes[0].add_p2p_connection(AddrReceiver())
        mocked_time = int(time.time())
        self.nodes[0].setmocktime(mocked_time)
        msg.addrs = gen_addrs(10, mocked_time)
        with self.nodes[0].assert_debug_log([
                'Added 10 addresses from 127.0.0.1: 0 tried',
                'received: addr (301 bytes) peer=0',
                'sending addr (301 bytes) peer=1',
        ]):
            addr_source.send_and_ping(msg)
            mocked_time += 30 * ONE_MINUTE
            self.nodes[0].setmocktime(mocked_time)
            addr_receiver.sync_with_ping()

        self.log.info('Check that within 24 hours an addr relay destination is rotated at most once')
        msg.addrs = gen_addrs(1, mocked_time)
        addr_receiver.received_addr = False
        addr_receivers = [addr_receiver]
        for _ in range(20):
            addr_receiver = self.nodes[0].add_p2p_connection(AddrReceiver())
            addr_receivers.append(addr_receiver)

        for _ in range(10):
            msg.addrs[0].time = mocked_time + 10 * ONE_MINUTE
            with self.nodes[0].assert_debug_log([
                'received: addr (31 bytes) peer=0',
            ]):
                addr_source.send_and_ping(msg)
                mocked_time += 2 * ONE_HOUR
                self.nodes[0].setmocktime(mocked_time)
                for receiver in addr_receivers:
                    receiver.sync_with_ping()
        nodes_received_addr = [node for node in addr_receivers if node.received_addr]

        # Per RelayAddress, we would announce these addrs to 2 destinations per day.
        # Since it's at most one rotation, at most 4 nodes can receive ADDR.
        ADDR_DESTINATIONS_THRESHOLD = 4
        assert(len(nodes_received_addr) <= ADDR_DESTINATIONS_THRESHOLD)

        self.log.info('Check that after several days an addr relay destination was rotated more than once')
        for node in addr_receivers:
            node.received_addr = False

        for _ in range(10):
            mocked_time += ONE_DAY
            msg.addrs[0].time = mocked_time + 10 * ONE_MINUTE
            self.nodes[0].setmocktime(mocked_time)
            with self.nodes[0].assert_debug_log([
                'received: addr (31 bytes) peer=0',
            ]):
                addr_source.send_and_ping(msg)
                mocked_time += ONE_HOUR  # should be enough to cover 30-min Poisson in most cases.
                self.nodes[0].setmocktime(mocked_time)
                for receiver in addr_receivers:
                    receiver.sync_with_ping()
        nodes_received_addr = [node for node in addr_receivers if node.received_addr]
        # Now that there should have been more than one rotation,
        # more than ADDR_DESTINATIONS_THRESHOLD nodes should have received ADDR.
        assert_greater_than(len(nodes_received_addr), ADDR_DESTINATIONS_THRESHOLD)




if __name__ == '__main__':
    AddrTest().main()
