#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test starting bitcoind with -bind and/or -bind=...=onion and confirm
that bind happens on the expected ports.
"""

import socket

from test_framework.netutil import (
    addr_to_hex,
    get_bind_addrs,
)
from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.test_node import ErrorMatch
from test_framework.util import (
    REGTEST_ONION_TARGET_PORT,
    assert_equal,
    p2p_port,
    rpc_port,
)

WILDCARD_ONION_ERROR = "Error: The automatic Tor onion service cannot use a wildcard onion bind because the Tor daemon wouldn't be able to forward incoming connections to us. Use a specific address such as -bind=127.0.0.1:<port>=onion, or disable the service with -listenonion=0."


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

        # Start custom ports by reusing unused p2p ports. Take each one from p2p_port(),
        # which never hands out the fixed regtest onion target port.
        ports = iter(p2p_port(n) for n in range(self.num_nodes, self.num_nodes + 4))

        # Array of tuples [command line arguments, expected bind addresses].
        self.expected = []

        # Node0, no normal -bind=... with -bind=...=onion, thus only the tor target.
        onion = next(ports)
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{onion}=onion"],
                [(loopback_ipv4, onion)]
            ],
        )

        # Node1, both -bind=... and -bind=...=onion.
        port, onion = next(ports), next(ports)
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}", f"-bind=127.0.0.1:{onion}=onion"],
                [(loopback_ipv4, port), (loopback_ipv4, onion)]
            ],
        )

        # Node2, -bind=... with -listenonion=0 and no -bind=...=onion, thus no extra port for Tor target.
        port = next(ports)
        self.expected.append(
            [
                [f"-bind=127.0.0.1:{port}", "-listenonion=0"],
                [(loopback_ipv4, port)]
            ],
        )

        self.extra_args = list(map(lambda e: e[0], self.expected))
        self.setup_nodes()

    def check_binds(self, i, expected_services):
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

    def run_test(self):
        for i, (args, expected_services) in enumerate(self.expected):
            self.log.info(f"Checking listening ports of node {i} with {args}")
            self.check_binds(i, expected_services)

        self.log.info("Test -bind without an onion bind also listens on the default onion target with the default -listenonion")
        self.stop_node(2)
        self.nodes[2].replace_in_config([("listenonion=0\n", "")])
        bind_args = [self.extra_args[2][0], "-torcontrol=127.0.0.1:1"]
        self.start_node(2, extra_args=bind_args)
        self.check_binds(2, self.expected[2][1] + [(addr_to_hex("127.0.0.1"), REGTEST_ONION_TARGET_PORT)])
        self.stop_node(2)

        with socket.create_server(("127.0.0.1", REGTEST_ONION_TARGET_PORT)):
            self.log.info("Test -bind still starts when another process holds the default onion target")
            self.start_node(2, extra_args=bind_args)
            self.check_binds(2, self.expected[2][1])
            self.stop_node(2)

            self.log.info("Test -listenonion=1 requires the default onion target")
            self.nodes[2].assert_start_raises_init_error(
                bind_args + ["-listenonion=1"],
                f"Error: Unable to bind to 127.0.0.1:{REGTEST_ONION_TARGET_PORT} on this computer.",
                match=ErrorMatch.PARTIAL_REGEX,
            )

        self.log.info("Test wildcard onion bind with the default -listenonion")
        self.nodes[2].assert_start_raises_init_error([f"-bind=0.0.0.0:{p2p_port(2)}=onion"], WILDCARD_ONION_ERROR)

        self.log.info("Test -bind with dedicated onion bind starts when -listenonion=1")
        self.restart_node(1, extra_args=self.extra_args[1] + ["-listenonion=1", "-torcontrol=127.0.0.1:1"])

        self.log.info("Test wildcard onion bind with -listenonion=1")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            [f"-bind=0.0.0.0:{p2p_port(0)}=onion", "-listenonion=1", "-torcontrol=127.0.0.1:1"],
            WILDCARD_ONION_ERROR,
        )

        self.log.info("Test wildcard onion bind with -listenonion=0")
        self.start_node(0, extra_args=[f"-bind=0.0.0.0:{p2p_port(0)}=onion", "-listenonion=0"])
        self.stop_node(0)

if __name__ == '__main__':
    BindExtraTest().main()
