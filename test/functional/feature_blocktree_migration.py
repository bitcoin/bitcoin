#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test migrating a v28.2 block tree to the flat file block tree store

"""

import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class BlockTreeMigrationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.add_nodes(
            self.num_nodes,
            versions=[
                None,
                280200,
            ],
        )

    def run_test(self):
        block_tree_store_node = self.nodes[0]
        legacy_node = self.nodes[1]

        self.start_node(1)
        nblocks = 100
        self.generate(legacy_node, nblocks, sync_fun=self.no_op)
        assert_equal(legacy_node.getblockchaininfo()["blocks"], nblocks)
        self.stop_node(1)

        migrate_log = "Successfully migrated the leveldb block tree db to new block tree store."
        detect_legacy_log = "Detected legacy leveldb block tree db - removing it"

        self.log.info("Start the node with a legacy data directory to trigger migration")
        shutil.copytree(legacy_node.chain_path, block_tree_store_node.chain_path)
        with block_tree_store_node.assert_debug_log(expected_msgs=[migrate_log]):
            self.start_node(0)
        index_dir = block_tree_store_node.chain_path / "blocks" / "index"
        assert (index_dir / "headers.dat").exists()
        assert not (index_dir / "CURRENT").exists()
        assert_equal(block_tree_store_node.getblockchaininfo()["blocks"], nblocks)
        self.stop_node(0)

        self.log.info("Re-starting the node to exercise non-migration path")
        with block_tree_store_node.assert_debug_log(expected_msgs=[], unexpected_msgs=[migrate_log]):
            self.start_node(0)
        assert_equal(block_tree_store_node.getblockchaininfo()["blocks"], nblocks)
        self.stop_node(0)

        self.log.info("A corrupt legacy block index fails the migration and kills the node")
        self.cleanup_folder(block_tree_store_node.chain_path)
        shutil.copytree(legacy_node.chain_path, block_tree_store_node.chain_path)
        manifests = list(block_tree_store_node.chain_path.glob("blocks/index/MANIFEST*"))
        assert_equal(len(manifests), 1)
        manifests[0].unlink()
        with block_tree_store_node.assert_debug_log(expected_msgs=["Error opening block database"]):
            block_tree_store_node.assert_start_raises_init_error()

        self.log.info("Restarting with -reindex will create a block tree store and remove the corrupt leveldb data")
        with block_tree_store_node.assert_debug_log(expected_msgs=[detect_legacy_log]):
            self.start_node(0, extra_args=["-reindex"])
        self.wait_until(lambda: block_tree_store_node.getblockchaininfo()["blocks"] == nblocks)
        assert (index_dir / "headers.dat").exists()
        assert not (index_dir / "CURRENT").exists()
        self.stop_node(0)

        self.log.info("A corrupt legacy block index prompts the user to reindex")
        self.cleanup_folder(block_tree_store_node.chain_path)
        shutil.copytree(legacy_node.chain_path, block_tree_store_node.chain_path)
        manifests = list(block_tree_store_node.chain_path.glob("blocks/index/MANIFEST*"))
        assert_equal(len(manifests), 1)
        manifests[0].unlink()
        with block_tree_store_node.assert_debug_log(expected_msgs=[detect_legacy_log]):
            self.start_node(0, extra_args=["-test=reindex_after_failure_noninteractive_yes"])
        self.wait_until(lambda: block_tree_store_node.getblockchaininfo()["blocks"] == nblocks)
        assert (index_dir / "headers.dat").exists()
        assert not (index_dir / "CURRENT").exists()
        self.stop_node(0)

        self.log.info("Recreate a pruned legacy node and test its migration")
        self.start_node(1, extra_args=["-prune=1", "-fastprune"])
        self.generate(legacy_node, 500, sync_fun=self.no_op)
        legacy_node.pruneblockchain(400)
        self.stop_node(1)
        self.cleanup_folder(block_tree_store_node.chain_path)
        shutil.copytree(legacy_node.chain_path, block_tree_store_node.chain_path)
        with block_tree_store_node.assert_debug_log(expected_msgs=[migrate_log, "Loading block index db: Block files have previously been pruned"]):
            block_tree_store_node.assert_start_raises_init_error()
        with block_tree_store_node.assert_debug_log(expected_msgs=["Loading block index db: Block files have previously been pruned"]):
            self.start_node(0, extra_args=["-prune=1"])
        self.stop_node(0)

if __name__ == '__main__':
    BlockTreeMigrationTest(__file__).main()
