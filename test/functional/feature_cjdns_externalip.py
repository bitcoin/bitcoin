#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that CJDNS addresses are advertised when -externalip is set.

When -externalip is specified, -discover is implicitly set to 0. This
should not prevent CJDNS bound addresses from being added to
localaddresses, since CJDNS operates on a separate overlay network.
"""

import ipaddress
import subprocess
import uuid

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import assert_equal, p2p_port


DOCKER_NET_LABEL = 'org.bitcoincore.functional-test'
DOCKER_NET_LABEL_VALUE = 'feature_cjdns_externalip'


def docker_is_available():
    """Return True if Docker is usable on this host."""
    try:
        subprocess.run(['docker', 'info'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def create_cjdns_interface(network_name, subnet, gateway):
    """Create a Docker network with an fc00::/8 subnet.

    This places an fc00:: address on a non-loopback bridge interface on
    the host, which bitcoind can bind to.
    """
    subprocess.run(
        ['docker', 'network', 'create', '--ipv6',
         '--subnet', subnet, '--gateway', gateway,
         '--label', f'{DOCKER_NET_LABEL}={DOCKER_NET_LABEL_VALUE}',
         network_name],
        capture_output=True, check=True)


def remove_cjdns_interface(network_name):
    if network_name is None:
        return

    try:
        inspect_result = subprocess.run(
            ['docker', 'network', 'inspect', '--format',
             f'{{{{ index .Labels "{DOCKER_NET_LABEL}" }}}}', network_name],
            capture_output=True, check=False, text=True)
    except FileNotFoundError:
        return

    if inspect_result.returncode != 0:
        return
    if inspect_result.stdout.strip() != DOCKER_NET_LABEL_VALUE:
        return

    subprocess.run(
        ['docker', 'network', 'rm', network_name],
        capture_output=True, check=False)


class CJDNSDiscoverWithExternalIPTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 1
        unique_id = uuid.uuid4().hex
        cjdns_prefix = f'fc00:{int(unique_id[:4], 16):x}:{int(unique_id[4:8], 16):x}::'
        self.docker_net_name = f'bitcoin_cjdns_test_{unique_id[:16]}'
        self.docker_net_subnet = f'{ipaddress.IPv6Address(cjdns_prefix)}/112'
        self.cjdns_addr = str(ipaddress.IPv6Address(f'{cjdns_prefix}1'))

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        if not docker_is_available():
            raise SkipTest(
                "Docker must be available to run this test.")

    def shutdown(self):
        try:
            return super().shutdown()
        finally:
            remove_cjdns_interface(self.docker_net_name)

    def get_local_cjdns_addrs(self):
        """Return CJDNS entries from getnetworkinfo localaddresses."""
        local_addrs = self.nodes[0].getnetworkinfo()['localaddresses']
        return [a for a in local_addrs if a['address'] == self.cjdns_addr]

    def run_test(self):
        try:
            create_cjdns_interface(self.docker_net_name, self.docker_net_subnet, self.cjdns_addr)
        except subprocess.CalledProcessError:
            raise SkipTest(
                "Could not create a Docker IPv6 network (unsupported on this platform).")

        # The test framework disables discovery by default in bitcoin.conf.
        # Remove that default so -externalip can soft-disable discovery here.
        self.nodes[0].replace_in_config([("discover=0\n", "")])

        port = p2p_port(0)
        bind_arg = f'-bind=[{self.cjdns_addr}]:{port}'

        self.log.info("Test: CJDNS address is not advertised with explicit -discover=0")
        self.restart_node(0, ['-discover=0', '-cjdnsreachable'])
        assert_equal(self.get_local_cjdns_addrs(), [])

        self.log.info("Test: CJDNS bound address is not advertised with explicit -discover=0")
        self.restart_node(0, [bind_arg, '-discover=0', '-cjdnsreachable'])
        assert_equal(self.get_local_cjdns_addrs(), [])

        self.log.info("Test: CJDNS bound address is advertised with -discover (baseline)")
        self.restart_node(0, [bind_arg, '-discover', '-cjdnsreachable'])
        assert self.get_local_cjdns_addrs(), (
            f"Baseline failed: {self.cjdns_addr} not in localaddresses")

        self.log.info("Test: CJDNS bound address is still advertised with -externalip")
        self.restart_node(0, [bind_arg, '-externalip=2.2.2.2', '-cjdnsreachable'])
        local_addrs = self.nodes[0].getnetworkinfo()['localaddresses']
        ext_addrs = [a for a in local_addrs if a['address'] == '2.2.2.2']
        assert ext_addrs, "externalip address 2.2.2.2 not in localaddresses"
        assert self.get_local_cjdns_addrs(), (
            f"{self.cjdns_addr} missing from localaddresses when -externalip is set")


if __name__ == '__main__':
    CJDNSDiscoverWithExternalIPTest(__file__).main()
