#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test for assumeutxo wallet related behavior.
See feature_assumeutxo.py for background.
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import get_generate_key
from test_framework.descriptors import descsum_create

START_HEIGHT = 199
SNAPSHOT_BASE_HEIGHT = 299
FINAL_HEIGHT = 399


class AssumeutxoTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        """Use the pregenerated, deterministic chain up to height 199."""
        self.num_nodes = 3
        self.rpc_timeout = 120
        self.extra_args = [
            [],
            [],
            ["-prune=1"]
        ]

    def setup_network(self):
        """Start with the nodes disconnected so that one can generate a snapshot
        including blocks the other hasn't yet seen."""
        self.add_nodes(3)
        self.start_nodes(extra_args=self.extra_args)

    def test_descriptor_import(self, node, wallet_name, key, timestamp, expected_error_message=None):
        import_request = [{"desc": descsum_create("pkh(" + key.pubkey + ")"),
                           "timestamp": timestamp,
                           "label": "Descriptor import test"}]
        wrpc = node.get_wallet_rpc(wallet_name)
        result = wrpc.importdescriptors(import_request)

        if expected_error_message is None:
            assert_equal(result[0]['success'], True)
        else:
            assert_equal(result[0]['error']['code'], -1)
            assert_equal(result[0]['error']['message'], expected_error_message)

    def validate_snapshot_import(self, node, loaded, base_hash):
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        normal, snapshot = node.getchainstates()["chainstates"]
        assert_equal(normal['blocks'], START_HEIGHT)
        assert_equal(normal.get('snapshot_blockhash'), None)
        assert_equal(normal['validated'], True)
        assert_equal(snapshot['blocks'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(snapshot['snapshot_blockhash'], base_hash)
        assert_equal(snapshot['validated'], False)

        assert_equal(node.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

    def run_test(self):
        """
        Bring up two (disconnected) nodes, mine some new blocks on the first,
        and generate a UTXO snapshot.

        Load the snapshot into the second, ensure it syncs to tip and completes
        background validation when connected to the first.
        """
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        n2 = self.nodes[2]

        self.mini_wallet = MiniWallet(n0)

        # Mock time for a deterministic chain
        for n in self.nodes:
            n.setmocktime(n.getblockheader(n.getbestblockhash())['time'])

        n0.createwallet('w')
        w = n0.get_wallet_rpc("w")

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

        # Ensure everyone is seeing the same headers.
        for n in self.nodes:
            assert_equal(n.getblockchaininfo()[
                         "headers"], SNAPSHOT_BASE_HEIGHT)

        w.backupwallet("backup_w.dat")

        self.log.info("-- Testing assumeutxo")

        assert_equal(n0.getblockcount(), SNAPSHOT_BASE_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)

        self.log.info(
            f"Creating a UTXO snapshot at height {SNAPSHOT_BASE_HEIGHT}")
        dump_output = n0.dumptxoutset('utxos.dat')

        assert_equal(
            dump_output['txoutset_hash'],
            "a4bf3407ccb2cc0145c49ebba8fa91199f8a3903daf0883875941497d2493c27")
        assert_equal(dump_output["nchaintx"], 334)
        assert_equal(n0.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        # Mine more blocks on top of the snapshot that n1 hasn't yet seen. This
        # will allow us to test n1's sync-to-tip on top of a snapshot.
        self.generate(n0, nblocks=100, sync_fun=self.no_op)

        assert_equal(n0.getblockcount(), FINAL_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)

        assert_equal(n0.getblockchaininfo()["blocks"], FINAL_HEIGHT)

        self.log.info(
            f"Loading snapshot into second node from {dump_output['path']}")
        loaded = n1.loadtxoutset(dump_output['path'])
        self.validate_snapshot_import(n1, loaded, dump_output['base_hash'])

        self.log.info("Backup can't be loaded during background sync")
        error_message = "Wallet loading failed. Error loading wallet. Wallet requires blocks to be downloaded, and software does not currently support loading wallets while blocks are being downloaded out of order when using assumeutxo snapshots. Wallet should be able to load successfully after node sync reaches height 299"
        assert_raises_rpc_error(-4, error_message, n1.restorewallet, "w", "backup_w.dat")

        self.log.info("Backup can't be loaded during background sync (pruned node)")
        loaded = n2.loadtxoutset(dump_output['path'])
        self.validate_snapshot_import(n2, loaded, dump_output['base_hash'])
        assert_raises_rpc_error(-4,  error_message, n2.restorewallet, "w", "backup_w.dat")

        self.log.info("Test loading descriptors during background sync")
        wallet_name = "w1"
        n1.createwallet(wallet_name, disable_private_keys=True)
        key = get_generate_key()
        time = n1.getblockchaininfo()['time']
        timestamp = 0
        error_message = f"Rescan failed for descriptor with timestamp {timestamp}. There was an error reading a block from time {time}, which is after or within 7200 seconds of key creation, and could contain transactions pertaining to the desc. As a result, transactions and coins using this desc may not appear in the wallet. This error could be caused by pruning or data corruption (see bitcoind log for details) and could be dealt with by downloading and rescanning the relevant blocks (see -reindex option and rescanblockchain RPC)."
        self.test_descriptor_import(n1, wallet_name, key, timestamp, error_message)

        PAUSE_HEIGHT = FINAL_HEIGHT - 40

        self.log.info("Restarting node to stop at height %d", PAUSE_HEIGHT)
        self.restart_node(1, extra_args=[
            f"-stopatheight={PAUSE_HEIGHT}", *self.extra_args[1]])

        # Finally connect the nodes and let them sync.
        #
        # Set `wait_for_connect=False` to avoid a race between performing connection
        # assertions and the -stopatheight tripping.
        self.connect_nodes(0, 1, wait_for_connect=False)

        n1.wait_until_stopped(timeout=5)

        self.log.info(
            "Restarted node before snapshot validation completed, reloading...")
        self.restart_node(1, extra_args=self.extra_args[1])

        # TODO: inspect state of e.g. the wallet before reconnecting
        self.connect_nodes(0, 1)

        self.log.info(
            f"Ensuring snapshot chain syncs to tip. ({FINAL_HEIGHT})")
        self.wait_until(lambda: n1.getchainstates()[
                        'chainstates'][-1]['blocks'] == FINAL_HEIGHT)
        self.sync_blocks(nodes=(n0, n1))

        self.log.info("Ensuring background validation completes")
        self.wait_until(lambda: len(n1.getchainstates()['chainstates']) == 1)

        self.log.info("Ensuring wallet can be restored from backup")
        n1.restorewallet("w", "backup_w.dat")

        # Now make sure background validation succeeds on n2
        # and try restoring the wallet
        self.connect_nodes(0, 2)
        self.sync_blocks(nodes=(n0, n2))
        self.wait_until(lambda: len(n2.getchainstates()['chainstates']) == 1)
        self.log.info("Ensuring wallet can be restored from backup (pruned node)")
        n2.restorewallet("w", "backup_w.dat")

        self.log.info("Ensuring descriptors can be loaded after background sync")
        n1.loadwallet(wallet_name)
        self.test_descriptor_import(n1, wallet_name, key, timestamp)


if __name__ == '__main__':
    AssumeutxoTest(__file__).main()
