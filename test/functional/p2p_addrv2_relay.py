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

class AddrTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

    def run_test(self):
        self.log.info('Check disconnection when sending sendaddrv2 after verack')
        conn = self.nodes[0].add_p2p_connection(P2PInterface())
        with self.nodes[0].assert_debug_log(['sendaddrv2 received after verack from peer=0; disconnecting']):
            conn.send_message(msg_sendaddrv2())
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
            addr_source.send_message(msg)
            addr_source.wait_for_disconnect()



if __name__ == '__main__':
    AddrTest(__file__).main()
