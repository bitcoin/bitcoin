#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that -outboundbind sets the source address of outgoing connections.
"""
import re

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.test_node import (
    ErrorMatch,
    FailedToStartError,
)
from test_framework.util import append_config, assert_equal, p2p_port

# We need to bind to routable addresses for this test to exercise the relevant
# code. Both addresses are set in the CI environment, see
# ci/test/02_run_container.py.
#
# To set these routable addresses on the machine, use:
# Linux:
# ip addr add 1.1.1.5/32 dev INTERFACE_NAME && ip addr add 1111:1111::5/128 dev INTERFACE_NAME  # to set up
# ip addr del 1.1.1.5/32 dev INTERFACE_NAME && ip addr del 1111:1111::5/128 dev INTERFACE_NAME  # to remove it
# FreeBSD or macOS:
# ifconfig INTERFACE_NAME 1.1.1.5/32 alias && ifconfig INTERFACE_NAME inet6 1111:1111::5/128 alias  # to set up
# ifconfig INTERFACE_NAME 1.1.1.5 -alias && ifconfig INTERFACE_NAME inet6 1111:1111::5 -alias  # to remove it, after the test
ADDR4 = '1.1.1.5' # This and the address below are set in the CI environment, don't change it just here (keep them in sync).
ADDR6 = '1111:1111::5'
# TEST-NET-1 documentation range, not assigned to an interface, so binding to it fails.
UNASSIGNED_ADDR = '192.0.2.99'
# RFC3849 documentation range; parses but is not assignable in practice.
DOC_ADDR6 = '2001:db8::1'


class BindOutgoingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 2
        self.port4 = p2p_port(0)
        # Not p2p_port(1): that is node 1's own framework-assigned listen port.
        self.port6 = p2p_port(2)

    def setup_network(self):
        # Node 0 listens on loopback in both families; node 1 binds a routable
        # source in each family. Because the bound source differs from the
        # loopback destination, observing it at node 0 proves the bind took
        # effect (the OS would otherwise select the loopback source).
        self.extra_args = [
            [f'-bind=127.0.0.1:{self.port4}', f'-bind=[::1]:{self.port6}'],
            [f'-outboundbind={ADDR4}', f'-outboundbind={ADDR6}'],
        ]
        try:
            self.setup_nodes()
        except FailedToStartError as e:
            self.cleanup_partially_started_nodes()
            # Match only the -outboundbind trial-bind failure (bare address, no
            # port); a listen bind failure reports "addr:port" and must not skip.
            if (f'Unable to bind to {ADDR4} ' in str(e)
                    or f'Unable to bind to {ADDR6} ' in str(e)):
                raise SkipTest(
                    f'To run this test make sure that {ADDR4} and {ADDR6} '
                    '(routable addresses) are assigned to non-loopback '
                    'interfaces on this machine')
            raise

    def check_source(self, target, expected_src):
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 0)
        self.nodes[1].addnode(target, 'onetry')
        self.wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 1)

        # The bound source differs from the (loopback) destination, so the source
        # observed here proves -outboundbind took effect end to end.
        peer_of_node0 = self.nodes[0].getpeerinfo()[0]
        assert_equal(peer_of_node0['inbound'], True)
        assert_equal(peer_of_node0['addr'].rsplit(':', 1)[0].strip('[]'), expected_src)

        peer_of_node1 = self.nodes[1].getpeerinfo()[0]
        assert_equal(peer_of_node1['inbound'], False)
        assert_equal(peer_of_node1['addrbind'].rsplit(':', 1)[0].strip('[]'), expected_src)
        self.nodes[1].disconnectnode(address=target)

    def run_test(self):
        self.log.info(f'Check that an IPv4 outgoing connection originates from {ADDR4}')
        self.check_source(f'127.0.0.1:{self.port4}', ADDR4)

        self.log.info(f'Check that an IPv6 outgoing connection originates from {ADDR6}')
        self.check_source(f'[::1]:{self.port6}', ADDR6)

        self.log.info(f'Check that -outboundbind={UNASSIGNED_ADDR} is rejected at startup')
        self.stop_node(1)
        self.nodes[1].assert_start_raises_init_error(
            extra_args=[f'-outboundbind={UNASSIGNED_ADDR}'],
            expected_msg=f'Error: Unable to bind to {UNASSIGNED_ADDR}',
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info('Check that an -outboundbind address with a port is rejected at startup')
        self.nodes[1].assert_start_raises_init_error(
            extra_args=[f'-outboundbind={ADDR4}:1234'],
            expected_msg='Error: Cannot resolve -outboundbind address',
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info('Check that the unspecified (wildcard) address is rejected at startup')
        for wildcard in ('0.0.0.0', '::'):
            self.nodes[1].assert_start_raises_init_error(
                extra_args=[f'-outboundbind={wildcard}'],
                expected_msg=f"Error: -outboundbind address '{wildcard}' is not a usable source address",
                match=ErrorMatch.PARTIAL_REGEX,
            )

        self.log.info(f'Check that -outboundbind={DOC_ADDR6} alone fails the trial bind at startup')
        self.nodes[1].assert_start_raises_init_error(
            extra_args=[f'-outboundbind={DOC_ADDR6}'],
            expected_msg=f'Error: Unable to bind to {DOC_ADDR6}',
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info('Check that a non-clearnet (Tor) address is rejected at startup')
        self.nodes[1].assert_start_raises_init_error(
            extra_args=['-outboundbind=pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion'],
            expected_msg='is not an IPv4 or IPv6 address',
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info('Check that an fc00::/8 address is rejected at startup when it '
                      'is a CJDNS address (-cjdnsreachable)')
        self.nodes[1].assert_start_raises_init_error(
            extra_args=['-cjdnsreachable', '-outboundbind=fc00:db8::1'],
            expected_msg='is a CJDNS address',
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info('Check that fc00::/8 stays a plain IPv6 source without -cjdnsreachable')
        # Accepted by the parser (proven by reaching the later trial bind, which
        # fails because the address is not assigned to this machine).
        self.nodes[1].assert_start_raises_init_error(
            extra_args=['-outboundbind=fc00:db8::1'],
            expected_msg='Unable to bind to fc00:db8::1',
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info('Check that the proxy warning is family-specific: an IPv6 bind '
                      'with an IPv6-only proxy warns at startup')
        self.start_node(1, extra_args=[f'-outboundbind={ADDR6}', '-proxy=127.0.0.1:9=ipv6'])
        self.stop_node(1, expected_stderr=re.compile(
            r"Option '-outboundbind' is set but a proxy is configured"))

        self.log.info('Check that no warning is issued when the proxy serves only '
                      'the other family (IPv4 bind, IPv6-only proxy)')
        self.start_node(1, extra_args=[f'-outboundbind={ADDR4}', '-proxy=127.0.0.1:9=ipv6'])
        self.stop_node(1)  # the default asserts empty stderr: no warning

        self.log.info('Check that an onion-only proxy does not trigger the warning')
        self.start_node(1, extra_args=[f'-outboundbind={ADDR4}', '-onion=127.0.0.1:9'])
        self.stop_node(1)

        self.log.info('Check that a command-line -outboundbind overrides a config-file '
                      'one of the same family, even when the config-file address cannot '
                      'be bound to (only the effective address is validated)')
        append_config(self.nodes[1].datadir_path, [f'outboundbind={UNASSIGNED_ADDR}'])
        self.restart_node(1, [f'-outboundbind={ADDR4}'])
        self.check_source(f'127.0.0.1:{self.port4}', ADDR4)

        self.log.info('Check that a configured proxy makes -outboundbind inert, '
                      'with a startup warning')
        self.stop_node(1)
        self.start_node(1, extra_args=[f'-outboundbind={ADDR4}', '-proxy=127.0.0.1:9'])
        self.stop_node(1, expected_stderr=re.compile(
            r"Option '-outboundbind' is set but a proxy is configured"))

        # Last: this leaves a second, IPv6-family value in the config file,
        # which later starts would have to override.
        self.log.info('Check that a command-line -outboundbind also overrides a config-file '
                      'v6 documentation address')
        append_config(self.nodes[1].datadir_path, [f'outboundbind={DOC_ADDR6}'])
        self.restart_node(1, [f'-outboundbind={ADDR4}', f'-outboundbind={ADDR6}'])
        self.check_source(f'[::1]:{self.port6}', ADDR6)


if __name__ == '__main__':
    BindOutgoingTest(__file__).main()
