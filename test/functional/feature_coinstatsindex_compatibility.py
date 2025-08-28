#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coinstatsindex across node versions.

This test may be removed some time after v29 has reached end of life.
"""

import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class CoinStatsIndexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False
        self.extra_args = [["-coinstatsindex"],["-coinstatsindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(
            self.num_nodes,
            extra_args=self.extra_args,
            versions=[
                None,
                280200,
            ],
        )
        self.start_nodes()

    def run_test(self):
        self._test_coin_stats_index_compatibility()

    def _test_coin_stats_index_compatibility(self):
        node = self.nodes[0]
        legacy_node = self.nodes[1]
        for n in self.nodes:
            self.wait_until(lambda: n.getindexinfo()['coinstatsindex']['synced'] is True)

        self.log.info("Test that gettxoutsetinfo() output is consistent between the different index versions")
        res0 = node.gettxoutsetinfo('muhash')
        res1 = legacy_node.gettxoutsetinfo('muhash')
        assert_equal(res1, res0)

        self.log.info("Test that gettxoutsetinfo() output is consistent for the new index running on a datadir with the old version")
        self.stop_nodes()
        shutil.rmtree(node.chain_path / "indexes" / "coinstatsindex")
        shutil.copytree(legacy_node.chain_path / "indexes" / "coinstats", node.chain_path / "indexes" / "coinstats")
        old_version_path = node.chain_path / "indexes" / "coinstats"
        msg = f'[warning] Old version of coinstatsindex found at {old_version_path}. This folder can be safely deleted unless you plan to downgrade your node to version 29 or lower.'
        with node.assert_debug_log(expected_msgs=[msg]):
            self.start_node(0, ['-coinstatsindex'])
        self.wait_until(lambda: node.getindexinfo()['coinstatsindex']['synced'] is True)
        res2 = node.gettxoutsetinfo('muhash')
        assert_equal(res2, res0)


if __name__ == '__main__':
    CoinStatsIndexTest(__file__).main()
