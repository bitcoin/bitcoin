#!/usr/bin/env python3
"""Test PoS activation gating rejects PoW blocks after the activation height."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class PosActivationGatingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-posactivationheight=2"]]

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(1, addr)
        assert_equal(node.getblockcount(), 1)
        assert_raises_rpc_error(-1, "bad-pow", node.generatetoaddress, 1, addr)
        assert_equal(node.getblockcount(), 1)


if __name__ == "__main__":
    PosActivationGatingTest(__file__).main()
