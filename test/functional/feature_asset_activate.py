#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import SyscoinTestFramework
import random
import time

class AssetActivateTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.generate(self.nodes[0], 200)
        self.basic_test(100, 8)

    def newasset(self):
        self.nodes[0].assetnew('0', 'TST', 'asset description', '0x', 8, 10000, 127, '', {}, {})

    def basic_test(self, numblocks, numassets):
        self.stop_node(1)
        randblocks = random.randrange(10, numblocks)
        self.log.info('starting with {} blocks'.format(randblocks))
        for i in range(randblocks):
            randassets = random.randrange(1, numassets)
            for _ in range(randassets):
                time.sleep(0.01)
                self.newasset()
            self.generate(self.nodes[0], 1)
            self.log.info('{}/{} block generated with {} assets'.format(i+1, randblocks, randassets))
        self.start_node(1)
        self.connect_nodes(0, 1)
        self.sync_blocks()

if __name__ == '__main__':
    AssetActivateTest().main()
