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
        invdir = os.path.join(self.nodes[0].datadir, "regtest", "foo")
        invalidname = os.path.join("foo", "foo.log")
        self.stop_node(0)
        self.assert_start_raises_init_error(0, ["-debuglogfile=%s" % (invalidname)],
                                                "Error: Could not open debug log file")
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (relative) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, ["-debuglogfile=%s" % (invalidname)])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) will cause error
        self.stop_node(0)
        invdir = os.path.join(self.options.tmpdir, "foo")
        invalidname = os.path.join(invdir, "foo.log")
        self.assert_start_raises_init_error(0, ["-debuglogfile=%s" % invalidname],
                                               "Error: Could not open debug log file")
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, ["-debuglogfile=%s" % (invalidname)])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))


if __name__ == '__main__':
    LoggingTest().main()
