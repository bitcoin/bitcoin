#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test file system permissions for POSIX platforms.
"""

import os
import stat

from test_framework.test_framework import SyscoinTestFramework


class PosixFsPermissionsTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_posix()

    def check_directory_permissions(self, dir):
        mode = os.lstat(dir).st_mode
        self.log.info(f"{stat.filemode(mode)} {dir}")
        assert mode == (stat.S_IFDIR | stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)

    def check_file_permissions(self, file):
        mode = os.lstat(file).st_mode
        self.log.info(f"{stat.filemode(mode)} {file}")
        assert mode == (stat.S_IFREG | stat.S_IRUSR | stat.S_IWUSR)

    def run_test(self):
        self.stop_node(0)
        datadir = os.path.join(self.nodes[0].datadir, self.chain)
        self.check_directory_permissions(datadir)
        walletsdir = os.path.join(datadir, "wallets")
        self.check_directory_permissions(walletsdir)
        debuglog = os.path.join(datadir, "debug.log")
        self.check_file_permissions(debuglog)


if __name__ == '__main__':
    PosixFsPermissionsTest().main()
