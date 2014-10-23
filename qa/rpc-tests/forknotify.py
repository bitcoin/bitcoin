#!/usr/bin/env python
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test -alertnotify 
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *
import os
import shutil

class ForkNotifyTest(BitcoinTestFramework):

    alert_filename = None  # Set by setup_network

    def setup_network(self, test_dir):
        nodes = []
        self.alert_filename = os.path.join(test_dir, "alert.txt")
        with open(self.alert_filename, 'w') as f:
            pass  # Just open then close to create zero-length file
        nodes.append(start_node(0, test_dir,
                            ["-blockversion=2", "-alertnotify=echo %s >> '" + self.alert_filename + "'"]))
        # Node1 mines block.version=211 blocks
        nodes.append(start_node(1, test_dir,
                                ["-blockversion=211"]))
        connect_nodes(nodes[1], 0)

        sync_blocks(nodes)
        return nodes
        

    def run_test(self, nodes):
        # Mine 51 up-version blocks
        nodes[1].setgenerate(True, 51)
        sync_blocks(nodes)
        # -alertnotify should trigger on the 51'st,
        # but mine and sync another to give
        # -alertnotify time to write
        nodes[1].setgenerate(True, 1)
        sync_blocks(nodes)

        with open(self.alert_filename, 'r') as f:
            alert_text = f.read()

        if len(alert_text) == 0:
            raise AssertionError("-alertnotify did not warn of up-version blocks")

        # Mine more up-version blocks, should not get more alerts:
        nodes[1].setgenerate(True, 1)
        sync_blocks(nodes)
        nodes[1].setgenerate(True, 1)
        sync_blocks(nodes)

        with open(self.alert_filename, 'r') as f:
            alert_text2 = f.read()

        if alert_text != alert_text2:
            raise AssertionError("-alertnotify excessive warning of up-version blocks")

if __name__ == '__main__':
    ForkNotifyTest().main()
