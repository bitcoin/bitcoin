#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test walletnotify.

Verify that a bitcoind node calls the given command for each wallet transaction
"""

import os
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class WalletNotifyTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()

        self.setup_clean_chain = True
        self.num_nodes = 1
        self.block_count = 10

    def setup_network(self):
        self.path = os.path.join(self.options.tmpdir, 'transactions.txt')

        # the command will append the notified transaction hash to file
        self.extra_args = [['-walletnotify=echo %%s >> %s' % self.path]]
        super().setup_network()

    def run_test(self):
        self.nodes[0].generate(self.block_count)
        self.test_file_content()

        # prepare for next test
        os.remove(self.path)

        # restart node with rescan to force wallet notifications
        self.stop_node(0)
        self.nodes[0] = self.start_node(0, self.options.tmpdir, ['-rescan', '-walletnotify=echo %%s >> %s' % self.path])
        self.test_file_content()

    def test_file_content(self):
        txids_rpc = list(map(lambda t: t['txid'], self.nodes[0].listtransactions("*", self.block_count)))

        # wait at most 10 seconds for expected file size before reading the content
        attempts = 0
        while os.stat(self.path).st_size < (self.block_count * 65) and attempts < 100:
            time.sleep(0.1)
            attempts += 1

        # file content should equal the generated transaction hashes
        with open(self.path, 'r') as f:
            txids_file = f.read().splitlines()

            # txids are sorted because there is no order guarantee in notifications
            assert_equal(sorted(txids_rpc), sorted(txids_file))

if __name__ == '__main__':
    WalletNotifyTest().main()
