#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test for assumeutxo wallet related behavior.
See feature_assumeutxo.py for background.
"""
from shutil import rmtree
from test_framework.address import address_to_scriptpubkey
from test_framework.blocktools import create_block, create_coinbase
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import COIN
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    dumb_sync_blocks,
    ensure_for,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import get_generate_key

START_HEIGHT = 199
SNAPSHOT_BASE_HEIGHT = 299
FINAL_HEIGHT = 399


class AssumeutxoTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        """Use the pregenerated, deterministic chain up to height 199."""
        self.num_nodes = 4
        self.rpc_timeout = 120
        self.extra_args = [
            [],
            [],
            [],
            ["-fastprune", "-prune=1"],
        ]

    def setup_network(self):
        """Start with the nodes disconnected so that one can generate a snapshot
        including blocks the other hasn't yet seen."""
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()

    def import_descriptor(self, node, wallet_name, key, timestamp):
        import_request = [{"desc": descsum_create("pkh(" + key.pubkey + ")"),
                           "timestamp": timestamp,
                           "label": "Descriptor import test"}]
        wrpc = node.get_wallet_rpc(wallet_name)
        return wrpc.importdescriptors(import_request)

    def validate_snapshot_import(self, node, loaded, base_hash, expect_start_height=True):
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        normal, snapshot = node.getchainstates()["chainstates"]
        if expect_start_height:
            assert_equal(normal['blocks'], START_HEIGHT)
        assert 'snapshot_blockhash' not in normal
        assert_equal(normal['validated'], True)
        assert_equal(snapshot['blocks'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(snapshot['snapshot_blockhash'], base_hash)
        assert_equal(snapshot['validated'], False)

        assert_equal(node.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

    def complete_background_validation(self, node):
        self.connect_nodes(0, node.index)

        # Ensuring snapshot chain syncs to tip
        self.wait_until(lambda: node.getchainstates()['chainstates'][-1]['blocks'] == FINAL_HEIGHT)
        self.sync_blocks(nodes=(self.nodes[0], node))

        # Ensuring background validation completes
        self.wait_until(lambda: len(node.getchainstates()['chainstates']) == 1)

    def test_backup_during_background_sync_pruned_node(self, n3, dump_output, expected_error_message):
        self.log.info("Backup from the snapshot height can be loaded during background sync (pruned node)")
        loaded = n3.loadtxoutset(dump_output['path'])
        assert_greater_than(n3.pruneblockchain(START_HEIGHT), 0)
        self.validate_snapshot_import(n3, loaded, dump_output['base_hash'])
        n3.restorewallet("w", "backup_w.dat")
        # Balance of w wallet is still 0 because n3 has not synced yet
        assert_equal(n3.getbalance(), 0)

        n3.unloadwallet("w")
        self.log.info("Backup from before the snapshot height can't be loaded during background sync (pruned node)")
        assert_raises_rpc_error(-4, expected_error_message, n3.restorewallet, "w2", "backup_w2.dat")

    def test_restore_wallet_pruneheight(self, n3):
        self.log.info("Ensuring wallet can't be restored from a backup that was created before the pruneheight (pruned node)")
        self.complete_background_validation(n3)
        # After background sync, pruneheight is reset to 0, so mine 200 blocks
        # and prune the chain again
        self.generate(n3, nblocks=200, sync_fun=self.no_op)
        assert_equal(n3.pruneblockchain(FINAL_HEIGHT), 298)  # 298 is the height of the last block pruned (pruneheight 299)
        error_message = "Wallet loading failed. Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of a pruned node)"
        # This backup (backup_w2.dat) was created at height 199, so it can't be restored in a node with a pruneheight of 299
        assert_raises_rpc_error(-4, error_message, n3.restorewallet, "w2_pruneheight", "backup_w2.dat")

        self.log.info("Ensuring wallet can be restored from a backup that was created at the pruneheight (pruned node)")
        # This backup (backup_w.dat) was created at height 299, so it can be restored in a node with a pruneheight of 299
        n3.restorewallet("w_alt", "backup_w.dat")
        # Check balance of w_alt wallet
        w_alt = n3.get_wallet_rpc("w_alt")
        assert_equal(w_alt.getbalance(), 34)

    def test_wallet_reorg_during_background_sync(self, dump_output):
        self.log.info("Ensure wallet balances stay consistent across a reorg during assumeutxo background sync")
        n0 = self.nodes[0]
        n3 = self.nodes[3]

        # Restart pruned node fresh to ensure snapshot has more work than active chainstate
        self.stop_node(n3.index)
        rmtree(n3.chain_path)
        self.start_node(n3.index, extra_args=self.extra_args[n3.index])
        n3.setmocktime(n0.getblockheader(n0.getbestblockhash())['time'])

        # Feed headers so the snapshot base block is known
        for i in range(1, SNAPSHOT_BASE_HEIGHT + 1):
            header = n0.getblock(n0.getblockhash(i), 0)
            n3.submitheader(header)

        loaded = n3.loadtxoutset(dump_output['path'])
        self.validate_snapshot_import(n3, loaded, dump_output['base_hash'], expect_start_height=False)

        # Restore wallet created at snapshot height it should be empty before sync
        n3.restorewallet("w_reorg", "backup_w.dat")
        w_reorg = n3.get_wallet_rpc("w_reorg")
        assert_equal(w_reorg.getbalance(), 0)

        # Begin background sync from n0
        self.connect_nodes(n3.index, n0.index)
        self.wait_until(lambda: n3.getchainstates()['chainstates'][-1]['blocks'] > SNAPSHOT_BASE_HEIGHT)

        # Fund the wallet on the main chain after the snapshot so the payment is orphaned by the reorg
        payment_addr = w_reorg.getnewaddress()
        pay_amount = 5
        funding_wallet = n0.get_wallet_rpc("w")
        txid = funding_wallet.sendtoaddress(payment_addr, pay_amount)
        self.generate(n0, nblocks=2, sync_fun=self.no_op)
        # Ensure the assumeutxo node receives the blocks with the payment
        self.sync_blocks(nodes=(n0, n3))
        self.wait_until(lambda: n3.getblockcount() == n0.getblockcount())
        # Payment is post snapshot and intended to be orphaned by the reorg ensure seen first
        self.wait_until(lambda: w_reorg.gettransaction(txid)['confirmations'] >= 1, timeout=180)
        self.wait_until(lambda: w_reorg.getbalance() >= pay_amount, timeout=30)

        # Build an alternative chain on n0 that excludes the post-snapshot wallet payment
        fork_point = SNAPSHOT_BASE_HEIGHT
        orig_height = n0.getblockcount()
        orig_chainwork = int(n0.getblockchaininfo()['chainwork'], 16)
        self.disconnect_nodes(n3.index, n0.index)

        fork_block_hash = int(n0.getblockhash(fork_point), 16)
        tip_time = n0.getblockheader(n0.getbestblockhash())['time']
        fork_blocks = []
        for i in range((orig_height - fork_point) + 2):
            block = create_block(
                hashprev=fork_block_hash,
                coinbase=create_coinbase(height=fork_point + 1 + i),
                ntime=tip_time + 1 + i,
            )
            block.solve()
            fork_blocks.append(block)
            fork_block_hash = block.hash_int

        for block in fork_blocks:
            assert n0.submitblock(block.serialize().hex()) in (None, 'inconclusive')
        assert_equal(n0.getbestblockhash(), fork_blocks[-1].hash_hex)

        assert_greater_than(int(n0.getblockchaininfo()['chainwork'], 16), orig_chainwork)
        n0.setmocktime(n3.getblockheader(n3.getbestblockhash())['time'])
        self.connect_nodes(n3.index, n0.index)

        # Wait for n3 to follow the new chain and finish background validation
        self.wait_until(lambda: n3.getblockcount() == n0.getblockcount())
        self.wait_until(lambda: len(n3.getchainstates()['chainstates']) == 1)

        # Wallet should reflect the reorged chain (post-snapshot payment was orphaned).
        assert_equal(w_reorg.getbalance(), 0)

    def run_test(self):
        """
        Bring up four (disconnected) nodes:
        - n0: mine some blocks and create a UTXO snapshot
        - n1: load the snapshot and test loading a wallet backup and descriptors during and after background sync
        - n2: load the snapshot and check the wallet balance during background sync
        - n3: load the snapshot, prune the chain, and test loading a wallet backup during and after background sync
        """
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        n2 = self.nodes[2]
        n3 = self.nodes[3]

        self.mini_wallet = MiniWallet(n0)

        # Mock time for a deterministic chain
        for n in self.nodes:
            n.setmocktime(n.getblockheader(n.getbestblockhash())['time'])

        # Create a wallet that we will create a backup for later (at snapshot height)
        n0.createwallet('w')
        w = n0.get_wallet_rpc("w")
        w_address = w.getnewaddress()

        # Create another wallet and backup now (before snapshot height)
        n0.createwallet('w2')
        w2 = n0.get_wallet_rpc("w2")
        w2_address = w2.getnewaddress()
        w2.backupwallet("backup_w2.dat")

        # Generate a series of blocks that `n0` will have in the snapshot,
        # but that n1 doesn't yet see. In order for the snapshot to activate,
        # though, we have to ferry over the new headers to n1 so that it
        # isn't waiting forever to see the header of the snapshot's base block
        # while disconnected from n0.
        for i in range(100):
            if i % 3 == 0:
                self.mini_wallet.send_self_transfer(from_node=n0)
            self.generate(n0, nblocks=1, sync_fun=self.no_op)
            newblock = n0.getblock(n0.getbestblockhash(), 0)

            # make n1 aware of the new header, but don't give it the block.
            n1.submitheader(newblock)
            n2.submitheader(newblock)
            n3.submitheader(newblock)
        # Ensure everyone is seeing the same headers.
        for n in self.nodes:
            assert_equal(n.getblockchaininfo()[
                         "headers"], SNAPSHOT_BASE_HEIGHT)

        # This backup is created at the snapshot height, so it's
        # not part of the background sync anymore
        w.backupwallet("backup_w.dat")

        self.log.info("-- Testing assumeutxo")

        assert_equal(n0.getblockcount(), SNAPSHOT_BASE_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)

        self.log.info(
            f"Creating a UTXO snapshot at height {SNAPSHOT_BASE_HEIGHT}")
        dump_output = n0.dumptxoutset('utxos.dat', "latest")

        assert_equal(
            dump_output['txoutset_hash'],
            "d2b051ff5e8eef46520350776f4100dd710a63447a8e01d917e92e79751a63e2")
        assert_equal(dump_output["nchaintx"], 334)
        assert_equal(n0.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        # Mine more blocks on top of the snapshot that n1 hasn't yet seen. This
        # will allow us to test n1's sync-to-tip on top of a snapshot.
        w_skp = address_to_scriptpubkey(w_address)
        w2_skp = address_to_scriptpubkey(w2_address)
        for i in range(100):
            if i % 3 == 0:
                self.mini_wallet.send_to(from_node=n0, scriptPubKey=w_skp, amount=1 * COIN)
                self.mini_wallet.send_to(from_node=n0, scriptPubKey=w2_skp, amount=10 * COIN)
            self.generate(n0, nblocks=1, sync_fun=self.no_op)

        assert_equal(n0.getblockcount(), FINAL_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)
        assert_equal(n2.getblockcount(), START_HEIGHT)

        assert_equal(n0.getblockchaininfo()["blocks"], FINAL_HEIGHT)

        self.log.info(
            f"Loading snapshot into second node from {dump_output['path']}")
        loaded = n1.loadtxoutset(dump_output['path'])
        self.validate_snapshot_import(n1, loaded, dump_output['base_hash'])

        self.log.info("Backup from the snapshot height can be loaded during background sync")
        n1.restorewallet("w", "backup_w.dat")
        # Balance of w wallet is still 0 because n1 has not synced yet
        assert_equal(n1.getbalance(), 0)

        self.log.info("Backup from before the snapshot height can't be loaded during background sync")
        # Error message for wallets that need blocks before the snapshot height.
        def loading_error(height):
            return f"Wallet loading failed. Error loading wallet. Wallet requires blocks to be downloaded, and software does not currently support loading wallets while blocks are being downloaded out of order when using assumeutxo snapshots. Wallet should be able to load successfully after node sync reaches height {height}"
        # The target height is SNAPSHOT_BASE_HEIGHT because that's when background sync completes.
        assert_raises_rpc_error(-4, loading_error(SNAPSHOT_BASE_HEIGHT), n1.restorewallet, "w2", "backup_w2.dat")

        self.test_backup_during_background_sync_pruned_node(n3, dump_output, loading_error(SNAPSHOT_BASE_HEIGHT))

        self.log.info("Test loading descriptors during background sync")
        wallet_name = "w1"
        n1.createwallet(wallet_name, disable_private_keys=True)
        key = get_generate_key()
        time = n1.getblockchaininfo()['time']
        timestamp = 0
        expected_error_message = f"Rescan failed for descriptor with timestamp {timestamp}. There was an error reading a block from time {time}, which is after or within 7200 seconds of key creation, and could contain transactions pertaining to the desc. As a result, transactions and coins using this desc may not appear in the wallet. This error is likely caused by an in-progress assumeutxo background sync. Check logs or getchainstates RPC for assumeutxo background sync progress and try again later."
        result = self.import_descriptor(n1, wallet_name, key, timestamp)
        assert_equal(result[0]['error']['code'], -1)
        assert_equal(result[0]['error']['message'], expected_error_message)

        self.log.info("Test that rescanning blocks from before the snapshot fails when blocks are not available from the background sync yet")
        w1 = n1.get_wallet_rpc(wallet_name)
        assert_raises_rpc_error(-1, "Failed to rescan unavailable blocks likely due to an in-progress assumeutxo background sync. Check logs or getchainstates RPC for assumeutxo background sync progress and try again later.", w1.rescanblockchain, 100)

        PAUSE_HEIGHT = FINAL_HEIGHT - 40

        self.log.info(f"Unload wallets and sync node up to height {PAUSE_HEIGHT}")
        n1.unloadwallet("w")
        n1.unloadwallet(wallet_name)
        dumb_sync_blocks(src=n0, dst=n1, height=PAUSE_HEIGHT)

        self.log.info("Verify node state during background sync")
        # Verify there are still two chainstates (background validation not complete)
        chainstates = n1.getchainstates()['chainstates']
        assert_equal(len(chainstates), 2)
        # The background chainstate should still be at START_HEIGHT
        assert_equal(chainstates[0]['blocks'], START_HEIGHT)
        assert_equal(chainstates[1]["blocks"], PAUSE_HEIGHT)

        # After restart, wallets that existed before cannot be loaded because
        # the wallet loading code checks if required blocks are available for
        # rescanning. During assumeutxo background sync, blocks before the
        # snapshot are not available, so wallet loading fails.
        # After restart, the required height is SNAPSHOT_BASE_HEIGHT + 1 for all wallets.
        assert_raises_rpc_error(-4, loading_error(SNAPSHOT_BASE_HEIGHT + 1), n1.loadwallet, "w")
        assert_raises_rpc_error(-4, loading_error(SNAPSHOT_BASE_HEIGHT + 1), n1.loadwallet, wallet_name)

        # Verify backup from before snapshot height still can't be restored
        assert_raises_rpc_error(-4, loading_error(SNAPSHOT_BASE_HEIGHT + 1), n1.restorewallet, "w2_test", "backup_w2.dat")

        self.complete_background_validation(n1)

        self.log.info("Ensuring wallet can be restored from a backup that was created before the snapshot height")
        n1.restorewallet("w2", "backup_w2.dat")
        # Check balance of w2 wallet
        assert_equal(n1.getbalance(), 340)

        # Check balance of w wallet after node is synced
        n1.loadwallet("w")
        w = n1.get_wallet_rpc("w")
        assert_equal(w.getbalance(), 34)

        self.log.info("Check balance of a wallet that is active during snapshot completion")
        n2.restorewallet("w", "backup_w.dat")
        loaded = n2.loadtxoutset(dump_output['path'])
        self.connect_nodes(0, 2)
        self.wait_until(lambda: len(n2.getchainstates()['chainstates']) == 1)
        ensure_for(duration=1, f=lambda: (n2.getbalance() == 34))

        self.log.info("Ensuring descriptors can be loaded after background sync")
        n1.loadwallet(wallet_name)
        result = self.import_descriptor(n1, wallet_name, key, timestamp)
        assert_equal(result[0]['success'], True)

        self.test_restore_wallet_pruneheight(n3)
        self.test_wallet_reorg_during_background_sync(dump_output)

if __name__ == '__main__':
    AssumeutxoTest(__file__).main()
