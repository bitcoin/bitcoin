#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that CJDNS addresses are advertised when -externalip is set.

When -externalip is specified, -discover is implicitly set to 0. This
should not prevent CJDNS bound addresses from being added to
localaddresses, since CJDNS operates on a separate overlay network.
"""

import subprocess

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import p2p_port


DOCKER_NET_NAME = 'bitcoin_cjdns_test'
DOCKER_NET_SUBNET = 'fc00:dead:beef::/112'
CJDNS_ADDR = 'fc00:dead:beef::1'


def docker_is_available():
    """Return True if Docker is usable on this host."""
    try:
        subprocess.run(['docker', 'info'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def create_cjdns_interface():
    """Create a docker network with an fc00::/8 subnet.

    This places an fc00:: address on a non-loopback bridge interface on
    the host, which bitcoind can bind to.  Any stale network left over
    from a previously killed test run is removed first.
    """
    remove_cjdns_interface()
    subprocess.run(
        ['docker', 'network', 'create', '--ipv6',
         '--subnet', DOCKER_NET_SUBNET, DOCKER_NET_NAME],
        capture_output=True, check=True)


def remove_cjdns_interface():
    subprocess.run(
        ['docker', 'network', 'rm', DOCKER_NET_NAME],
        capture_output=True, check=False)


class CJDNSDiscoverWithExternalIPTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        if not docker_is_available():
            raise SkipTest(
                "Docker must be available to run this test.")

    def shutdown(self):
        remove_cjdns_interface()
        super().shutdown()

    def get_local_cjdns_addrs(self):
        """Return CJDNS entries from getnetworkinfo localaddresses."""
        local_addrs = self.nodes[0].getnetworkinfo()['localaddresses']
        return [a for a in local_addrs if a['address'] == CJDNS_ADDR]

    def run_test(self):
        try:
            create_cjdns_interface()
        except subprocess.CalledProcessError:
            raise SkipTest(
                "Could not create a Docker IPv6 network (unsupported on this platform).")

        port = p2p_port(0)
        bind_arg = f'-bind=[{CJDNS_ADDR}]:{port}'

        self.log.info("Test: CJDNS bound address is advertised with -discover (baseline)")
        self.restart_node(0, [bind_arg, '-discover', '-cjdnsreachable'])
        assert self.get_local_cjdns_addrs(), (
            f"Baseline failed: {CJDNS_ADDR} not in localaddresses")

        self.log.info("Test: CJDNS bound address is still advertised with -externalip")
        self.restart_node(0, [bind_arg, '-externalip=2.2.2.2', '-cjdnsreachable'])
        local_addrs = self.nodes[0].getnetworkinfo()['localaddresses']
        ext_addrs = [a for a in local_addrs if a['address'] == '2.2.2.2']
        assert ext_addrs, "externalip address 2.2.2.2 not in localaddresses"
        assert self.get_local_cjdns_addrs(), (
            f"{CJDNS_ADDR} missing from localaddresses when -externalip is set")


if __name__ == '__main__':
    CJDNSDiscoverWithExternalIPTest(__file__).main()
