#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -uacomment option."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class UacommentTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("test multiple -uacomment")
        test_uacomment = self.nodes[0].getnetworkinfo()["subversion"][-12:-1]
        assert_equal(test_uacomment, "(testnode0)")

        self.restart_node(0, ["-uacomment=foo"])
        foo_uacomment = self.nodes[0].getnetworkinfo()["subversion"][-17:-1]
        assert_equal(foo_uacomment, "(testnode0; foo)")

        self.log.info("test -uacomment max length")
        self.stop_node(0)
        expected = "exceeds maximum length (256). Reduce the number or size of uacomments."
        self.assert_start_raises_init_error(0, ["-uacomment=" + 'a' * 256], expected)

        self.log.info("test -uacomment unsafe characters")
        for unsafe_char in ['/', ':', '(', ')']:
            expected = "User Agent comment (" + unsafe_char + ") contains unsafe characters"
            self.assert_start_raises_init_error(0, ["-uacomment=" + unsafe_char], expected)

if __name__ == '__main__':
    UacommentTest().main()
