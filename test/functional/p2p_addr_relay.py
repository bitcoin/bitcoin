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
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
import random
import time

ADDRS = []
for i in range(10):
    addr = CAddress()
    addr.time = int(time.time()) + i
    addr.nServices = NODE_NETWORK | NODE_WITNESS
    addr.ip = "123.123.123.{}".format(i % 256)
    addr.port = 9333 + i
    ADDRS.append(addr)


class AddrReceiver(P2PInterface):
    _tokens = 1
    def on_addr(self, message):
        for addr in message.addrs:
            assert_equal(addr.nServices, 9)
            assert addr.ip.startswith('123.123.123.')
            assert (9333 <= addr.port < 9343)

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


class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

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

        self.rate_limit_tests()       

    def setup_rand_addr_msg(self, num):
        addrs = []
        for i in range(num):
            addr = CAddress()
            addr.time = self.mocktime + i
            addr.nServices = NODE_NETWORK | NODE_WITNESS
            addr.ip = f"{random.randrange(128,169)}.{random.randrange(1,255)}.{random.randrange(1,255)}.{random.randrange(1,255)}"
            addr.port = 9333
            addrs.append(addr)
        msg = msg_addr()
        msg.addrs = addrs
        return msg

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
