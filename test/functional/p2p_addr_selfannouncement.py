#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that a node sends a self-announcement with its external IP to
in- and outbound peers after connection open and again sometime later.
"""

import time

from test_framework.messages import (
    CAddress,
    from_hex,
    msg_headers,
    CBlockHeader,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than

IP_TO_ANNOUNCE = "42.42.42.42"
ONE_DAY = 60 * 60 * 24


class SelfAnnouncementReceiver(P2PInterface):
    self_announcements_received = 0
    addresses_received = 0
    addr_messages_received = 0

    expected = None
    addrv2_test = False

    def __init__(self, *, expected, addrv2):
        super().__init__(support_addrv2=addrv2)
        self.expected = expected
        self.addrv2_test = addrv2

    def handle_addr_message(self, message):
        self.addr_messages_received += 1
        for addr in message.addrs:
            self.addresses_received += 1
            if addr == self.expected:
                self.self_announcements_received += 1
            # The tricky part of this test is the timestamp of the self-announcement.
            # The following helped me to debug timestamp mismatches, so I'm leaving it in for the next one.
            elif addr.ip == self.expected.ip and addr.port == self.expected.port and addr.nServices == self.expected.nServices:
                print(f"Self-announcement timestamp mismatch: got={addr.time} expected={self.expected.time}")

    def on_addrv2(self, message):
        assert (self.addrv2_test)
        self.handle_addr_message(message)

    def on_addr(self, message):
        assert (not self.addrv2_test)
        self.handle_addr_message(message)


class AddrSelfAnnouncementTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f"-externalip={IP_TO_ANNOUNCE}"]]

    def run_test(self):
        # populate addrman to have some addresses for a GETADDR response
        for i in range(50):
            first_octet = 1 + (i >> 8)
            second_octet = i % 256
            a = f"{first_octet}.{second_octet}.1.1"
            self.nodes[0].addpeeraddress(a, 8333)

        self.self_announcement_inbound_test(addrv2=False)
        self.self_announcement_inbound_test(addrv2=True)
        self.self_announcement_outbound_test(addrv2=False)
        self.self_announcement_outbound_test(addrv2=True)

    def self_announcement_inbound_test(self, addrv2=False):
        addr_version = "addrv2" if addrv2 else "addrv1"
        self.log.info(f"Test that the node does an address self-announcement to inbound connections ({addr_version})")

        # We only self-announce after initial block download is done
        assert (not self.nodes[0].getblockchaininfo()["initialblockdownload"])

        netinfo = self.nodes[0].getnetworkinfo()
        port = netinfo["localaddresses"][0]["port"]
        self.nodes[0].setmocktime(int(time.time()))

        expected = CAddress()
        expected.nServices = int(netinfo["localservices"], 16)
        expected.ip = IP_TO_ANNOUNCE
        expected.port = port
        expected.time = self.nodes[0].mocktime

        self.log.info(f"Check that we get an initial self-announcement when connecting to a node and sending a GETADDR (inbound, {addr_version})")
        with self.nodes[0].assert_debug_log([f'Advertising address {IP_TO_ANNOUNCE}:{port}']):
            addr_receiver = self.nodes[0].add_p2p_connection(SelfAnnouncementReceiver(expected=expected, addrv2=addrv2))
            addr_receiver.sync_with_ping()

        # We expect one self-announcement and multiple other addresses in
        # response to a GETADDR in a single addr / addrv2 message.
        assert_equal(addr_receiver.self_announcements_received, 1)
        assert_equal(addr_receiver.addr_messages_received, 1)
        assert_greater_than(addr_receiver.addresses_received, 1)

        self.log.info(f"Check that we get more self-announcements sometime later (inbound, {addr_version})")
        for _ in range(5):
            last_self_announcements_received = addr_receiver.self_announcements_received
            last_addr_messages_received = addr_receiver.addr_messages_received
            last_addresses_received = addr_receiver.addresses_received
            with self.nodes[0].assert_debug_log([f'Advertising address {IP_TO_ANNOUNCE}:{port}']):
                # m_next_local_addr_send and AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL:
                # self-announcements are sent on an exponential distribution with mean interval of 24h.
                # Setting the mocktime 20d forward gives a probability of (1 - e^-(480/24)) that
                # the event will occur (i.e. this fails once in ~500 million repeats).
                self.nodes[0].bumpmocktime(20 * ONE_DAY)
                addr_receiver.expected.time = self.nodes[0].mocktime
                addr_receiver.sync_with_ping()

            assert_equal(addr_receiver.self_announcements_received, last_self_announcements_received + 1)
            assert_equal(addr_receiver.addr_messages_received, last_addr_messages_received + 1)
            assert_equal(addr_receiver.addresses_received, last_addresses_received + 1)

        self.nodes[0].disconnect_p2ps()

    def self_announcement_outbound_test(self, addrv2=True):
        addr_version = "addrv2" if addrv2 else "addrv1"
        self.log.info(f"Test that the node does an address self-announcement to outbound connections ({addr_version})")

        # We only self-announce after initial block download is done
        assert (not self.nodes[0].getblockchaininfo()["initialblockdownload"])

        netinfo = self.nodes[0].getnetworkinfo()
        port = netinfo["localaddresses"][0]["port"]
        self.nodes[0].setmocktime(int(time.time()))

        expected = CAddress()
        expected.nServices = int(netinfo["localservices"], 16)
        expected.ip = IP_TO_ANNOUNCE
        expected.port = port
        expected.time = self.nodes[0].mocktime

        self.log.info(f"Check that we get an initial self-announcement on a outbound connection from the node (outbound, {addr_version})")
        with self.nodes[0].assert_debug_log([f'Advertising address {IP_TO_ANNOUNCE}:{port}']):
            addr_receiver = self.nodes[0].add_outbound_p2p_connection(SelfAnnouncementReceiver(expected=expected, addrv2=addrv2), p2p_idx=0, connection_type="outbound-full-relay")
            addr_receiver.sync_with_ping()

        # We expect only the self-announcement.
        assert_equal(addr_receiver.self_announcements_received, 1)
        assert_equal(addr_receiver.addr_messages_received, 1)
        assert_equal(addr_receiver.addresses_received, 1)

        # to avoid the node evicting the outbound peer, protect it by announcing the most recent header to it
        tip_header = from_hex(CBlockHeader(), self.nodes[0].getblockheader(self.nodes[0].getbestblockhash(), False))
        addr_receiver.send_and_ping(msg_headers([tip_header]))

        self.log.info(f"Check that we get more self-announcements sometime later (outbound, {addr_version})")
        for _ in range(5):
            last_self_announcements_received = addr_receiver.self_announcements_received
            last_addr_messages_received = addr_receiver.addr_messages_received
            last_addresses_received = addr_receiver.addresses_received
            with self.nodes[0].assert_debug_log([f'Advertising address {IP_TO_ANNOUNCE}:{port}']):
                # m_next_local_addr_send and AVG_LOCAL_ADDRESS_BROADCAST_INTERVAL:
                # self-announcements are sent on an exponential distribution with mean interval of 24h.
                # Setting the mocktime 20d forward gives a probability of (1 - e^-(480/24)) that
                # the event will occur (i.e. this fails once in ~500 million repeats).
                self.nodes[0].bumpmocktime(20 * ONE_DAY)
                addr_receiver.expected.time = self.nodes[0].mocktime
                addr_receiver.sync_with_ping()

            assert_equal(addr_receiver.self_announcements_received, last_self_announcements_received + 1)
            assert_equal(addr_receiver.addr_messages_received, last_addr_messages_received + 1)
            assert_equal(addr_receiver.addresses_received, last_addresses_received + 1)

        self.nodes[0].disconnect_p2ps()


if __name__ == '__main__':
    AddrSelfAnnouncementTest(__file__).main()
