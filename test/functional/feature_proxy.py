#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bitcoind with different proxy configuration.

Test plan:
- Start bitcoind's with different proxy configurations
- Use addnode to initiate connections
- Verify that proxies are connected to, and the right connection command is given
- Proxy configurations to test on bitcoind side:
    - `-proxy` (proxy everything)
    - `-onion` (proxy just onions)
    - `-proxyrandomize` Circuit randomization
    - `-cjdnsreachable`
- Proxy configurations to test on proxy side,
    - support no authentication (other proxy)
    - support no authentication + user/pass authentication (Tor)
    - proxy on IPv6

- Create various proxies (as threads)
- Create nodes that connect to them
- Manipulate the peer connections using addnode (onetry) and observe effects
- Test the getpeerinfo `network` field for the peer

addnode connect to IPv4
addnode connect to IPv6
addnode connect to onion
addnode connect to generic DNS name
addnode connect to a CJDNS address

- Test getnetworkinfo for each node
"""

import socket
import os

from test_framework.socks5 import Socks5Configuration, Socks5Command, Socks5Server, AddressType
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    PORT_MIN,
    PORT_RANGE,
    assert_equal,
)
from test_framework.netutil import test_ipv6_local

RANGE_BEGIN = PORT_MIN + 2 * PORT_RANGE  # Start after p2p and rpc ports

# Networks returned by RPC getpeerinfo.
NET_UNROUTABLE = "not_publicly_routable"
NET_IPV4 = "ipv4"
NET_IPV6 = "ipv6"
NET_ONION = "onion"
NET_I2P = "i2p"
NET_CJDNS = "cjdns"

# Networks returned by RPC getnetworkinfo, defined in src/rpc/net.cpp::GetNetworksInfo()
NETWORKS = frozenset({NET_IPV4, NET_IPV6, NET_ONION, NET_I2P, NET_CJDNS})


class ProxyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 5
        self.setup_clean_chain = True

    def setup_nodes(self):
        self.have_ipv6 = test_ipv6_local()
        # Create two proxies on different ports
        # ... one unauthenticated
        self.conf1 = Socks5Configuration()
        self.conf1.addr = ('127.0.0.1', RANGE_BEGIN + (os.getpid() % 1000))
        self.conf1.unauth = True
        self.conf1.auth = False
        # ... one supporting authenticated and unauthenticated (Tor)
        self.conf2 = Socks5Configuration()
        self.conf2.addr = ('127.0.0.1', RANGE_BEGIN + 1000 + (os.getpid() % 1000))
        self.conf2.unauth = True
        self.conf2.auth = True
        if self.have_ipv6:
            # ... one on IPv6 with similar configuration
            self.conf3 = Socks5Configuration()
            self.conf3.af = socket.AF_INET6
            self.conf3.addr = ('::1', RANGE_BEGIN + 2000 + (os.getpid() % 1000))
            self.conf3.unauth = True
            self.conf3.auth = True
        else:
            self.log.warning("Testing without local IPv6 support")

        self.serv1 = Socks5Server(self.conf1)
        self.serv1.start()
        self.serv2 = Socks5Server(self.conf2)
        self.serv2.start()
        if self.have_ipv6:
            self.serv3 = Socks5Server(self.conf3)
            self.serv3.start()

        # We will not try to connect to this.
        self.i2p_sam = ('127.0.0.1', 7656)

        # Note: proxies are not used to connect to local nodes. This is because the proxy to
        # use is based on CService.GetNetwork(), which returns NET_UNROUTABLE for localhost.
        args = [
            ['-listen', f'-proxy={self.conf1.addr[0]}:{self.conf1.addr[1]}','-proxyrandomize=1'],
            ['-listen', f'-proxy={self.conf1.addr[0]}:{self.conf1.addr[1]}',f'-onion={self.conf2.addr[0]}:{self.conf2.addr[1]}',
                f'-i2psam={self.i2p_sam[0]}:{self.i2p_sam[1]}', '-i2pacceptincoming=0', '-proxyrandomize=0'],
            ['-listen', f'-proxy={self.conf2.addr[0]}:{self.conf2.addr[1]}','-proxyrandomize=1'],
            [],
            ['-listen', f'-proxy={self.conf1.addr[0]}:{self.conf1.addr[1]}','-proxyrandomize=1',
                '-cjdnsreachable']
        ]
        if self.have_ipv6:
            args[3] = ['-listen', f'-proxy=[{self.conf3.addr[0]}]:{self.conf3.addr[1]}','-proxyrandomize=0', '-noonion']
        self.add_nodes(self.num_nodes, extra_args=args)
        self.start_nodes()

    def network_test(self, node, addr, network):
        for peer in node.getpeerinfo():
            if peer["addr"] == addr:
                assert_equal(peer["network"], network)

    def node_test(self, node, *, proxies, auth, test_onion, test_cjdns):
        rv = []
        addr = "15.61.23.23:1234"
        self.log.debug(f"Test: outgoing IPv4 connection through node for address {addr}")
        node.addnode(addr, "onetry")
        cmd = proxies[0].queue.get()
        assert isinstance(cmd, Socks5Command)
        # Note: bitcoind's SOCKS5 implementation only sends atyp DOMAINNAME, even if connecting directly to IPv4/IPv6
        assert_equal(cmd.atyp, AddressType.DOMAINNAME)
        assert_equal(cmd.addr, b"15.61.23.23")
        assert_equal(cmd.port, 1234)
        if not auth:
            assert_equal(cmd.username, None)
            assert_equal(cmd.password, None)
        rv.append(cmd)
        self.network_test(node, addr, network=NET_IPV4)

        if self.have_ipv6:
            addr = "[1233:3432:2434:2343:3234:2345:6546:4534]:5443"
            self.log.debug(f"Test: outgoing IPv6 connection through node for address {addr}")
            node.addnode(addr, "onetry")
            cmd = proxies[1].queue.get()
            assert isinstance(cmd, Socks5Command)
            # Note: bitcoind's SOCKS5 implementation only sends atyp DOMAINNAME, even if connecting directly to IPv4/IPv6
            assert_equal(cmd.atyp, AddressType.DOMAINNAME)
            assert_equal(cmd.addr, b"1233:3432:2434:2343:3234:2345:6546:4534")
            assert_equal(cmd.port, 5443)
            if not auth:
                assert_equal(cmd.username, None)
                assert_equal(cmd.password, None)
            rv.append(cmd)
            self.network_test(node, addr, network=NET_IPV6)

        if test_onion:
            addr = "pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion:8333"
            self.log.debug(f"Test: outgoing onion connection through node for address {addr}")
            node.addnode(addr, "onetry")
            cmd = proxies[2].queue.get()
            assert isinstance(cmd, Socks5Command)
            assert_equal(cmd.atyp, AddressType.DOMAINNAME)
            assert_equal(cmd.addr, b"pg6mmjiyjmcrsslvykfwnntlaru7p5svn6y2ymmju6nubxndf4pscryd.onion")
            assert_equal(cmd.port, 8333)
            if not auth:
                assert_equal(cmd.username, None)
                assert_equal(cmd.password, None)
            rv.append(cmd)
            self.network_test(node, addr, network=NET_ONION)

        if test_cjdns:
            addr = "[fc00:1:2:3:4:5:6:7]:8888"
            self.log.debug(f"Test: outgoing CJDNS connection through node for address {addr}")
            node.addnode(addr, "onetry")
            cmd = proxies[1].queue.get()
            assert isinstance(cmd, Socks5Command)
            assert_equal(cmd.atyp, AddressType.DOMAINNAME)
            assert_equal(cmd.addr, b"fc00:1:2:3:4:5:6:7")
            assert_equal(cmd.port, 8888)
            if not auth:
                assert_equal(cmd.username, None)
                assert_equal(cmd.password, None)
            rv.append(cmd)
            self.network_test(node, addr, network=NET_CJDNS)

        addr = "node.noumenon:8333"
        self.log.debug(f"Test: outgoing DNS name connection through node for address {addr}")
        node.addnode(addr, "onetry")
        cmd = proxies[3].queue.get()
        assert isinstance(cmd, Socks5Command)
        assert_equal(cmd.atyp, AddressType.DOMAINNAME)
        assert_equal(cmd.addr, b"node.noumenon")
        assert_equal(cmd.port, 8333)
        if not auth:
            assert_equal(cmd.username, None)
            assert_equal(cmd.password, None)
        rv.append(cmd)
        self.network_test(node, addr, network=NET_UNROUTABLE)

        return rv

    def run_test(self):
        # basic -proxy
        self.node_test(self.nodes[0],
            proxies=[self.serv1, self.serv1, self.serv1, self.serv1],
            auth=False, test_onion=True, test_cjdns=False)

        # -proxy plus -onion
        self.node_test(self.nodes[1],
            proxies=[self.serv1, self.serv1, self.serv2, self.serv1],
            auth=False, test_onion=True, test_cjdns=False)

        # -proxy plus -onion, -proxyrandomize
        rv = self.node_test(self.nodes[2],
            proxies=[self.serv2, self.serv2, self.serv2, self.serv2],
            auth=True, test_onion=True, test_cjdns=False)
        # Check that credentials as used for -proxyrandomize connections are unique
        credentials = set((x.username,x.password) for x in rv)
        assert_equal(len(credentials), len(rv))

        if self.have_ipv6:
            # proxy on IPv6 localhost
            self.node_test(self.nodes[3],
                proxies=[self.serv3, self.serv3, self.serv3, self.serv3],
                auth=False, test_onion=False, test_cjdns=False)

        # -proxy=unauth -proxyrandomize=1 -cjdnsreachable
        self.node_test(self.nodes[4],
            proxies=[self.serv1, self.serv1, self.serv1, self.serv1],
            auth=False, test_onion=True, test_cjdns=True)

        def networks_dict(d):
            r = {}
            for x in d['networks']:
                r[x['name']] = x
            return r

        self.log.info("Test RPC getnetworkinfo")
        n0 = networks_dict(self.nodes[0].getnetworkinfo())
        assert_equal(NETWORKS, n0.keys())
        for net in NETWORKS:
            if net == NET_I2P:
                expected_proxy = ''
                expected_randomize = False
            else:
                expected_proxy = '%s:%i' % (self.conf1.addr)
                expected_randomize = True
            assert_equal(n0[net]['proxy'], expected_proxy)
            assert_equal(n0[net]['proxy_randomize_credentials'], expected_randomize)
        assert_equal(n0['onion']['reachable'], True)
        assert_equal(n0['i2p']['reachable'], False)
        assert_equal(n0['cjdns']['reachable'], False)

        n1 = networks_dict(self.nodes[1].getnetworkinfo())
        assert_equal(NETWORKS, n1.keys())
        for net in ['ipv4', 'ipv6']:
            assert_equal(n1[net]['proxy'], f'{self.conf1.addr[0]}:{self.conf1.addr[1]}')
            assert_equal(n1[net]['proxy_randomize_credentials'], False)
        assert_equal(n1['onion']['proxy'], f'{self.conf2.addr[0]}:{self.conf2.addr[1]}')
        assert_equal(n1['onion']['proxy_randomize_credentials'], False)
        assert_equal(n1['onion']['reachable'], True)
        assert_equal(n1['i2p']['proxy'], f'{self.i2p_sam[0]}:{self.i2p_sam[1]}')
        assert_equal(n1['i2p']['proxy_randomize_credentials'], False)
        assert_equal(n1['i2p']['reachable'], True)

        n2 = networks_dict(self.nodes[2].getnetworkinfo())
        assert_equal(NETWORKS, n2.keys())
        for net in NETWORKS:
            if net == NET_I2P:
                expected_proxy = ''
                expected_randomize = False
            else:
                expected_proxy = f'{self.conf2.addr[0]}:{self.conf2.addr[1]}'
                expected_randomize = True
            assert_equal(n2[net]['proxy'], expected_proxy)
            assert_equal(n2[net]['proxy_randomize_credentials'], expected_randomize)
        assert_equal(n2['onion']['reachable'], True)
        assert_equal(n2['i2p']['reachable'], False)
        assert_equal(n2['cjdns']['reachable'], False)

        if self.have_ipv6:
            n3 = networks_dict(self.nodes[3].getnetworkinfo())
            assert_equal(NETWORKS, n3.keys())
            for net in NETWORKS:
                if net == NET_I2P:
                    expected_proxy = ''
                else:
                    expected_proxy = f'[{self.conf3.addr[0]}]:{self.conf3.addr[1]}'
                assert_equal(n3[net]['proxy'], expected_proxy)
                assert_equal(n3[net]['proxy_randomize_credentials'], False)
            assert_equal(n3['onion']['reachable'], False)
            assert_equal(n3['i2p']['reachable'], False)
            assert_equal(n3['cjdns']['reachable'], False)

        n4 = networks_dict(self.nodes[4].getnetworkinfo())
        assert_equal(NETWORKS, n4.keys())
        for net in NETWORKS:
            if net == NET_I2P:
                expected_proxy = ''
                expected_randomize = False
            else:
                expected_proxy = '%s:%i' % (self.conf1.addr)
                expected_randomize = True
            assert_equal(n4[net]['proxy'], expected_proxy)
            assert_equal(n4[net]['proxy_randomize_credentials'], expected_randomize)
        assert_equal(n4['onion']['reachable'], True)
        assert_equal(n4['i2p']['reachable'], False)
        assert_equal(n4['cjdns']['reachable'], True)


if __name__ == '__main__':
    ProxyTest().main()
