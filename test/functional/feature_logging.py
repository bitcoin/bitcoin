#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test debug logging."""

import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_equal


class LoggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def relative_log_path(self, name):
        return os.path.join(self.nodes[0].datadir, self.chain, name)

    def run_test(self):
        # test default log file name
        default_log_path = self.relative_log_path("debug.log")
        assert os.path.isfile(default_log_path)

        # test alternative log file name in datadir
        self.restart_node(0, ["-debuglogfile=foo.log"])
        assert os.path.isfile(self.relative_log_path("foo.log"))

        # test alternative log file name outside datadir
        tempname = os.path.join(self.options.tmpdir, "foo.log")
        self.restart_node(0, ["-debuglogfile=%s" % tempname])
        assert os.path.isfile(tempname)

        # check that invalid log (relative) will cause error
        invdir = self.relative_log_path("foo")
        invalidname = os.path.join("foo", "foo.log")
        self.stop_node(0)
        exp_stderr = r"Error: Could not open debug log file \S+$"
        self.nodes[0].assert_start_raises_init_error(["-debuglogfile=%s" % (invalidname)], exp_stderr, match=ErrorMatch.FULL_REGEX)
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
        self.nodes[0].assert_start_raises_init_error(["-debuglogfile=%s" % invalidname], exp_stderr, match=ErrorMatch.FULL_REGEX)
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, ["-debuglogfile=%s" % (invalidname)])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that -nodebuglogfile disables logging
        self.stop_node(0)
        os.unlink(default_log_path)
        assert not os.path.isfile(default_log_path)
        self.start_node(0, ["-nodebuglogfile"])
        assert not os.path.isfile(default_log_path)

        # just sanity check no crash here
        self.restart_node(0, ["-debuglogfile=%s" % os.devnull])

        # debug log file rotation
        self.log.info("Log file rotation")
        self.stop_node(0)
        self.start_node(0, ["-debuglogfile=altname-debug.log", "-debuglogrotatekeep=2", "-debugloglimit=1"])
        self.log.info("Generating many log messages")
        # Each getblockcount RPC logs 251 bytes, so 13k of them generates
        # more than 3 MB of logging, which will test that one of the
        # rotated log files gets deleted.
        for _ in range(13000):
            self.nodes[0].getblockcount()
        n_debugfiles = 0
        with os.scandir(os.path.join(self.nodes[0].datadir, 'regtest', 'debug-rotated')) as it:
            for entry in it:
                # rotated log files' names should be like 2021-07-09T014959Z
                assert len(entry.name) == len("2021-07-09T014959Z")
                assert_equal(entry.name[10], 'T')
                assert_equal(entry.name[17], 'Z')
                # rotated files should have sizes slightly more than 1 MB
                assert entry.stat().st_size > 1000000
                assert entry.stat().st_size < int(1000000 * 1.01)
                n_debugfiles += 1
        # There should be and two rotated files
        assert_equal(n_debugfiles, 2)


if __name__ == '__main__':
    LoggingTest().main()
