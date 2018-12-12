#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reported errors on file corruption.

See doc/files.md for a list of files.
"""

import os

from test_framework.test_framework import BitcoinTestFramework


class FileCorruptionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.log.info('Test file corruption on mempool.dat')
        mempool_dat_path = os.path.join(self.nodes[0].datadir, 'regtest', 'mempool.dat')
        self.stop_nodes()
        with open(mempool_dat_path, 'rb+') as mempool_dat:
            mempool_dat.seek(9)
            mempool_dat.write(b'ff')
        with self.nodes[0].assert_debug_log(expected_msgs=['Failed to deserialize mempool data on disk']):
            self.start_nodes()
        self.stop_node(0, expected_stderr='Warning: Failed to deserialize mempool data on disk: CAutoFile::read: end of file: iostream error. Continuing anyway.')


if __name__ == '__main__':
    FileCorruptionTest().main()
