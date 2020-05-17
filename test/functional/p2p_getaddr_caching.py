#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test addr response caching"""

import time
from test_framework.messages import (
    CAddress,
    NODE_NETWORK,
    NODE_WITNESS,
    msg_addr,
    msg_getaddr,
)
from test_framework.mininode import (
    P2PInterface,
    mininode_lock
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

MAX_ADDR_TO_SEND = 1000

def gen_addrs(n):
    addrs = []
    for i in range(n):
        addr = CAddress()
        addr.time = int(time.time())
        addr.nServices = NODE_NETWORK | NODE_WITNESS
        # Use first octets to occupy different AddrMan buckets
        first_octet = i >> 8
        second_octet = i % 256
        addr.ip = "{}.{}.1.1".format(first_octet, second_octet)
        addr.port = 8333
        addrs.append(addr)
    return addrs

class AddrReceiver(P2PInterface):

    def __init__(self):
        super().__init__()
        self.received_addrs = None

    def get_received_addrs(self):
        with mininode_lock:
            return self.received_addrs

    def on_addr(self, message):
        self.received_addrs = []
        for addr in message.addrs:
            self.received_addrs.append(addr.ip)

    def addr_received(self):
        return self.received_addrs is not None


class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def run_test(self):
        self.log.info('Create connection that sends and requests addr messages')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())

        msg_send_addrs = msg_addr()
        self.log.info('Fill peer AddrMan with a lot of records')
        # Since these addrs are sent from the same source, not all of them will be stored,
        # because we allocate a limited number of AddrMan buckets per addr source.
        total_addrs = 10000
        addrs = gen_addrs(total_addrs)
        for i in range(int(total_addrs/MAX_ADDR_TO_SEND)):
            msg_send_addrs.addrs = addrs[i * MAX_ADDR_TO_SEND:(i + 1) * MAX_ADDR_TO_SEND]
            addr_source.send_and_ping(msg_send_addrs)

        responses = []
        self.log.info('Send many addr requests within short time to receive same response')
        N = 5
        cur_mock_time = int(time.time())
        for i in range(N):
            addr_receiver = self.nodes[0].add_p2p_connection(AddrReceiver())
            addr_receiver.send_and_ping(msg_getaddr())
            # Trigger response
            cur_mock_time += 5 * 60
            self.nodes[0].setmocktime(cur_mock_time)
            addr_receiver.wait_until(addr_receiver.addr_received)
            responses.append(addr_receiver.get_received_addrs())
        for response in responses[1:]:
            assert_equal(response, responses[0])
        assert(len(response) < MAX_ADDR_TO_SEND)

        cur_mock_time += 3 * 24 * 60 * 60
        self.nodes[0].setmocktime(cur_mock_time)

        self.log.info('After time passed, see a new response to addr request')
        last_addr_receiver = self.nodes[0].add_p2p_connection(AddrReceiver())
        last_addr_receiver.send_and_ping(msg_getaddr())
        # Trigger response
        cur_mock_time += 5 * 60
        self.nodes[0].setmocktime(cur_mock_time)
        last_addr_receiver.wait_until(last_addr_receiver.addr_received)
        # new response is different
        assert(set(responses[0]) != set(last_addr_receiver.get_received_addrs()))


if __name__ == '__main__':
    AddrTest().main()
