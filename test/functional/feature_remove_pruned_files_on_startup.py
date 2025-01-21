#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test removing undeleted pruned blk files on startup."""

import platform
from test_framework.test_framework import BitcoinTestFramework


class FeatureRemovePrunedFilesOnStartupTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-fastprune", "-prune=1"]]

    def mine_batches(self, blocks):
        n = blocks // 250
        for _ in range(n):
            self.generate(self.nodes[0], 250)
        self.generate(self.nodes[0], blocks % 250)

    def run_test(self):
        blk0 = self.nodes[0].blocks_path / "blk00000.dat"
        rev0 = self.nodes[0].blocks_path / "rev00000.dat"
        blk1 = self.nodes[0].blocks_path / "blk00001.dat"
        rev1 = self.nodes[0].blocks_path / "rev00001.dat"
        self.mine_batches(800)

        self.log.info("Open some files to check that this may delay deletion")
        fd1 = open(blk0, "rb")
        fd2 = open(rev1, "rb")
        self.nodes[0].pruneblockchain(600)

        # Windows systems will not remove files with an open fd
        if platform.system() != 'Windows':
            assert not blk0.exists()
            assert not rev0.exists()
            assert not blk1.exists()
            assert not rev1.exists()
        else:
            assert blk0.exists()
            assert not rev0.exists()
            assert not blk1.exists()
            assert rev1.exists()

        self.log.info("Check that the files are removed on restart once the fds are closed")
        fd1.close()
        fd2.close()
        self.restart_node(0)
        assert not blk0.exists()
        assert not rev1.exists()


if __name__ == '__main__':
    FeatureRemovePrunedFilesOnStartupTest(__file__).main()
