#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test file system permissions for non-Windows platforms.
"""

import os
import stat

from test_framework.test_framework import BitcoinTestFramework


class FsPermissionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def check_directory(self, path):
        assert os.path.isdir(path)
        permissions = os.stat(path).st_mode
        self.log.info('{}  {}'.format(stat.filemode(permissions), path))
        assert (permissions & (stat.S_IRWXO | stat.S_IRWXG)) == 0

    def run_test(self):
        self.stop_node(0)
        datadir = os.path.join(self.nodes[0].datadir, self.chain)
        self.check_directory(datadir)
        walletsdir = os.path.join(datadir, "wallets")
        self.check_directory(walletsdir)


if __name__ == '__main__' and os.name != 'nt':
    FsPermissionsTest().main()
