#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that -discover does not add all interfaces' addresses if we listen on only some of them
"""

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    p2p_port,
)

# We need to bind to a routable address for this test to exercise the relevant code
# and also must have another routable address on another interface which must not
# be named "lo" or "lo0".
# To set these routable addresses on the machine, use:
# Linux:
# ifconfig lo:0 1.1.1.1/32 up && ifconfig lo:1 2.2.2.2/32 up  # to set up
# ifconfig lo:0 down && ifconfig lo:1 down  # to remove it, after the test
# FreeBSD:
# ifconfig em0 1.1.1.1/32 alias && ifconfig wlan0 2.2.2.2/32 alias  # to set up
# ifconfig em0 1.1.1.1 -alias && ifconfig wlan0 2.2.2.2 -alias  # to remove it, after the test
ADDR1 = '1.1.1.1'
ADDR2 = '2.2.2.2'

class BindPortDiscoverTest(BitcoinTestFramework):
    def set_test_params(self):
        # Avoid any -bind= on the command line. Force the framework to avoid adding -bind=127.0.0.1.
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        # Get dynamic ports for each node from the test framework
        self.bind_port_0 = p2p_port(0)
        self.bind_port_1 = p2p_port(1)
        self.extra_args = [
            ['-discover', f'-port={self.bind_port_0}'], # bind on any
            ['-discover', f'-bind={ADDR1}:{self.bind_port_1}'],
        ]
        self.num_nodes = len(self.extra_args)

    def add_options(self, parser):
        parser.add_argument(
            "--ihave1111and2222", action='store_true', dest="ihave1111and2222",
            help=f"Run the test, assuming {ADDR1} and {ADDR2} are configured on the machine",
            default=False)

    def skip_test_if_missing_module(self):
        if not self.options.ihave1111and2222:
            raise SkipTest(
                f"To run this test make sure that {ADDR1} and {ADDR2} (routable addresses) are "
                "assigned to the interfaces on this machine and rerun with --ihave1111and2222")

    def run_test(self):
        self.log.info(
                "Test that if -bind= is not passed then all addresses are "
                "added to localaddresses")
        found_addr1 = False
        found_addr2 = False
        for local in self.nodes[0].getnetworkinfo()['localaddresses']:
            if local['address'] == ADDR1:
                found_addr1 = True
                assert_equal(local['port'], self.bind_port_0)
            if local['address'] == ADDR2:
                found_addr2 = True
                assert_equal(local['port'], self.bind_port_0)
        assert found_addr1
        assert found_addr2

        self.log.info(
                "Test that if -bind= is passed then only that address is "
                "added to localaddresses")
        found_addr1 = False
        for local in self.nodes[1].getnetworkinfo()['localaddresses']:
            if local['address'] == ADDR1:
                found_addr1 = True
                assert_equal(local['port'], self.bind_port_1)
            assert_not_equal(local['address'], ADDR2)
        assert found_addr1

if __name__ == '__main__':
    BindPortDiscoverTest(__file__).main()
