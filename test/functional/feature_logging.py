#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test debug logging."""

import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.p2p import P2PInterface
from test_framework.test_node import ErrorMatch
from test_framework.util import assert_raises_rpc_error


class LoggingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def relative_log_path(self, name):
        return os.path.join(self.nodes[0].chain_path, name)

    def run_test(self):
        # test default log file name
        default_log_path = self.relative_log_path("debug.log")
        assert os.path.isfile(default_log_path)

        # test alternative log file name in datadir
        self.restart_node(0, ["-debuglogfile=foo.log"])
        assert os.path.isfile(self.relative_log_path("foo.log"))

        # test alternative log file name outside datadir
        tempname = os.path.join(self.options.tmpdir, "foo.log")
        self.restart_node(0, [f"-debuglogfile={tempname}"])
        assert os.path.isfile(tempname)

        # check that invalid log (relative) will cause error
        invdir = self.relative_log_path("foo")
        invalidname = os.path.join("foo", "foo.log")
        self.stop_node(0)
        exp_stderr = "Error: Could not open debug log file "
        self.nodes[0].assert_start_raises_init_error([f"-debuglogfile={invalidname}"], exp_stderr, match=ErrorMatch.PARTIAL_REGEX)
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (relative) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, [f"-debuglogfile={invalidname}"])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) will cause error
        self.stop_node(0)
        invdir = os.path.join(self.options.tmpdir, "foo")
        invalidname = os.path.join(invdir, "foo.log")
        self.nodes[0].assert_start_raises_init_error([f"-debuglogfile={invalidname}"], exp_stderr, match=ErrorMatch.PARTIAL_REGEX)
        assert not os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that invalid log (absolute) works after path exists
        self.stop_node(0)
        os.mkdir(invdir)
        self.start_node(0, [f"-debuglogfile={invalidname}"])
        assert os.path.isfile(os.path.join(invdir, "foo.log"))

        # check that -nodebuglogfile disables logging
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
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-loglevel=net:trace,debug"],
            expected_msg="global level must be specified before any category-specific levels",
            match=ErrorMatch.PARTIAL_REGEX,
        )

        self.log.info("Test -loglevel enables categories without -debug")
        # -nologlevel and -nodebug clear the test framework's -loglevel=trace and -debug for a clean baseline
        self.restart_node(0, ['-nologlevel', '-nodebug', '-loglevel=debug'])
        logging = self.nodes[0].logging()
        assert logging['net']
        assert logging['http']

        self.restart_node(0, ['-nologlevel', '-nodebug', '-loglevel=net:trace'])
        logging = self.nodes[0].logging()
        assert logging['net']
        assert not logging['http']

        self.log.info("Test -loglevel comma-separated syntax")
        self.restart_node(0, ['-nologlevel', '-nodebug', '-loglevel=debug,net:trace'])
        logging = self.nodes[0].logging()
        assert logging['net']
        assert logging['http']  # enabled by global debug

        self.log.info("Test -loglevel precedence over -debug and -debugexclude")
        # -loglevel is processed after -debug regardless of command-line order
        self.restart_node(0, ['-loglevel=info', '-debug=net'])
        assert not self.nodes[0].logging()['net']

        # -debugexclude is processed after -loglevel regardless of command-line order
        self.restart_node(0, ['-debugexclude=net', '-loglevel=debug'])
        logging = self.nodes[0].logging()
        assert not logging['net']
        assert logging['http']

        self.log.info("Test that -nodebug,-debug=0,-debug=none clear previously specified debug options")
        disable_debug_options = [
            '-debug=0',
            '-debug=none',
            '-nodebug'
        ]

        for disable_debug_opt in disable_debug_options:
            # Every category before disable_debug_opt will be ignored, including the invalid 'abc'.
            # -nologlevel clears the test framework's -loglevel=trace which would otherwise
            # enable all categories and interfere with this test.
            self.restart_node(0, ['-nologlevel', '-debug=http', '-debug=abc', disable_debug_opt, '-debug=rpc', '-debug=net'])
            logging = self.nodes[0].logging()
            assert not logging['http']
            assert 'abc' not in logging
            assert logging['rpc']
            assert logging['net']

        self.log.info("Test -logips formatting in net logs")
        self.restart_node(0, ['-debug=net', '-logips=1'])
        with self.nodes[0].assert_debug_log(["peer=0, peeraddr="]):
            p2p = self.nodes[0].add_p2p_connection(P2PInterface())
            p2p.wait_for_verack()
            self.nodes[0].disconnect_p2ps()

        self.log.info("Test loglevel RPC")
        # Start with a clean baseline: all categories at info (disabled)
        self.restart_node(0, ['-nologlevel', '-nodebug'])

        # Read-only call returns all categories with level strings
        levels = self.nodes[0].loglevel()
        assert isinstance(levels, dict)
        assert 'net' in levels
        assert 'http' in levels
        assert all(v in ('trace', 'debug', 'info') for v in levels.values())

        # All categories should be at 'info' (disabled) with our clean baseline
        assert all(v == 'info' for v in levels.values()), f"Expected all info, got: {levels}"

        # Set a single category to trace
        result = self.nodes[0].loglevel(net='trace')
        assert result['net'] == 'trace'
        assert result['http'] == 'info'
        # Cross-check with logging RPC: net should be active (trace < info), http inactive
        assert self.nodes[0].logging()['net']
        assert not self.nodes[0].logging()['http']

        # Set a single category to debug
        result = self.nodes[0].loglevel(net='debug')
        assert result['net'] == 'debug'

        # Set a category back to info (disable it)
        result = self.nodes[0].loglevel(net='info')
        assert result['net'] == 'info'
        assert not self.nodes[0].logging()['net']

        # Set all categories to debug using the positional shorthand
        result = self.nodes[0].loglevel('debug')
        assert all(v == 'debug' for v in result.values()), f"Expected all debug, got: {result}"
        assert self.nodes[0].logging()['net']
        assert self.nodes[0].logging()['http']

        # "all" named argument combined with per-category override
        result = self.nodes[0].loglevel(all='info', net='trace')
        assert result['net'] == 'trace'
        assert result['http'] == 'info'

        # Set all categories to trace, then override net back to info
        result = self.nodes[0].loglevel(all='trace', net='info')
        assert result['net'] == 'info'
        assert result['http'] == 'trace'

        # Invalid category raises an error
        assert_raises_rpc_error(-8, "Unknown named parameter notacategory", self.nodes[0].loglevel, **{'notacategory': 'debug'})

        # Invalid level raises an error
        assert_raises_rpc_error(-8, "unknown log level", self.nodes[0].loglevel, net='verbose')

        # loglevel and logging RPCs stay consistent
        self.nodes[0].loglevel(all='info', rpc='debug')
        logging_result = self.nodes[0].logging()
        levels_result = self.nodes[0].loglevel()
        for cat, active in logging_result.items():
            expected_active = levels_result[cat] != 'info'
            assert active == expected_active, f"Inconsistency for {cat}: logging={active}, loglevel={levels_result[cat]}"

if __name__ == '__main__':
    LoggingTest(__file__).main()
