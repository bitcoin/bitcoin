#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running bitcoind with the -rpcbind and -rpcallowip options."""

import socket
import subprocess
import time

from test_framework.netutil import NETWORK_ERRORS, all_interfaces, addr_to_hex, get_bind_addrs, test_ipv6_local
from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.test_node import ErrorMatch, FailedToStartError
from test_framework.util import assert_equal, get_auth_cookie, rpc_port, str_to_b64str

class RPCBindTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_posix()
        self.skip_if_no_lsof_on_nonlinux()

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

    def run_partial_bind_test(self, explicit_binds):
        self.log.info(f'Check startup when the {"explicit" if explicit_binds else "default IPv4"} RPC address is already owned')
        self.nodes[0].rpchost = '[::1]'
        error = ''
        credential_captured = False
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            sock.bind(('127.0.0.1', self.defaultport))
            sock.listen()
            sock.settimeout(0.1)
            try:
                extra_args = ['-disablewallet', '-nolisten']
                if explicit_binds:
                    extra_args += ['-rpcallowip=127.0.0.1', '-rpcbind=127.0.0.1', '-rpcbind=[::1]']
                self.start_node(0, extra_args=extra_args)
                started = True
                cli = subprocess.Popen([
                    self.nodes[0].binaries.paths.bitcoincli,
                    f'-datadir={self.nodes[0].datadir_path}',
                    '-noipcconnect',
                    '-rpcclienttimeout=2',
                    'getblockcount',
                ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                try:
                    conn = None
                    deadline = time.monotonic() + 5
                    while conn is None and time.monotonic() < deadline:
                        try:
                            conn = sock.accept()[0]
                        except TimeoutError:
                            if cli.poll() is not None:
                                _, stderr = cli.communicate()
                                raise AssertionError(f'bitcoin-cli exited before connecting: {stderr.decode()}')
                    assert conn is not None
                    with conn:
                        conn.settimeout(5)
                        request = b''
                        while b'\r\n\r\n' not in request:
                            chunk = conn.recv(4096)
                            assert chunk
                            request += chunk
                        conn.sendall(b'HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nConnection: close\r\n\r\n')
                    cli.communicate(timeout=5)
                finally:
                    if cli.poll() is None:
                        cli.kill()
                        cli.wait()
                user, password = get_auth_cookie(self.nodes[0].datadir_path, self.chain)
                credential_captured = f'Authorization: Basic {str_to_b64str(f"{user}:{password}")}\r\n'.encode() in request
                self.stop_node(0)
            except FailedToStartError as e:
                started = False
                error = str(e)
                self.cleanup_partially_started_nodes()
        self.nodes[0].rpchost = None

        assert_equal(started, False)
        assert_equal(credential_captured, False)
        assert 'Unable to start HTTP server' in error

    def run_allowip_test(self, allow_ips, rpchost, rpcport):
        '''
        Start a node with rpcallow IP, and request getnetworkinfo
        at a non-localhost IP.
        '''
        success = True
        self.log.info("Allow IP test for %s:%d" % (rpchost, rpcport))
        node_args = \
            ['-disablewallet', '-nolisten'] + \
            ['-rpcallowip='+x for x in allow_ips] + \
            ['-rpcbind='+addr for addr in ['127.0.0.1', "%s:%d" % (rpchost, rpcport)]] # Bind to localhost as well so start_nodes doesn't hang
        self.nodes[0].rpchost = None
        self.start_nodes([node_args])
        self.nodes[0].rpchost = f"{rpchost}:{rpcport}"
        # connect to node through non-loopback interface
        node = self.nodes[0].create_new_rpc_connection()
        try:
            node.getnetworkinfo()
        except NETWORK_ERRORS:
            success = False
        self.stop_nodes()
        return success

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
        interfaces = all_interfaces()
        if not interfaces:
            raise AssertionError("all_interfaces() returned no IPv4 interfaces")
        self.non_loopback_ip = None
        for name,ip in interfaces:
            if not ip.startswith('127.'):
                self.non_loopback_ip = ip
                break
        if self.non_loopback_ip is None and self.options.run_nonloopback:
            raise SkipTest("This test requires a non-loopback ip address.")

        self.defaultport = rpc_port(0)

        if not self.options.run_nonloopback:
            if not self.options.run_ipv4:
                for explicit_binds in [False, True]:
                    self.run_partial_bind_test(explicit_binds)
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
            self.run_bind_test(['127.0.0.1'], '127.0.0.1', ['127.0.0.1', '127.0.0.1'],
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

        # Check that connections from allowed IPs are allowed
        assert self.run_allowip_test([self.non_loopback_ip], self.non_loopback_ip, self.defaultport)
        # Otherwise we are denied
        if self.options.usecli:
            self.log.info("Skip negative IP test with CLI, because the CLI can not throw the tested exception type")
            return
        assert not self.run_allowip_test(['1.1.1.1'], self.non_loopback_ip, self.defaultport)

if __name__ == '__main__':
    RPCBindTest(__file__).main()
