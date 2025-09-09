#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that the proper port is used for -externalip=
"""

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.test_node import (
    FailedToStartError,
)
from test_framework.util import assert_equal, p2p_port

# We need to bind to a routable address for this test to exercise the relevant
# code. Those addresses must be on an interface that is UP and is not a loopback
# interface (IFF_LOOPBACK).
# To set these routable addresses on the machine, use:
# Linux:
# First find your interfaces: ip addr show
# Then use your actual interface names (replace INTERFACE_NAME with yours):
# ip addr add 1.1.1.5/32 dev INTERFACE_NAME  # to set up
# ip addr del 1.1.1.5/32 dev INTERFACE_NAME  # to remove it
# FreeBSD and MacOS:
# ifconfig INTERFACE_NAME 1.1.1.5/32 alias  # to set up
# ifconfig INTERFACE_NAME 1.1.1.5 -alias  # to remove it, after the test
ADDR = '1.1.1.5'

# array of tuples [arguments, expected port in localaddresses]
EXPECTED = [
    [['-externalip=2.2.2.2',       '-port=30001'],                        30001],
    [['-externalip=2.2.2.2',       '-port=30002', f'-bind={ADDR}'],       30002],
    [['-externalip=2.2.2.2',                      f'-bind={ADDR}'],       'default_p2p_port'],
    [['-externalip=2.2.2.2',       '-port=30003', f'-bind={ADDR}:30004'], 30004],
    [['-externalip=2.2.2.2',                      f'-bind={ADDR}:30005'], 30005],
    [['-externalip=2.2.2.2:30006', '-port=30007'],                        30006],
    [['-externalip=2.2.2.2:30008', '-port=30009', f'-bind={ADDR}'],       30008],
    [['-externalip=2.2.2.2:30010',                f'-bind={ADDR}'],       30010],
    [['-externalip=2.2.2.2:30011', '-port=30012', f'-bind={ADDR}:30013'], 30011],
    [['-externalip=2.2.2.2:30014',                f'-bind={ADDR}:30015'], 30014],
    [['-externalip=2.2.2.2',       '-port=30016', f'-bind={ADDR}:30017',
                                             f'-whitebind={ADDR}:30018'], 30017],
    [['-externalip=2.2.2.2',       '-port=30019',
                                             f'-whitebind={ADDR}:30020'], 30020],
]

class BindPortExternalIPTest(BitcoinTestFramework):
    def set_test_params(self):
        # Avoid any -bind= on the command line. Force the framework to avoid adding -bind=127.0.0.1.
        self.setup_clean_chain = True
        self.bind_to_localhost_only = False
        self.num_nodes = len(EXPECTED)
        self.extra_args = list(map(lambda e: e[0], EXPECTED))

    def setup_network(self):
        self.setup_nodes()

    def setup_nodes(self):
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
                    f'To run this test make sure that {ADDR} '
                    '(routable address) is assigned to non-loopback '
                    'interface on this machine')
            else:
                raise e

    def run_test(self):
        self.log.info("Test the proper port is used for -externalip=")
        for i in range(len(EXPECTED)):
            expected_port = EXPECTED[i][1]
            if expected_port == 'default_p2p_port':
                expected_port = p2p_port(i)
            found = False
            for local in self.nodes[i].getnetworkinfo()['localaddresses']:
                if local['address'] == '2.2.2.2':
                    assert_equal(local['port'], expected_port)
                    found = True
                    break
            assert found

if __name__ == '__main__':
    BindPortExternalIPTest(__file__).main()
