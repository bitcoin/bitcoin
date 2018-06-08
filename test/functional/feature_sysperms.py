#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify -sysperms=false (default) works as expected."""

import os
import platform
import stat

from test_framework.test_framework import BitcoinTestFramework, SkipTest


class SyspermsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def check_nosysperms_pathname(self, fullpathname):
        self.log.info('{}  {}'.format(stat.filemode(os.stat(fullpathname).st_mode), fullpathname))
        return (os.stat(fullpathname).st_mode & (stat.S_IRWXO | stat.S_IRWXG)) == 0

    def check_nosysperms_dir(self, regtest_dirpath):
        result = []
        for path, dirs, files in os.walk(regtest_dirpath):
            for dirname in dirs:
                result.append(self.check_nosysperms_pathname(os.path.join(path, dirname)))

            for filename in files:
                result.append(self.check_nosysperms_pathname(os.path.join(path, filename)))
        return result

    def run_test(self):
        self.log.info("Check for valid platform")
        if platform.system() == 'Windows':
            raise SkipTest("This test does not run on Windows.")

        self.restart_node(0)
        datadir = self.nodes[0].datadir

        self.log.info("Running test without -sysperms option - assuming umask of '077'")
        self.log.info('-datadir: {}'.format(datadir))

        self.log.info('Checking permissions... ')
        # check regtest dir and its contents
        regtest_dirpath = os.path.join(datadir, 'regtest')
        assert self.check_nosysperms_pathname(regtest_dirpath)
        assert any(self.check_nosysperms_dir(regtest_dirpath))
        self.log.info('Pass.')

        # Run test again with -sysperms
        self.stop_node(1)
        # Now retry an confirm node starts as expected
        self.log.info("Running test with -sysperms -disablewallet options")
        self.start_node(1, extra_args=["-sysperms", "-disablewallet"])
        self.log.info('Pass.')


if __name__ == '__main__':
    SyspermsTest().main()
