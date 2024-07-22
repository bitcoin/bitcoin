#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test wallet import on pruned node."""

from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block
)
from test_framework.blocktools import create_coinbase
from test_framework.test_framework import BitcoinTestFramework

from test_framework.script import (
    CScript,
    OP_RETURN,
    OP_TRUE,
)

class WalletPruningTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.wallet_names = []
        self.extra_args = [
            [], # node dedicated to mining
            ['-prune=550'], # node dedicated to testing pruning
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def mine_large_blocks(self, node, n):
        # Get the block parameters for the first block
        best_block = node.getblockheader(node.getbestblockhash())
        height = int(best_block["height"]) + 1
        self.nTime = max(self.nTime, int(best_block["time"])) + 1
        previousblockhash = int(best_block["hash"], 16)
        big_script = CScript([OP_RETURN] + [OP_TRUE] * 950000)
        # Set mocktime to accept all future blocks
        for i in self.nodes:
            if i.running:
                i.setmocktime(self.nTime + 600 * n)
        for _ in range(n):
            block = create_block(hashprev=previousblockhash, ntime=self.nTime, coinbase=create_coinbase(height, script_pubkey=big_script))
            block.solve()

            # Submit to the node
            node.submitblock(block.serialize().hex())

            previousblockhash = block.sha256
            height += 1

            # Simulate 10 minutes of work time per block
            # Important for matching a timestamp with a block +- some window
            self.nTime += 600
        self.sync_all()

    def test_wallet_import_pruned(self, wallet_name):
        self.log.info("Make sure we can import wallet when pruned and required blocks are still available")

        wallet_file = wallet_name + ".dat"
        wallet_birthheight = self.get_birthheight(wallet_file)

        # Verify that the block at wallet's birthheight is available at the pruned node
        self.nodes[1].getblock(self.nodes[1].getblockhash(wallet_birthheight))

        # Import wallet into pruned node
        self.nodes[1].createwallet(wallet_name="wallet_pruned", descriptors=False, load_on_startup=True)
        self.nodes[1].importwallet(self.nodes[0].datadir_path / wallet_file)

        # Make sure that prune node's wallet correctly accounts for balances
        assert_equal(self.nodes[1].getbalance(), self.nodes[0].getbalance())

        self.log.info("- Done")

    def test_wallet_import_pruned_with_missing_blocks(self, wallet_name):
        self.log.info("Make sure we cannot import wallet when pruned and required blocks are not available")

        wallet_file = wallet_name + ".dat"
        wallet_birthheight = self.get_birthheight(wallet_file)

        # Verify that the block at wallet's birthheight is not available at the pruned node
        assert_raises_rpc_error(-1, "Block not available (pruned data)", self.nodes[1].getblock, self.nodes[1].getblockhash(wallet_birthheight))

        # Make sure wallet cannot be imported because of missing blocks
        # This will try to rescan blocks `TIMESTAMP_WINDOW` (2h) before the wallet birthheight.
        # There are 6 blocks an hour, so 11 blocks (excluding birthheight).
        assert_raises_rpc_error(-4, f"Pruned blocks from height {wallet_birthheight - 11} required to import keys. Use RPC call getblockchaininfo to determine your pruned height.", self.nodes[1].importwallet, self.nodes[0].datadir_path / wallet_file)
        self.log.info("- Done")

    def get_birthheight(self, wallet_file):
        """Gets birthheight of a wallet on node0"""
        with open(self.nodes[0].datadir_path / wallet_file, 'r', encoding="utf8") as f:
            for line in f:
                if line.startswith('# * Best block at time of backup'):
                    wallet_birthheight = int(line.split(' ')[9])
                    return wallet_birthheight

    def has_block(self, block_index):
        """Checks if the pruned node has the specific blk0000*.dat file"""
        return (self.nodes[1].blocks_path / f"blk{block_index:05}.dat").is_file()

    def create_wallet(self, wallet_name, *, unload=False):
        """Creates and dumps a wallet on the non-pruned node0 to be later import by the pruned node"""
        self.nodes[0].createwallet(wallet_name=wallet_name, descriptors=False, load_on_startup=True)
        self.nodes[0].dumpwallet(self.nodes[0].datadir_path / f"{wallet_name}.dat")
        if (unload):
            self.nodes[0].unloadwallet(wallet_name)

    def run_test(self):
        self.nTime = 0
        self.log.info("Warning! This test requires ~1.3GB of disk space")

        self.log.info("Generating a long chain of blocks...")

        # A blk*.dat file is 128MB
        # Generate 250 light blocks
        self.generate(self.nodes[0], 250)
        # Generate 50MB worth of large blocks in the blk00000.dat file
        self.mine_large_blocks(self.nodes[0], 50)

        # Create a wallet which birth's block is in the blk00000.dat file
        wallet_birthheight_1 = "wallet_birthheight_1"
        assert_equal(self.has_block(1), False)
        self.create_wallet(wallet_birthheight_1, unload=True)

        # Generate enough large blocks to reach pruning disk limit
        # Not pruning yet because we are still below PruneAfterHeight
        self.mine_large_blocks(self.nodes[0], 600)
        self.log.info("- Long chain created")

        # Create a wallet with birth height > wallet_birthheight_1
        wallet_birthheight_2 = "wallet_birthheight_2"
        self.create_wallet(wallet_birthheight_2)

        # Fund wallet to later verify that importwallet correctly accounts for balances
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, self.nodes[0].getnewaddress(), sync_fun=self.no_op)

        # We've reached pruning storage & height limit but
        # pruning doesn't run until another chunk (blk*.dat file) is allocated.
        # That's why we are generating another 5 large blocks
        self.mine_large_blocks(self.nodes[0], 5)

        # blk00000.dat file is now pruned from node1
        assert_equal(self.has_block(0), False)

        self.test_wallet_import_pruned(wallet_birthheight_2)
        self.test_wallet_import_pruned_with_missing_blocks(wallet_birthheight_1)

if __name__ == '__main__':
    WalletPruningTest(__file__).main()
