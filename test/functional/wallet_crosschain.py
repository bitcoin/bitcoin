#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import os
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, \
    assert_raises_rpc_error, assert_raises_process_error, \
    connect_nodes, sync_blocks, sync_mempools


class WalletCrossChain(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self, split=False):
        self.setup_nodes()
        connect_nodes(self.nodes[1], 2)

    def run_test(self):
        self.log.info("Generating initial blockchains")
        # Separate blockchain.
        self.nodes[0].generate(10)

        # Create wallet below allowed height difference (5 blocks).
        self.nodes[1].generate(5)
        ok_path = os.path.join(self.nodes[1].datadir, 'ok-wallet')
        self.nodes[1].createwallet(ok_path)
        self.nodes[1].unloadwallet(ok_path)

        # Create wallet at allowed height difference (6 blocks).
        self.nodes[1].generate(1)
        edge_path = os.path.join(self.nodes[1].datadir, 'edge-wallet')
        self.nodes[1].createwallet(edge_path)
        self.nodes[1].unloadwallet(edge_path)

        # Create wallet outside allowed height difference (7 blocks).
        self.nodes[1].generate(1)
        not_ok_path = os.path.join(self.nodes[1].datadir, 'not-ok-wallet')
        self.nodes[1].createwallet(not_ok_path)
        self.nodes[1].unloadwallet(not_ok_path)

        self.log.info("Loading reorg wallet within allowed height difference")
        self.nodes[0].loadwallet(ok_path)

        self.log.info("Loading reorg wallet exactly at allowed height difference")
        self.nodes[0].loadwallet(edge_path)

        self.log.info("Loading reorg wallet outside allowed height difference")
        assert_raises_rpc_error(-4, 'Wallet loading failed.', self.nodes[0].loadwallet, not_ok_path)
        self.stop_node(0, expected_stderr='Error: Wallet files should not be reused across chains')

        self.sync_blocks(self.nodes[1:3])

        # Create wallet below allowed height difference (5 blocks).
        self.nodes[2].generate(5)
        ok_path = os.path.join(self.nodes[2].datadir, 'ok-wallet')
        self.nodes[2].createwallet(ok_path)
        self.nodes[2].unloadwallet(ok_path)

        # Create wallet exactly at allowed height difference (6 blocks).
        self.nodes[2].generate(1)
        edge_path = os.path.join(self.nodes[2].datadir, 'edge-wallet')
        self.nodes[2].createwallet(edge_path)
        self.nodes[2].unloadwallet(edge_path)

        # Create wallet outside allowed height difference (7 blocks).
        self.nodes[2].generate(1)
        not_ok_path = os.path.join(self.nodes[2].datadir, 'not-ok-wallet')
        self.nodes[2].createwallet(not_ok_path)
        self.nodes[2].unloadwallet(not_ok_path)

        self.log.info("Loading no-reorg wallet within allowed height difference")
        self.nodes[1].loadwallet(ok_path)

        self.log.info("Loading no-reorg wallet exactly at allowed height difference")
        self.nodes[1].loadwallet(edge_path)

        self.log.info("Loading no-reorg wallet outside allowed height difference")
        self.nodes[1].loadwallet(not_ok_path)


if __name__ == '__main__':
    WalletCrossChain().main()
