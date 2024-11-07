#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet backup features.

Test case is:
4 nodes. 1 2 and 3 send transactions between each other,
fourth node is a miner.
1 2 3 each mine a block to start, then
Miner creates 100 blocks so 1 2 3 each have 50 mature
coins to spend.
Then 5 iterations of 1/2/3 sending coins amongst
themselves to get transactions in the wallets,
and the miner mining one block.

Wallets are backed up using dumpwallet/backupwallet.
Then 5 more iterations of transactions and mining a block.

Miner then generates 101 more blocks, so any
transaction fees paid mature.

Sanity check:
  Sum(1,2,3,4 balances) == 114*50

1/2/3 are shutdown, and their wallets erased.
Then restore using wallet.dat backup. And
confirm 1/2/3/4 balances are same as before.

Shutdown again, restore using importwallet,
and confirm again balances are correct.
"""
from decimal import Decimal
import os
from random import randint
import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class WalletBackupTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        # nodes 1, 2, 3 are spenders, let's give them a keypool=100
        self.extra_args = [
            ["-keypool=100"],
            ["-keypool=100"],
            ["-keypool=100"],
            [],
        ]
        self.rpc_timeout = 120

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 3)
        self.connect_nodes(1, 3)
        self.connect_nodes(2, 3)
        self.connect_nodes(2, 0)
        self.sync_all()

    def one_send(self, from_node, to_address):
        if (randint(1,2) == 1):
            amount = Decimal(randint(1,10)) / Decimal(10)
            self.nodes[from_node].sendtoaddress(to_address, amount)

    def do_one_round(self):
        a0 = self.nodes[0].getnewaddress()
        a1 = self.nodes[1].getnewaddress()
        a2 = self.nodes[2].getnewaddress()

        self.one_send(0, a1)
        self.one_send(0, a2)
        self.one_send(1, a0)
        self.one_send(1, a2)
        self.one_send(2, a0)
        self.one_send(2, a1)

        # Have the miner (node3) mine a block.
        # Must sync mempools before mining.
        self.sync_mempools()
        self.generate(self.nodes[3], 1)

    # As above, this mirrors the original bash test.
    def start_three(self, args=()):
        self.start_node(0, self.extra_args[0] + list(args))
        self.start_node(1, self.extra_args[1] + list(args))
        self.start_node(2, self.extra_args[2] + list(args))
        self.connect_nodes(0, 3)
        self.connect_nodes(1, 3)
        self.connect_nodes(2, 3)
        self.connect_nodes(2, 0)

    def stop_three(self):
        self.stop_node(0)
        self.stop_node(1)
        self.stop_node(2)

    def erase_three(self):
        for node_num in range(3):
            (self.nodes[node_num].wallets_path / self.default_wallet_name / self.wallet_data_filename).unlink()

    def restore_invalid_wallet(self):
        node = self.nodes[3]
        invalid_wallet_file = self.nodes[0].datadir_path / 'invalid_wallet_file.bak'
        open(invalid_wallet_file, 'a', encoding="utf8").write('invald wallet')
        wallet_name = "res0"
        not_created_wallet_file = node.wallets_path / wallet_name
        error_message = "Wallet file verification failed. Failed to load database path '{}'. Data is not in recognized format.".format(not_created_wallet_file)
        assert_raises_rpc_error(-18, error_message, node.restorewallet, wallet_name, invalid_wallet_file)
        assert not not_created_wallet_file.exists()

    def restore_nonexistent_wallet(self):
        node = self.nodes[3]
        nonexistent_wallet_file = self.nodes[0].datadir_path / 'nonexistent_wallet.bak'
        wallet_name = "res0"
        assert_raises_rpc_error(-8, "Backup file does not exist", node.restorewallet, wallet_name, nonexistent_wallet_file)
        not_created_wallet_file = node.wallets_path / wallet_name
        assert not not_created_wallet_file.exists()

    def restore_wallet_existent_name(self):
        node = self.nodes[3]
        backup_file = self.nodes[0].datadir_path / 'wallet.bak'
        wallet_name = "res0"
        wallet_file = node.wallets_path / wallet_name
        error_message = "Failed to create database path '{}'. Database already exists.".format(wallet_file)
        assert_raises_rpc_error(-36, error_message, node.restorewallet, wallet_name, backup_file)
        assert wallet_file.exists()

    def test_pruned_wallet_backup(self):
        self.log.info("Test loading backup on a pruned node when the backup was created close to the prune height of the restoring node")
        node = self.nodes[3]
        self.restart_node(3, ["-prune=1", "-fastprune=1"])
        # Ensure the chain tip is at height 214, because this test assume it is.
        assert_equal(node.getchaintips()[0]["height"], 214)
        # We need a few more blocks so we can actually get above an realistic
        # minimal prune height
        self.generate(node, 50, sync_fun=self.no_op)
        # Backup created at block height 264
        node.backupwallet(node.datadir_path / 'wallet_pruned.bak')
        # Generate more blocks so we can actually prune the older blocks
        self.generate(node, 300, sync_fun=self.no_op)
        # This gives us an actual prune height roughly in the range of 220 - 240
        node.pruneblockchain(250)
        # The backup should be updated with the latest height (locator) for
        # the backup to load successfully this close to the prune height
        node.restorewallet(f'pruned', node.datadir_path / 'wallet_pruned.bak')

    def run_test(self):
        self.log.info("Generating initial blockchain")
        self.generate(self.nodes[0], 1)
        self.generate(self.nodes[1], 1)
        self.generate(self.nodes[2], 1)
        self.generate(self.nodes[3], COINBASE_MATURITY)

        assert_equal(self.nodes[0].getbalance(), 50)
        assert_equal(self.nodes[1].getbalance(), 50)
        assert_equal(self.nodes[2].getbalance(), 50)
        assert_equal(self.nodes[3].getbalance(), 0)

        self.log.info("Creating transactions")
        # Five rounds of sending each other transactions.
        for _ in range(5):
            self.do_one_round()

        self.log.info("Backing up")

        for node_num in range(3):
            self.nodes[node_num].backupwallet(self.nodes[node_num].datadir_path / 'wallet.bak')

        if not self.options.descriptors:
            for node_num in range(3):
                self.nodes[node_num].dumpwallet(self.nodes[node_num].datadir_path / 'wallet.dump')

        self.log.info("More transactions")
        for _ in range(5):
            self.do_one_round()

        # Generate 101 more blocks, so any fees paid mature
        self.generate(self.nodes[3], COINBASE_MATURITY + 1)

        balance0 = self.nodes[0].getbalance()
        balance1 = self.nodes[1].getbalance()
        balance2 = self.nodes[2].getbalance()
        balance3 = self.nodes[3].getbalance()
        total = balance0 + balance1 + balance2 + balance3

        # At this point, there are 214 blocks (103 for setup, then 10 rounds, then 101.)
        # 114 are mature, so the sum of all wallets should be 114 * 50 = 5700.
        assert_equal(total, 5700)

        ##
        # Test restoring spender wallets from backups
        ##
        self.log.info("Restoring wallets on node 3 using backup files")

        self.restore_invalid_wallet()
        self.restore_nonexistent_wallet()

        backup_files = []
        for node_num in range(3):
            backup_files.append(self.nodes[node_num].datadir_path / 'wallet.bak')

        for idx, backup_file in enumerate(backup_files):
            self.nodes[3].restorewallet(f'res{idx}', backup_file)
            assert (self.nodes[3].wallets_path / f'res{idx}').exists()

        res0_rpc = self.nodes[3].get_wallet_rpc("res0")
        res1_rpc = self.nodes[3].get_wallet_rpc("res1")
        res2_rpc = self.nodes[3].get_wallet_rpc("res2")

        assert_equal(res0_rpc.getbalance(), balance0)
        assert_equal(res1_rpc.getbalance(), balance1)
        assert_equal(res2_rpc.getbalance(), balance2)

        self.restore_wallet_existent_name()

        if not self.options.descriptors:
            self.log.info("Restoring using dumped wallet")
            self.stop_three()
            self.erase_three()

            #start node2 with no chain
            shutil.rmtree(self.nodes[2].blocks_path)
            shutil.rmtree(self.nodes[2].chain_path / 'chainstate')

            self.start_three(["-nowallet"])
            # Create new wallets for the three nodes.
            # We will use this empty wallets to test the 'importwallet()' RPC command below.
            for node_num in range(3):
                self.nodes[node_num].createwallet(wallet_name=self.default_wallet_name, descriptors=self.options.descriptors, load_on_startup=True)
                assert_equal(self.nodes[node_num].getbalance(), 0)
                self.nodes[node_num].importwallet(self.nodes[node_num].datadir_path / 'wallet.dump')

            self.sync_blocks()

            assert_equal(self.nodes[0].getbalance(), balance0)
            assert_equal(self.nodes[1].getbalance(), balance1)
            assert_equal(self.nodes[2].getbalance(), balance2)

        # Backup to source wallet file must fail
        sourcePaths = [
            os.path.join(self.nodes[0].wallets_path, self.default_wallet_name, self.wallet_data_filename),
            os.path.join(self.nodes[0].wallets_path, '.', self.default_wallet_name, self.wallet_data_filename),
            os.path.join(self.nodes[0].wallets_path, self.default_wallet_name),
            os.path.join(self.nodes[0].wallets_path)]

        for sourcePath in sourcePaths:
            assert_raises_rpc_error(-4, "backup failed", self.nodes[0].backupwallet, sourcePath)

        self.test_pruned_wallet_backup()


if __name__ == '__main__':
    WalletBackupTest(__file__).main()
