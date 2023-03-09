#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test debug logging."""

import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch


class LoggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-debug=0"]]

    def relative_log_path(self, name):
        return os.path.join(self.nodes[0].chain_path, name)

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Test default log file name and -debug=0")
        default_log_path = self.relative_log_path("debug.log")
        assert os.path.isfile(default_log_path)
        assert all(v == False for v in node.logging().values())

        self.log.info("Test alternative log file name in datadir and -debugexclude={net,tor}")
        self.restart_node(0, extra_args=[
            "-debuglogfile=foo.log",
            "-debugexclude=net",
            "-debugexclude=tor",
        ])
        assert os.path.isfile(self.relative_log_path("foo.log"))
        # In addition to net and tor, leveldb/libevent/rand are excluded by the test framework.
        result = node.logging()
        for category in ["leveldb", "libevent", "net", "rand", "tor"]:
            assert not result[category]

        self.log.info("Test alternative log file name outside datadir and -debugexclude={none,qt}")
        tempname = os.path.join(self.options.tmpdir, "foo.log")
        self.restart_node(0, extra_args=[
            f"-debuglogfile={tempname}",
            "-debugexclude=none",
            "-debugexclude=qt",
        ])
        assert os.path.isfile(tempname)
        # Expect the "none" value passed in -debugexclude=[none,qt] to mean that
        # none of the categories passed with -debugexclude are excluded: neither
        # qt passed here nor leveldb/libevent/rand passed by the test framework.
        assert all(v == True for v in node.logging().values())

        self.log.info("Test -debugexclude=1/all excludes all categories")
        for all_value in ['1', 'all']:
            self.restart_node(0, extra_args=[f"-debugexclude={all_value}"])
            assert all(v == False for v in node.logging().values())

        self.log.info("Test -debugexclude with no category passed defaults to -debugexclude=1/all")
        self.restart_node(0, extra_args=["-debugexclude"])
        assert all(v == False for v in node.logging().values())

        self.log.info("Test invalid log (relative) raises")
        invdir = self.relative_log_path("foo")
        invalidname = os.path.join("foo", "foo.log")
        self.stop_node(0)
        exp_stderr = r"Error: Could not open debug log file \S+$"
        self.nodes[0].assert_start_raises_init_error([f"-debuglogfile={invalidname}"], exp_stderr, match=ErrorMatch.FULL_REGEX)
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        self.log.info("Test invalid log (relative) works after path exists")
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, [f"-debuglogfile={invalidname}"])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        self.log.info("Test invalid log (absolute) raises")
        self.stop_node(0)
        invdir = os.path.join(self.options.tmpdir, "foo")
        invalidname = os.path.join(invdir, "foo.log")
        self.nodes[0].assert_start_raises_init_error([f"-debuglogfile={invalidname}"], exp_stderr, match=ErrorMatch.FULL_REGEX)
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        self.log.info("Test invalid log (absolute) works after path exists")
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, [f"-debuglogfile={invalidname}"])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        self.log.info("Test -nodebuglogfile disables logging")
        self.stop_node(0)
        os.unlink(default_log_path)
        assert not os.path.isfile(default_log_path)
        self.start_node(0, ["-nodebuglogfile"])
        assert not os.path.isfile(default_log_path)

        # just sanity check no crash here
        self.restart_node(0, [f"-debuglogfile={os.devnull}"])

        self.log.info("Test -debug and -debugexclude raise when invalid values are passed")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-debug=abc"],
            expected_msg="Error: Unsupported logging category -debug=abc.",
            match=ErrorMatch.FULL_REGEX,
        )
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-debugexclude=abc"],
            expected_msg="Error: Unsupported logging category -debugexclude=abc.",
            match=ErrorMatch.FULL_REGEX,
        )

        self.log.info("Test -loglevel raises when invalid values are passed")
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-loglevel=abc"],
            expected_msg="Error: Unsupported global logging level -loglevel=abc. Valid values: info, debug, trace.",
            match=ErrorMatch.FULL_REGEX,
        )
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-loglevel=net:abc"],
            expected_msg="Error: Unsupported category-specific logging level -loglevel=net:abc.",
            match=ErrorMatch.PARTIAL_REGEX,
        )
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-loglevel=net:info:abc"],
            expected_msg="Error: Unsupported category-specific logging level -loglevel=net:info:abc.",
            match=ErrorMatch.PARTIAL_REGEX,
        )


if __name__ == '__main__':
    LoggingTest().main()
