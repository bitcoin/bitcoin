#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test successful startup with symlinked directories.
"""

import os
import shutil
import sys

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch


def rename_and_link(*, from_name, to_name):
    os.rename(from_name, to_name)
    os.symlink(to_name, from_name)
    assert os.path.islink(from_name) and os.path.isdir(from_name)


class SymlinkTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        dir_new_blocks = self.nodes[0].chain_path / "new_blocks"
        dir_new_chainstate = self.nodes[0].chain_path / "new_chainstate"
        self.stop_node(0)

        rename_and_link(
            from_name=self.nodes[0].chain_path / "blocks",
            to_name=dir_new_blocks,
        )
        rename_and_link(
            from_name=self.nodes[0].chain_path / "chainstate",
            to_name=dir_new_chainstate,
        )

        self.start_node(0)
        self.stop_node(0)

        self.log.info("Symlink pointing to a deleted blocksdir raises an init error")
        shutil.rmtree(dir_new_blocks)
        self.nodes[0].assert_start_raises_init_error(
            expected_msg="create_directories: " if sys.platform == "win32"
            # Temporarily check empty string on non-Windows platforms to work
            # around libc++ bug, affecting libc++-11 and lower.
            # See https://github.com/bitcoin/bitcoin/pull/24432#issuecomment-1049060318
            else "",
            match=ErrorMatch.PARTIAL_REGEX,
        )


if __name__ == "__main__":
    SymlinkTest().main()
