#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test successful startup with symlinked directories.
"""

import os
import sys

from test_framework.test_framework import BitcoinTestFramework, SkipTest


def rename_and_link(*, from_name, to_name):
    os.rename(from_name, to_name)
    os.symlink(to_name, from_name)
    assert os.path.islink(from_name) and os.path.isdir(from_name)

class SymlinkTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        if sys.platform == 'win32':
            raise SkipTest("Symlinks test skipped on Windows")

    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.stop_node(0)

        rename_and_link(from_name=os.path.join(self.nodes[0].datadir, self.chain, "blocks"),
                        to_name=os.path.join(self.nodes[0].datadir, self.chain, "newblocks"))
        rename_and_link(from_name=os.path.join(self.nodes[0].datadir, self.chain, "chainstate"),
                        to_name=os.path.join(self.nodes[0].datadir, self.chain, "newchainstate"))

        self.start_node(0)


if __name__ == '__main__':
    SymlinkTest().main()
