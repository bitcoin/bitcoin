#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test -startupnotify."""

import os

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
)

NODE_DIR = "node0"
FILE_NAME = "test.txt"


class StartupNotifyTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.disable_syscall_sandbox = True

    def run_test(self):
        tmpdir_file = os.path.join(self.options.tmpdir, NODE_DIR, FILE_NAME)
        assert not os.path.exists(tmpdir_file)

        self.log.info("Test -startupnotify command is run when node starts")
        self.restart_node(0, extra_args=[f"-startupnotify=echo '{FILE_NAME}' >> {NODE_DIR}/{FILE_NAME}"])
        self.wait_until(lambda: os.path.exists(tmpdir_file))

        self.log.info("Test -startupnotify is executed once")

        def get_count():
            with open(tmpdir_file, "r", encoding="utf8") as f:
                file_content = f.read()
                return file_content.count(FILE_NAME)

        self.wait_until(lambda: get_count() > 0)
        assert_equal(get_count(), 1)

        self.log.info("Test node is fully started")
        assert_equal(self.nodes[0].getblockcount(), 200)


if __name__ == '__main__':
    StartupNotifyTest().main()
