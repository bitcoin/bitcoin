#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that unsupported utxo db causes an init error.

Previous releases are required by this test, see test/README.md.
"""

import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class UnsupportedUtxoDbTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.add_nodes(
            self.num_nodes,
            versions=[
                120105,  # Last release with previous utxo db format
                None,    # For MiniWallet, without migration code
            ],
        )

    def run_test(self):
        self.log.info("Create previous version (v0.12.1.5) utxo db")
        self.start_node(0)
        # 'generatetoaddress' was introduced in v0.12.3, use legacy 'generate' call
        block = self.nodes[0].rpc.generate(1)[-1]
        assert_equal(self.nodes[0].getbestblockhash(), block)
        assert_equal(self.nodes[0].gettxoutsetinfo()["total_amount"], 500)
        self.stop_nodes()

        self.log.info("Check init error")
        legacy_utxos_dir = self.nodes[0].chain_path / "chainstate"
        legacy_blocks_dir = self.nodes[0].chain_path / "blocks"
        recent_utxos_dir = self.nodes[1].chain_path / "chainstate"
        recent_blocks_dir = self.nodes[1].chain_path / "blocks"
        shutil.copytree(legacy_utxos_dir, recent_utxos_dir)
        shutil.copytree(legacy_blocks_dir, recent_blocks_dir)
        self.nodes[1].assert_start_raises_init_error(
            expected_msg="Error: Unsupported chainstate database format found. "
            "Please restart with -reindex-chainstate. "
            "This will rebuild the chainstate database.",
        )

        self.log.info("Drop legacy utxo db")
        self.start_node(1, extra_args=["-reindex-chainstate", "-txindex=0", "-blockfilterindex=0"])
        assert_equal(self.nodes[1].getbestblockhash(), block)
        assert_equal(self.nodes[1].gettxoutsetinfo()["total_amount"], 500)


if __name__ == "__main__":
    UnsupportedUtxoDbTest().main()
