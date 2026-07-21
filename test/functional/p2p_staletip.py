#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test stale-tip tracking."""

from test_framework.test_framework import BitcoinTestFramework


class P2PStaleTipTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-staletips=headers"]]

    def run_test(self):
        self.test_staletip_option_validation()

    def test_staletip_option_validation(self):
        self.log.info("Test staletip option validation")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=["-staletips=bogus"],
            expected_msg="Error: Invalid value for -staletips=<mode>: 'bogus'. Expected one of: none, headers.",
        )
        for valid_arg in ["-staletips=none", "-nostaletips", "-staletips=headers"]:
            self.start_node(0, extra_args=[valid_arg])
            self.stop_node(0)
        self.start_node(0)


if __name__ == "__main__":
    P2PStaleTipTest(__file__).main()
