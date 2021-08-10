#!/usr/bin/env python3
# Copyright (c) 2021-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test -onlynet configuration option.
"""

from test_framework.basic_server import (
    BasicServer,
    tor_control,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class OnlynetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.tor_control_server = BasicServer(bind=('127.0.0.1', 0),
                                              response_generator=tor_control)
        port = self.tor_control_server.listen_port
        self.extra_args = [
            ['-onlynet=ipv4', f'-torcontrol=127.0.0.1:{port}', '-listenonion=1'],
            ['-onlynet=ipv4', f'-torcontrol=127.0.0.1:{port}', '-listenonion=1', '-onion=127.0.0.1:9050'],
            ['-onlynet=ipv4', f'-torcontrol=127.0.0.1:{port}', '-listenonion=1', '-proxy=127.0.0.1:9050'],
        ]
        self.num_nodes = len(self.extra_args)

    def onion_is_reachable(self, node):
        for net in self.nodes[node].getnetworkinfo()['networks']:
            if net['name'] == 'onion':
                return net['reachable']
        return False

    def run_test(self):
        # Node 0 would fail the check without 5384c98993fed5480719e1c3380c0c66263daa7e.
        # Nodes 1 and 2 pass with or without that commit, they are here just to ensure
        # behavior does not change unintentionally.
        for node, expected_reachable in [[0, False], [1, True], [2, True]]:
            self.log.info(f'Test node {node} {self.extra_args[node]}')
            assert_equal(self.onion_is_reachable(node), expected_reachable)


if __name__ == '__main__':
    OnlynetTest().main()
