#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Check that it's not possible to start a second bitcoind instance using the same datadir or wallet."""
import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch

class FilelockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=None)
        self.nodes[0].start()
        self.nodes[0].wait_for_rpc_connection()

    def run_test(self):
        datadir = os.path.join(self.nodes[0].datadir, self.chain)
        self.log.info("Using datadir {}".format(datadir))

        self.log.info("Check that we can't start a second bitcoind instance using the same datadir")
        expected_msg = "Error: Cannot obtain a lock on data directory {0}. {1} is probably already running.".format(datadir, self.config['environment']['PACKAGE_NAME'])
        self.nodes[1].assert_start_raises_init_error(extra_args=['-datadir={}'.format(self.nodes[0].datadir), '-noserver'], expected_msg=expected_msg)

        if self.is_wallet_compiled():
            self.nodes[0].createwallet(self.default_wallet_name)
            wallet_dir = os.path.join(datadir, 'wallets')
            self.log.info("Check that we can't start a second bitcoind instance using the same wallet")
            expected_msg = "Error: Error initializing wallet database environment"
            self.nodes[1].assert_start_raises_init_error(extra_args=['-walletdir={}'.format(wallet_dir), '-wallet=' + self.default_wallet_name, '-noserver'], expected_msg=expected_msg, match=ErrorMatch.PARTIAL_REGEX)

if __name__ == '__main__':
    FilelockTest().main()
