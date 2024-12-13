#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that -discover does not add all interfaces' addresses if we listen on only some of them
"""

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal

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

BIND_PORT = 31001

class BindPortDiscoverTest(BitcoinTestFramework):
    def set_test_params(self):
        # Avoid any -bind= on the command line. Force the framework to avoid adding -bind=127.0.0.1.
        self.bind_to_localhost_only = False
        self.extra_args = [
            ['-discover', f'-port={BIND_PORT}'], # bind on any
            ['-discover', f'-bind={ADDR1}:{BIND_PORT}'],
            ['-discover', f'-bind=0.0.0.0:{BIND_PORT+1}'],
            ['-discover', f'-bind=[::]:{BIND_PORT+2}'],
            ['-discover', f'-bind=::', f'-port={BIND_PORT+3}'],
            ['-discover', f'-bind=0.0.0.0:{BIND_PORT+4}=onion'],
        ]
        self.num_nodes = len(self.extra_args)

    def setup_network(self):
        """
        BitcoinTestFramework.setup_network() without connecting node0 to node1,
        we don't need that and avoiding it makes the test faster.
        """
        self.setup_nodes()

    def setup_nodes(self):
        """
        BitcoinTestFramework.setup_nodes() with overriden has_explicit_bind=True
        for each node and without the chain setup.
        """
        self.add_nodes(self.num_nodes, self.extra_args)
        # TestNode.start() will add -bind= to extra_args if has_explicit_bind is
        # False. We do not want any -bind= thus set has_explicit_bind to True.
        for node in self.nodes:
            node.has_explicit_bind = True
        self.start_nodes()

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

    def verify_addrs(self, found_addrs, expected_addr1, expected_addr2, port):
        print(found_addrs, port)
        found_addr1 = False
        found_addr2 = False
        for local in found_addrs:
            if local['address'] == ADDR1:
                found_addr1 = True
                assert_equal(local['port'], port)
            if local['address'] == ADDR2:
                found_addr2 = True
                assert_equal(local['port'], port)
        assert found_addr1 == expected_addr1
        assert found_addr2 == expected_addr2

    def run_test(self):
        self.log.info(
                "Test that if -bind= is not passed then all addresses are "
                "added to localaddresses")
        self.verify_addrs(self.nodes[0].getnetworkinfo()['localaddresses'], True, True, BIND_PORT)

        self.log.info(
                "Test that if -bind= is passed then only that address is "
                "added to localaddresses")
        self.verify_addrs(self.nodes[1].getnetworkinfo()['localaddresses'], True, False, BIND_PORT)

        self.log.info(
                "Test that if -bind=0.0.0.0:port is passed then all addresses are "
                "added to localaddresses")
        self.verify_addrs(self.nodes[2].getnetworkinfo()['localaddresses'], True, True, BIND_PORT+1)

        self.log.info(
                "Test that if -bind=[::]:port is passed then all addresses are "
                "added to localaddresses")
        self.verify_addrs(self.nodes[3].getnetworkinfo()['localaddresses'], True, True, BIND_PORT+2)

        self.log.info(
                "Test that if -bind=:: is passed then all addresses are "
                "added to localaddresses")
        self.verify_addrs(self.nodes[4].getnetworkinfo()['localaddresses'], True, True, BIND_PORT+3)

        self.log.info(
                "Test that if -bind=0.0.0.0:port=onion is passed then all addresses are "
                "added to localaddresses")
        self.verify_addrs(self.nodes[5].getnetworkinfo()['localaddresses'], True, True, BIND_PORT+4)

if __name__ == '__main__':
    BindPortDiscoverTest(__file__).main()
