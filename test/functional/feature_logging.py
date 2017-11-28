#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test debug logging."""

import os

from test_framework.test_framework import BitcoinTestFramework

class LoggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        # test default log file name
        assert os.path.isfile(os.path.join(self.nodes[0].datadir, "regtest", "debug.log"))

        # test alternative log file name in datadir
        self.restart_node(0, ["-debuglogfile=foo.log"])
        assert os.path.isfile(os.path.join(self.nodes[0].datadir, "regtest", "foo.log"))

        # test alternative log file name outside datadir
        tempname = os.path.join(self.options.tmpdir, "foo.log")
        self.restart_node(0, ["-debuglogfile=%s" % tempname])
        assert os.path.isfile(tempname)

        # check that invalid log (relative) will cause error
        self.stop_node(0)
        self.assert_start_raises_init_error(0, ["-debuglogfile=ssdksjdf/sdasdfa/sdfsdfsfd"],
                                                "Error: Could not open debug log file")

        # check that invalid log (absolute) will cause error
        self.stop_node(0)
        invalidname = os.path.join(self.options.tmpdir, "foo/foo.log")
        self.assert_start_raises_init_error(0, ["-debuglogfile=%s" % invalidname],
                                               "Error: Could not open debug log file")


if __name__ == '__main__':
    LoggingTest().main()
