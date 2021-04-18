#!/usr/bin/env python3
# Copyright (c) 2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test generate RPC."""

from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class RPCGenerateTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        message = (
            "generate\n"
            "has been replaced by the -generate "
            "cli option. Refer to -help for more information."
        )

        self.log.info("Test rpc generate raises with message to use cli option")
        assert_raises_rpc_error(-32601, message, self.nodes[0].rpc.generate)

        self.log.info("Test rpc generate help prints message to use cli option")
        assert_equal(message, self.nodes[0].help("generate"))

        self.log.info("Test rpc generate is a hidden command not discoverable in general help")
        assert message not in self.nodes[0].help()


if __name__ == "__main__":
    RPCGenerateTest().main()
