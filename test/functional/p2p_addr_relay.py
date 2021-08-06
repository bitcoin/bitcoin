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
    assert_greater_than_or_equal,
)
import os
import random
import time

ADDRS = []
for i in range(10):
    addr = CAddress()
    addr.time = int(time.time()) + i
    addr.nServices = NODE_NETWORK | NODE_WITNESS
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
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

    def setup_rand_addr_msg(self, num):
        addrs = []
        for i in range(num):
            addr = CAddress()
            addr.time = self.mocktime + i
            addr.nServices = NODE_NETWORK | NODE_WITNESS
            addr.ip = "%i.%i.%i.%i" % (random.randrange(128,169), random.randrange(1,255), random.randrange(1,255), random.randrange(1,255))
            addr.port = 8333
            addrs.append(addr)
        msg = msg_addr()
        msg.addrs = addrs
        return msg

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
                'sending addr (301 bytes) peer=1',
        ]):
            addr_source.send_and_ping(msg)
            self.nodes[0].setmocktime(int(time.time()) + 30 * 60)
            addr_receiver.sync_with_ping()

        # The following test is backported. The original test also verified behavior for
        # outbound peers, but lacking add_outbound_p2p_connection, those tests have been
        # removed here.
        for contype, tokens, no_relay in [("inbound", 1, False)]:
            self.log.info('Test rate limiting of addr processing for %s peers' % contype)
            self.stop_node(0)
            os.remove(os.path.join(self.nodes[0].datadir, "regtest", "peers.dat"))
            self.start_node(0, [])
            self.mocktime = int(time.time())
            self.nodes[0].setmocktime(self.mocktime)
            peer = self.nodes[0].add_p2p_connection(AddrReceiver())

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
