#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that -discover does not add all interfaces' addresses if we listen on only some of them
"""

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.test_node import (
    FailedToStartError,
)
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    p2p_port,
    tor_port,
)

# We need to bind to a routable address for this test to exercise the relevant code
# and also must have another routable address. Those addresses must be on an interface
# that is UP and is not a loopback interface (IFF_LOOPBACK).
# To set these routable addresses on the machine, use:
# Linux:
# First find your interfaces: ip addr show
# Then use your actual interface names (replace INTERFACE_NAME with yours):
# ip addr add 1.1.1.5/32 dev INTERFACE_NAME && ip addr add 1111:1111::5/128 dev INTERFACE_NAME  # to set up
# ip addr del 1.1.1.5/32 dev INTERFACE_NAME && ip addr del 1111:1111::5/128 dev INTERFACE_NAME  # to remove it
#
# FreeBSD and MacOS:
# ifconfig INTERFACE_NAME 1.1.1.5/32 alias && ifconfig INTERFACE_NAME 1111:1111::5/128 alias  # to set up
# ifconfig INTERFACE_NAME 1.1.1.5 -alias && ifconfig INTERFACE_NAME 1111:1111::5 -alias       # to remove it, after the test
ADDR1 = '1.1.1.5'
ADDR2 = '1111:1111::5'

class BindPortDiscoverTest(BitcoinTestFramework):
    def set_test_params(self):
        # Avoid any -bind= on the command line. Force the framework to avoid adding -bind=127.0.0.1.
        self.bind_to_localhost_only = False
        # Get dynamic ports for each node from the test framework
        self.bind_ports = [
            p2p_port(0),
            p2p_port(2), # node0 will use their port + 1 for onion listen, which is the same as p2p_port(1), so avoid collision
            p2p_port(3),
            p2p_port(4),
        ]
        self.extra_args = [
            ['-discover', f'-port={self.bind_ports[0]}', '-listen=1'], # Without any -bind
            ['-discover', f'-bind=0.0.0.0:{self.bind_ports[1]}'], # Explicit -bind=0.0.0.0
            # Explicit -whitebind=0.0.0.0, add onion bind to avoid port conflict
            ['-discover', f'-whitebind=0.0.0.0:{self.bind_ports[2]}', f'-bind=127.0.0.1:{tor_port(3)}=onion'],
            ['-discover', f'-bind={ADDR1}:{self.bind_ports[3]}'], # Explicit -bind=routable_addr
        ]
        self.num_nodes = len(self.extra_args)

    def setup_network(self):
        """
        Override to avoid connecting nodes together. This test intentionally does not connect nodes
        because each node is bound to a different address or interface, and connections are not needed.
        """
        self.setup_nodes()

    def setup_nodes(self):
        """
        Override to set has_explicit_bind=True for nodes with explicit bind arguments.
        """
        self.add_nodes(self.num_nodes, self.extra_args)
        # TestNode.start() will add -bind= to extra_args if has_explicit_bind is
        # False. We do not want any -bind= thus set has_explicit_bind to True.
        for node in self.nodes:
            node.has_explicit_bind = True

        try:
            self.start_nodes()
        except FailedToStartError as e:
            for node in self.nodes:
                if node.running:
                    if node.rpc_connected:
                        node.stop_node(wait=node.rpc_timeout)
                    else:
                        node.process.kill()
                        node.process.wait(timeout=node.rpc_timeout)
                        node.process = None
                        node.stdout.close()
                        node.stderr.close()
                        node.running = False
            if 'Unable to bind to ' in str(e):
                raise SkipTest(
                    f'To run this test make sure that {ADDR1} and {ADDR2} '
                    '(routable addresses) are assigned to non-loopback '
                    'interfaces on this machine')
            else:
                raise e

    def run_test(self):
        self.log.info(
                "Test that if -bind= is not passed or -bind=0.0.0.0 is used then all addresses are "
                "added to localaddresses")
        for i in [0, 1, 2]:
            found_addr1 = False
            found_addr2 = False
            localaddresses = self.nodes[i].getnetworkinfo()['localaddresses']
            for local in localaddresses:
                if local['address'] == ADDR1:
                    found_addr1 = True
                    assert_equal(local['port'], self.bind_ports[i])
                if local['address'] == ADDR2:
                    found_addr2 = True
                    assert_equal(local['port'], self.bind_ports[i])
            if not found_addr1:
                self.log.error(f"Address {ADDR1} not found in node{i}'s local addresses: {localaddresses}")
                assert False
            if not found_addr2:
                self.log.error(f"Address {ADDR2} not found in node{i}'s local addresses: {localaddresses}")
                assert False

        self.log.info(
                "Test that if -bind=routable_addr is passed then only that address is "
                "added to localaddresses")
        found_addr1 = False
        i = 3
        for local in self.nodes[i].getnetworkinfo()['localaddresses']:
            if local['address'] == ADDR1:
                found_addr1 = True
                assert_equal(local['port'], self.bind_ports[i])
            assert_not_equal(local['address'], ADDR2)
        assert found_addr1

if __name__ == '__main__':
    BindPortDiscoverTest(__file__).main()
