#!/usr/bin/env python3
"""Test wallet staking enable and disable behavior."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class WalletStakingToggleTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.generatetoaddress(200, addr)

        assert node.walletstaking(True)
        self.wait_until(lambda: node.getstakinginfo()["staking"])
        assert not node.walletstaking(False)
        self.wait_until(lambda: not node.getstakinginfo()["staking"])
        assert node.walletstaking(True)
        self.wait_until(lambda: node.getstakinginfo()["staking"])


if __name__ == "__main__":
    WalletStakingToggleTest(__file__).main()
