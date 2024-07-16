#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test starting bitcoind with -bind and/or -bind=...=onion and confirm
that bind happens on the expected ports.
"""

from test_framework.netutil import (
    addr_to_hex,
    get_bind_addrs,
)
from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.util import (
    assert_equal,
    p2p_port,
    rpc_port,
)


class BindExtraTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        # Avoid any -bind= on the command line. Force the framework to avoid
        # adding -bind=127.0.0.1.
        self.bind_to_localhost_only = False
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        # Due to OS-specific network stats queries, we only run on Linux.
        self.skip_if_platform_not_linux()

    def setup_network(self):
        loopback_ipv4 = addr_to_hex("127.0.0.1")

        # Start custom ports by reusing unused p2p ports
        port = p2p_port(self.num_nodes)

        # Array of tuples [command line arguments, expected bind addresses].
        self.expected = []

        # Node0, no normal -bind=... with -bind=...=onion, thus only the tor target.
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}=onion"],
                [(loopback_ipv4, port)]
            ],
        )
        port += 1

        # Node1, both -bind=... and -bind=...=onion.
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}", f"-bind=127.0.0.1:{port + 1}=onion"],
                [(loopback_ipv4, port), (loopback_ipv4, port + 1)]
            ],
        )
        port += 2

        # Node2, no -bind=...=onion, thus no extra port for Tor target.
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}"],
                [(loopback_ipv4, port)]
            ],
        )
        port += 1

        self.extra_args = list(map(lambda e: e[0], self.expected))
        self.setup_nodes()

    def run_test(self):
        for i, (args, expected_services) in enumerate(self.expected):
            self.log.info(f"Checking listening ports of node {i} with {args}")
            pid = self.nodes[i].process.pid
            binds = set(get_bind_addrs(pid))
            # Remove IPv6 addresses because on some CI environments "::1" is not configured
            # on the system (so our test_ipv6_local() would return False), but it is
            # possible to bind on "::". This makes it unpredictable whether to expect
            # that bitcoind has bound on "::1" (for RPC) and "::" (for P2P).
            ipv6_addr_len_bytes = 32
            binds = set(filter(lambda e: len(e[0]) != ipv6_addr_len_bytes, binds))
            # Remove RPC ports. They are not relevant for this test.
            binds = set(filter(lambda e: e[1] != rpc_port(i), binds))
            assert_equal(binds, set(expected_services))

if __name__ == '__main__':
    BindExtraTest(__file__).main()
