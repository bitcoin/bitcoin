#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test addrv2 relay
"""

import time

from test_framework.messages import (
    CAddress,
    msg_addrv2,
    msg_sendaddrv2,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

I2P_ADDR = "c4gfnttsuwqomiygupdqqqyy5y5emnk5c73hrfvatri67prd7vyq.b32.i2p"
ONION_ADDR = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion"

ADDRS = []
for i in range(10):
    addr = CAddress()
    addr.time = int(time.time()) + i
    addr.port = 8333 + i
    addr.nServices = P2P_SERVICES
    # Add one I2P and one onion V3 address at an arbitrary position.
    if i == 5:
        addr.net = addr.NET_I2P
        addr.ip = I2P_ADDR
        addr.port = 0
    elif i == 8:
        addr.net = addr.NET_TORV3
        addr.ip = ONION_ADDR
    else:
        addr.ip = f"123.123.123.{i % 256}"
    ADDRS.append(addr)


class AddrReceiver(P2PInterface):
    addrv2_received_and_checked = False

    def __init__(self):
        super().__init__(support_addrv2 = True)

    def on_addrv2(self, message):
        expected_set = set((addr.ip, addr.port) for addr in ADDRS)
        received_set = set((addr.ip, addr.port) for addr in message.addrs)
        if expected_set == received_set:
            self.addrv2_received_and_checked = True

    def wait_for_addrv2(self):
        self.wait_until(lambda: "addrv2" in self.last_message)


def calc_addrv2_msg_size(addrs):
    size = 1  # vector length byte
    for addr in addrs:
        size += 4  # time
        size += 1  # services, COMPACTSIZE(P2P_SERVICES)
        size += 1  # network id
        size += 1  # address length byte
        size += addr.ADDRV2_ADDRESS_LENGTH[addr.net]  # address
        size += 2  # port
    return size

class SelfAnnoucementReceiver(P2PInterface):
    self_announcements_received = 0
    addresses_received = 0
    addr_messages_received = 0
    expected = None

    def __init__(self, expected):
        super().__init__(support_addrv2 = True)
        self.expected = expected

    def on_addrv2(self, message):
        self.addr_messages_received += 1
        for addr in message.addrs:
            self.addresses_received += 1
            if addr == self.expected:
                self.self_announcements_received += 1

class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

    def run_test(self):
        self.addrv2_relay_tests()
        self.self_annoucement_test()

    def addrv2_relay_tests(self):
        self.log.info('Check disconnection when sending sendaddrv2 after verack')
        conn = self.nodes[0].add_p2p_connection(P2PInterface())
        with self.nodes[0].assert_debug_log(['sendaddrv2 received after verack, disconnecting peer=0']):
            conn.send_without_ping(msg_sendaddrv2())
            conn.wait_for_disconnect()

        self.log.info('Create connection that sends addrv2 messages')
        addr_source = self.nodes[0].add_p2p_connection(P2PInterface())
        msg = msg_addrv2()

        self.log.info('Check that addrv2 message content is relayed and added to addrman')
        addr_receiver = self.nodes[0].add_p2p_connection(AddrReceiver())
        msg.addrs = ADDRS
        msg_size = calc_addrv2_msg_size(ADDRS)
        with self.nodes[0].assert_debug_log([
                f'received: addrv2 ({msg_size} bytes) peer=1',
                f'sending addrv2 ({msg_size} bytes) peer=2',
        ]):
            addr_source.send_and_ping(msg)
            self.nodes[0].setmocktime(int(time.time()) + 30 * 60)
            addr_receiver.wait_for_addrv2()

        assert addr_receiver.addrv2_received_and_checked
        assert_equal(len(self.nodes[0].getnodeaddresses(count=0, network="i2p")), 0)

        self.log.info('Send too-large addrv2 message')
        msg.addrs = ADDRS * 101
        with self.nodes[0].assert_debug_log(['addrv2 message size = 1010']):
            addr_source.send_without_ping(msg)
            addr_source.wait_for_disconnect()

    def self_annoucement_test(self):
        IP_TO_ANNOUNCE = "42.42.42.42"
        self.restart_node(0, [f"-externalip={IP_TO_ANNOUNCE}"])
        self.log.info('Test that the node does an address self-annoucement')

        # We only self-announce after initial block download is done
        assert(not self.nodes[0].getblockchaininfo()["initialblockdownload"])

        # Use mocktime to freeze time to make sure we can always match the
        # timestamp in the self-annoucement.
        self.mocktime = int(time.time())
        self.nodes[0].setmocktime(self.mocktime)

        port = self.nodes[0].getnetworkinfo()["localaddresses"][0]["port"]
        expected = CAddress()
        expected.nServices = 1033
        expected.ip = IP_TO_ANNOUNCE
        expected.port = port
        expected.time = self.mocktime

        with self.nodes[0].assert_debug_log([f'Advertising address {IP_TO_ANNOUNCE}:{port}']):
            addr_receiver = self.nodes[0].add_p2p_connection(SelfAnnoucementReceiver(expected=expected))
            addr_receiver.sync_with_ping()

        # We expect one self-annoucement and multiple other addresses in
        # response to a GETADDR.
        assert_equal(addr_receiver.self_announcements_received, 1)
        assert_greater_than(addr_receiver.addresses_received, 1)

if __name__ == '__main__':
    AddrTest(__file__).main()
