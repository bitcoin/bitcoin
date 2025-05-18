#!/usr/bin/env python3
# Copyright (c) 2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test successful startup with symlinked directories.
"""

import os

from test_framework.test_framework import TortoisecoinTestFramework


def rename_and_link(*, from_name, to_name):
    os.rename(from_name, to_name)
    os.symlink(to_name, from_name)
    assert os.path.islink(from_name) and os.path.isdir(from_name)


class SymlinkTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        dir_new_blocks = self.nodes[0].chain_path / "new_blocks"
        dir_new_chainstate = self.nodes[0].chain_path / "new_chainstate"
        self.stop_node(0)

        rename_and_link(
            from_name=self.nodes[0].blocks_path,
            to_name=dir_new_blocks,
        )
        rename_and_link(
            from_name=self.nodes[0].chain_path / "chainstate",
            to_name=dir_new_chainstate,
        )

        self.start_node(0)


if __name__ == "__main__":
    SymlinkTest(__file__).main()
