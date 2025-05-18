#!/usr/bin/env python3
# Copyright (c) 2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that unsupported utxo db causes an init error.

Previous releases are required by this test, see test/README.md.
"""

import shutil

from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import assert_equal


class UnsupportedUtxoDbTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.add_nodes(
            self.num_nodes,
            versions=[
                140300,  # Last release with previous utxo db format
                None,  # For MiniWallet, without migration code
            ],
        )

    def run_test(self):
        self.log.info("Create previous version (v0.14.3) utxo db")
        self.start_node(0)
        block = self.generate(self.nodes[0], 1, sync_fun=self.no_op)[-1]
        assert_equal(self.nodes[0].getbestblockhash(), block)
        assert_equal(self.nodes[0].gettxoutsetinfo()["total_amount"], 50)
        self.stop_nodes()

        self.log.info("Check init error")
        legacy_utxos_dir = self.nodes[0].chain_path / "chainstate"
        legacy_blocks_dir = self.nodes[0].blocks_path
        recent_utxos_dir = self.nodes[1].chain_path / "chainstate"
        recent_blocks_dir = self.nodes[1].blocks_path
        shutil.copytree(legacy_utxos_dir, recent_utxos_dir)
        shutil.copytree(legacy_blocks_dir, recent_blocks_dir)
        self.nodes[1].assert_start_raises_init_error(
            expected_msg="Error: Unsupported chainstate database format found. "
            "Please restart with -reindex-chainstate. "
            "This will rebuild the chainstate database.",
        )

        self.log.info("Drop legacy utxo db")
        self.start_node(1, extra_args=["-reindex-chainstate"])
        assert_equal(self.nodes[1].getbestblockhash(), block)
        assert_equal(self.nodes[1].gettxoutsetinfo()["total_amount"], 50)


if __name__ == "__main__":
    UnsupportedUtxoDbTest(__file__).main()
