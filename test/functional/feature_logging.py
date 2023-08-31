#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test debug logging."""

from os import devnull
from pathlib import Path

from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch


class LoggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def relative_log_path(self, name):
        return self.nodes[0].chain_path / name

    def run_test(self):
        # test default log file name
        default_log_path = self.relative_log_path("debug.log")
        assert default_log_path.is_file()

        # test alternative log file name in datadir
        self.restart_node(0, ["-debuglogfile=foo.log"])
        assert self.relative_log_path("foo.log").is_file()

        # test alternative log file name outside datadir
        tempname = Path(self.options.tmpdir) / "foo.log"
        self.restart_node(0, [f"-debuglogfile={tempname}"])
        assert tempname.is_file()

        # check that invalid log (relative) will cause error
        invdir = self.relative_log_path("foo")
        invfile = invdir / "foo.log"
        self.stop_node(0)
        exp_stderr = r"Error: Could not open debug log file \S+$"
        self.nodes[0].assert_start_raises_init_error([f"-debuglogfile={invfile}"], exp_stderr, match=ErrorMatch.FULL_REGEX)
        assert not invfile.is_file()

        # check that invalid log (relative) works after path exists
        self.stop_node(0)
        invdir.mkdir()
        self.start_node(0, [f"-debuglogfile={invfile}"])
        assert invfile.is_file()

        # check that invalid log (absolute) will cause error
        self.stop_node(0)
        invdir = Path(self.options.tmpdir) / "foo"
        invfile = invdir / "foo.log"
        self.nodes[0].assert_start_raises_init_error([f"-debuglogfile={invfile}"], exp_stderr, match=ErrorMatch.FULL_REGEX)
        assert not invfile.is_file()

        # check that invalid log (absolute) works after path exists
        self.stop_node(0)
        invdir.mkdir()
        self.start_node(0, [f"-debuglogfile={invfile}"])
        assert invfile.is_file()

        # check that -nodebuglogfile disables logging
        self.stop_node(0)
        default_log_path.unlink()
        assert not default_log_path.is_file()
        self.start_node(0, ["-nodebuglogfile"])
        assert not default_log_path.is_file()

        # just sanity check no crash here
        self.restart_node(0, [f"-debuglogfile={devnull}"])

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
