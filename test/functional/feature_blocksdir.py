#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the blocksdir option.
"""

from test_framework.test_framework import BitcoinTestFramework, initialize_datadir

import shutil
import os

class BlocksdirTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.stop_node(0)
        node0path = os.path.join(self.options.tmpdir, "node0")
        shutil.rmtree(node0path)
        initialize_datadir(self.options.tmpdir, 0)
        self.log.info("Starting with non exiting blocksdir ...")
        self.assert_start_raises_init_error(0, ["-blocksdir="+self.options.tmpdir+ "/blocksdir"], "Specified blocks director")
        os.mkdir(self.options.tmpdir+ "/blocksdir")
        self.log.info("Starting with exiting blocksdir ...")
        self.start_node(0, ["-blocksdir="+self.options.tmpdir+ "/blocksdir"])
        self.log.info("mining blocks..")
        self.nodes[0].generate(10)
        assert(os.path.isfile(self.options.tmpdir+ "/blocksdir/regtest/blocks/blk00000.dat"))
        assert(os.path.isdir(self.options.tmpdir+ "/blocksdir/regtest/blocks/index"))

if __name__ == '__main__':
    BlocksdirTest().main()
