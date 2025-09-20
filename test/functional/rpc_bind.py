#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running bitcoind with the -rpcbind and -rpcallowip options."""

from test_framework.netutil import all_interfaces, addr_to_hex, get_bind_addrs, test_ipv6_local
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_equal, assert_raises_rpc_error, get_rpc_proxy, rpc_port, rpc_url

class RPCBindTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 1
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        # due to OS-specific network stats queries, this test works only on Linux
        self.skip_if_platform_not_linux()

    def setup_network(self):
        self.add_nodes(self.num_nodes, None)

    def add_options(self, parser):
        parser.add_argument("--ipv4", action='store_true', dest="run_ipv4", help="Run ipv4 tests only", default=False)
        parser.add_argument("--ipv6", action='store_true', dest="run_ipv6", help="Run ipv6 tests only", default=False)
        parser.add_argument("--nonloopback", action='store_true', dest="run_nonloopback", help="Run non-loopback tests only", default=False)

    def run_bind_test(self, allow_ips, connect_to, addresses, expected):
        '''
        Start a node with requested rpcallowip and rpcbind parameters,
        then try to connect, and check if the set of bound addresses
        matches the expected set.
        '''
        self.log.info("Bind test for %s" % str(addresses))
        expected = [(addr_to_hex(addr), port) for (addr, port) in expected]
        base_args = ['-disablewallet', '-nolisten']
        if allow_ips:
            base_args += ['-rpcallowip=' + x for x in allow_ips]
        binds = ['-rpcbind='+addr for addr in addresses]
        self.nodes[0].rpchost = connect_to
        self.start_node(0, base_args + binds)
        pid = self.nodes[0].process.pid
        assert_equal(set(get_bind_addrs(pid)), set(expected))
        self.stop_nodes()

    def run_invalid_bind_test(self, allow_ips, addresses):
        '''
        Attempt to start a node with requested rpcallowip and rpcbind
        parameters, expecting that the node will fail.
        '''
        self.log.info(f'Invalid bind test for {addresses}')
        base_args = ['-disablewallet', '-nolisten']
        if allow_ips:
            base_args += ['-rpcallowip=' + x for x in allow_ips]
        init_error = 'Error: Invalid port specified in -rpcbind: '
        for addr in addresses:
            self.nodes[0].assert_start_raises_init_error(base_args + [f'-rpcbind={addr}'], init_error + f"'{addr}'")

    def run_allowip_test(self, allow_ips, rpchost, rpcport):
        '''
        Start a node with rpcallow IP, and request getnetworkinfo
        at a non-localhost IP.
        '''
        self.log.info("Allow IP test for %s:%d" % (rpchost, rpcport))
        node_args = \
            ['-disablewallet', '-nolisten'] + \
            ['-rpcallowip='+x for x in allow_ips] + \
            ['-rpcbind='+addr for addr in ['127.0.0.1', "%s:%d" % (rpchost, rpcport)]] # Bind to localhost as well so start_nodes doesn't hang
        self.nodes[0].rpchost = None
        self.start_nodes([node_args])
        # connect to node through non-loopback interface
        node = get_rpc_proxy(rpc_url(self.nodes[0].datadir_path, 0, self.chain, "%s:%d" % (rpchost, rpcport)), 0, coveragedir=self.options.coveragedir)
        node.getnetworkinfo()
        self.stop_nodes()

    def run_invalid_allowip_test(self):
        '''
        Check parameter interaction with -rpcallowip and -cjdnsreachable.
        RFC4193 addresses are fc00::/7 like CJDNS but have an optional
        "local" L bit making them fd00:: which should always be OK.
        '''
        self.log.info("Allow RFC4193 when compatible with CJDNS options")
        # Don't rpcallow RFC4193 with L-bit=0 if CJDNS is enabled
        self.nodes[0].assert_start_raises_init_error(
            ["-rpcallowip=fc00:db8:c0:ff:ee::/80","-cjdnsreachable"],
            "Invalid -rpcallowip subnet specification",
            match=ErrorMatch.PARTIAL_REGEX)
        # OK to rpcallow RFC4193 with L-bit=1 if CJDNS is enabled
        self.start_node(0, ["-rpcallowip=fd00:db8:c0:ff:ee::/80","-cjdnsreachable"])
        self.stop_nodes()
        # OK to rpcallow RFC4193 with L-bit=0 if CJDNS is not enabled
        self.start_node(0, ["-rpcallowip=fc00:db8:c0:ff:ee::/80"])
        self.stop_nodes()

    def run_test(self):
        if sum([self.options.run_ipv4, self.options.run_ipv6, self.options.run_nonloopback]) > 1:
            raise AssertionError("Only one of --ipv4, --ipv6 and --nonloopback can be set")

        self.log.info("Check for ipv6")
        have_ipv6 = test_ipv6_local()
        if not have_ipv6 and not (self.options.run_ipv4 or self.options.run_nonloopback):
            raise SkipTest("This test requires ipv6 support.")

        self.log.info("Check for non-loopback interface")
        self.non_loopback_ip = None
        for name,ip in all_interfaces():
            if ip != '127.0.0.1':
                self.non_loopback_ip = ip
                break
        if self.non_loopback_ip is None and self.options.run_nonloopback:
            raise SkipTest("This test requires a non-loopback ip address.")

        self.defaultport = rpc_port(0)

        if not self.options.run_nonloopback:
            self._run_loopback_tests()
            if self.options.run_ipv4:
                self.run_invalid_bind_test(['127.0.0.1'], ['127.0.0.1:notaport', '127.0.0.1:-18443', '127.0.0.1:0', '127.0.0.1:65536'])
            if self.options.run_ipv6:
                self.run_invalid_bind_test(['[::1]'], ['[::1]:notaport', '[::1]:-18443', '[::1]:0', '[::1]:65536'])
                self.run_invalid_allowip_test()
        if not self.options.run_ipv4 and not self.options.run_ipv6:
            if self.non_loopback_ip:
                self._run_nonloopback_tests()
            else:
                self.log.info('Non-loopback IP address not found, skipping non-loopback tests')

    def _run_loopback_tests(self):
        if self.options.run_ipv4:
            # check only IPv4 localhost (explicit)
            self.run_bind_test(['127.0.0.1'], '127.0.0.1', ['127.0.0.1'],
                [('127.0.0.1', self.defaultport)])
            # check only IPv4 localhost (explicit) with alternative port
            self.run_bind_test(['127.0.0.1'], '127.0.0.1:32171', ['127.0.0.1:32171'],
                [('127.0.0.1', 32171)])
            # check only IPv4 localhost (explicit) with multiple alternative ports on same host
            self.run_bind_test(['127.0.0.1'], '127.0.0.1:32171', ['127.0.0.1:32171', '127.0.0.1:32172'],
                [('127.0.0.1', 32171), ('127.0.0.1', 32172)])
        else:
            # check default without rpcallowip (IPv4 and IPv6 localhost)
            self.run_bind_test(None, '127.0.0.1', [],
                [('127.0.0.1', self.defaultport), ('::1', self.defaultport)])
            # check default with rpcallowip (IPv4 and IPv6 localhost)
            self.run_bind_test(['127.0.0.1'], '127.0.0.1', [],
                [('127.0.0.1', self.defaultport), ('::1', self.defaultport)])
            # check only IPv6 localhost (explicit)
            self.run_bind_test(['[::1]'], '[::1]', ['[::1]'],
                [('::1', self.defaultport)])
            # check both IPv4 and IPv6 localhost (explicit)
            self.run_bind_test(['127.0.0.1'], '127.0.0.1', ['127.0.0.1', '[::1]'],
                [('127.0.0.1', self.defaultport), ('::1', self.defaultport)])

    def _run_nonloopback_tests(self):
        self.log.info("Using interface %s for testing" % self.non_loopback_ip)

        # check only non-loopback interface
        self.run_bind_test([self.non_loopback_ip], self.non_loopback_ip, [self.non_loopback_ip],
            [(self.non_loopback_ip, self.defaultport)])

        # Check that with invalid rpcallowip, we are denied
        self.run_allowip_test([self.non_loopback_ip], self.non_loopback_ip, self.defaultport)
        assert_raises_rpc_error(-342, "non-JSON HTTP response with '403 Forbidden' from server", self.run_allowip_test, ['1.1.1.1'], self.non_loopback_ip, self.defaultport)

if __name__ == '__main__':
    RPCBindTest(__file__).main()
