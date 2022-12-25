#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test syscoind with different proxy configuration.

Test plan:
- Start syscoind's with different proxy configurations
- Use addnode to initiate connections
- Verify that proxies are connected to, and the right connection command is given
- Proxy configurations to test on syscoind side:
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

- Test passing invalid -proxy
- Test passing invalid -onion
- Test passing invalid -i2psam
- Test passing -onlynet=onion without -proxy or -onion
- Test passing -onlynet=onion with -onion=0 and with -noonion
- Test passing unknown -onlynet
"""

import socket

from test_framework.socks5 import Socks5Configuration, Socks5Command, Socks5Server, AddressType
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
)
from test_framework.netutil import test_ipv6_local

# Networks returned by RPC getpeerinfo.
NET_UNROUTABLE = "not_publicly_routable"
NET_IPV4 = "ipv4"
NET_IPV6 = "ipv6"
NET_ONION = "onion"
NET_I2P = "i2p"
NET_CJDNS = "cjdns"

# Networks returned by RPC getnetworkinfo, defined in src/rpc/net.cpp::GetNetworksInfo()
NETWORKS = frozenset({NET_IPV4, NET_IPV6, NET_ONION, NET_I2P, NET_CJDNS})


class ProxyTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 5
        self.setup_clean_chain = True

    def setup_nodes(self):
        self.have_ipv6 = test_ipv6_local()
        # Create two proxies on different ports
        # ... one unauthenticated
        self.conf1 = Socks5Configuration()
        self.conf1.addr = ('127.0.0.1', p2p_port(self.num_nodes))
        self.conf1.unauth = True
        self.conf1.auth = False
        # ... one supporting authenticated and unauthenticated (Tor)
        self.conf2 = Socks5Configuration()
        self.conf2.addr = ('127.0.0.1', p2p_port(self.num_nodes + 1))
        self.conf2.unauth = True
        self.conf2.auth = True
        if self.have_ipv6:
            # ... one on IPv6 with similar configuration
            self.conf3 = Socks5Configuration()
            self.conf3.af = socket.AF_INET6
            self.conf3.addr = ('::1', p2p_port(self.num_nodes + 2))
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
        # Note: syscoind's SOCKS5 implementation only sends atyp DOMAINNAME, even if connecting directly to IPv4/IPv6
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
            # Note: syscoind's SOCKS5 implementation only sends atyp DOMAINNAME, even if connecting directly to IPv4/IPv6
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
        nodes_network_info = []

        self.log.debug("Test that setting -proxy disables local address discovery, i.e. -discover=0")
        for node in self.nodes:
            network_info = node.getnetworkinfo()
            assert_equal(network_info["localaddresses"], [])
            nodes_network_info.append(network_info)

        n0 = networks_dict(nodes_network_info[0])
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

        n1 = networks_dict(nodes_network_info[1])
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

        n2 = networks_dict(nodes_network_info[2])
        assert_equal(NETWORKS, n2.keys())
        proxy = f'{self.conf2.addr[0]}:{self.conf2.addr[1]}'
        for net in NETWORKS:
            if net == NET_I2P:
                expected_proxy = ''
                expected_randomize = False
            else:
                expected_proxy = proxy
                expected_randomize = True
            assert_equal(n2[net]['proxy'], expected_proxy)
            assert_equal(n2[net]['proxy_randomize_credentials'], expected_randomize)
        assert_equal(n2['onion']['reachable'], True)
        assert_equal(n2['i2p']['reachable'], False)
        assert_equal(n2['cjdns']['reachable'], False)

        if self.have_ipv6:
            n3 = networks_dict(nodes_network_info[3])
            assert_equal(NETWORKS, n3.keys())
            proxy = f'[{self.conf3.addr[0]}]:{self.conf3.addr[1]}'
            for net in NETWORKS:
                expected_proxy = '' if net == NET_I2P or net == NET_ONION else proxy
                assert_equal(n3[net]['proxy'], expected_proxy)
                assert_equal(n3[net]['proxy_randomize_credentials'], False)
            assert_equal(n3['onion']['reachable'], False)
            assert_equal(n3['i2p']['reachable'], False)
            assert_equal(n3['cjdns']['reachable'], False)

        n4 = networks_dict(nodes_network_info[4])
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

        self.stop_node(1)

        self.log.info("Test passing invalid -proxy hostname raises expected init error")
        self.nodes[1].extra_args = ["-proxy=abc..abc:23456"]
        msg = "Error: Invalid -proxy address or hostname: 'abc..abc:23456'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -proxy port raises expected init error")
        self.nodes[1].extra_args = ["-proxy=192.0.0.1:def"]
        msg = "Error: Invalid port specified in -proxy: '192.0.0.1:def'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -onion hostname raises expected init error")
        self.nodes[1].extra_args = ["-onion=xyz..xyz:23456"]
        msg = "Error: Invalid -onion address or hostname: 'xyz..xyz:23456'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -onion port raises expected init error")
        self.nodes[1].extra_args = ["-onion=192.0.0.1:def"]
        msg = "Error: Invalid port specified in -onion: '192.0.0.1:def'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -i2psam hostname raises expected init error")
        self.nodes[1].extra_args = ["-i2psam=def..def:23456"]
        msg = "Error: Invalid -i2psam address or hostname: 'def..def:23456'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -i2psam port raises expected init error")
        self.nodes[1].extra_args = ["-i2psam=192.0.0.1:def"]
        msg = "Error: Invalid port specified in -i2psam: '192.0.0.1:def'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -onlynet=i2p without -i2psam raises expected init error")
        self.nodes[1].extra_args = ["-onlynet=i2p"]
        msg = "Error: Outbound connections restricted to i2p (-onlynet=i2p) but -i2psam is not provided"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing invalid -onlynet=cjdns without -cjdnsreachable raises expected init error")
        self.nodes[1].extra_args = ["-onlynet=cjdns"]
        msg = "Error: Outbound connections restricted to CJDNS (-onlynet=cjdns) but -cjdnsreachable is not provided"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing -onlynet=onion with -onion=0/-noonion raises expected init error")
        msg = (
            "Error: Outbound connections restricted to Tor (-onlynet=onion) but "
            "the proxy for reaching the Tor network is explicitly forbidden: -onion=0"
        )
        for arg in ["-onion=0", "-noonion"]:
            self.nodes[1].extra_args = ["-onlynet=onion", arg]
            self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing -onlynet=onion without -proxy, -onion or -listenonion raises expected init error")
        self.nodes[1].extra_args = ["-onlynet=onion", "-listenonion=0"]
        msg = (
            "Error: Outbound connections restricted to Tor (-onlynet=onion) but the proxy for "
            "reaching the Tor network is not provided: none of -proxy, -onion or -listenonion is given"
        )
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)

        self.log.info("Test passing -onlynet=onion without -proxy or -onion but with -listenonion=1 is ok")
        self.start_node(1, extra_args=["-onlynet=onion", "-listenonion=1"])
        self.stop_node(1)

        self.log.info("Test passing unknown network to -onlynet raises expected init error")
        self.nodes[1].extra_args = ["-onlynet=abc"]
        msg = "Error: Unknown network specified in -onlynet: 'abc'"
        self.nodes[1].assert_start_raises_init_error(expected_msg=msg)


if __name__ == '__main__':
    ProxyTest().main()
