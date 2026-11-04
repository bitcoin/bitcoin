#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet backup features.

Test case is:
4 nodes. 1 2 and 3 send transactions between each other,
fourth node is a miner.
1 2 3 each mine a block to start, then
Miner creates 100 blocks so 1 2 3 each have 500 mature
coins to spend.
Then 5 iterations of 1/2/3 sending coins amongst
themselves to get transactions in the wallets,
and the miner mining one block.

Wallets are backed up using dumpwallet/backupwallet.
Then 5 more iterations of transactions and mining a block.

Miner then generates 101 more blocks, so any
transaction fees paid mature.

Sanity check:
  Sum(1,2,3,4 balances) == 114*500

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
    sha256sum_file,
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
        os.remove(os.path.join(self.nodes[0].datadir, self.chain, 'wallets', self.default_wallet_name, self.wallet_data_filename))
        os.remove(os.path.join(self.nodes[1].datadir, self.chain, 'wallets', self.default_wallet_name, self.wallet_data_filename))
        os.remove(os.path.join(self.nodes[2].datadir, self.chain, 'wallets', self.default_wallet_name, self.wallet_data_filename))

    def restore_invalid_wallet(self):
        node = self.nodes[3]
        invalid_wallet_file = os.path.join(self.nodes[0].datadir, 'invalid_wallet_file.bak')
        open(invalid_wallet_file, 'a', encoding="utf8").write('invald wallet')
        wallet_name = "res0"
        not_created_wallet_file = os.path.join(node.datadir, self.chain, 'wallets', wallet_name)
        error_message = "Wallet file verification failed. Failed to load database path '{}'. Data is not in recognized format.".format(not_created_wallet_file)
        assert_raises_rpc_error(-18, error_message, node.restorewallet, wallet_name, invalid_wallet_file)
        assert not os.path.exists(not_created_wallet_file)

    def restore_corrupt_bdb_wallet(self):
        if self.options.descriptors:
            return
        node = self.nodes[3]
        backup_file = os.path.join(self.nodes[0].datadir, 'wallet.bak')
        corrupt_file = backup_file + '.corrupt'
        with open(backup_file, 'rb') as source, open(corrupt_file, 'wb') as dest:
            dest.write(source.read(4096))
        wallet_name = 'restore_corrupt_bdb'
        assert_raises_rpc_error(-4, 'corrupt', node.restorewallet, wallet_name, corrupt_file)
        assert not os.path.exists(os.path.join(node.datadir, self.chain, 'wallets', wallet_name))

    def restore_nonexistent_wallet(self):
        node = self.nodes[3]
        nonexistent_wallet_file = os.path.join(self.nodes[0].datadir, 'nonexistent_wallet.bak')
        wallet_name = "res0"
        assert_raises_rpc_error(-8, "Backup file does not exist", node.restorewallet, wallet_name, nonexistent_wallet_file)
        not_created_wallet_file = os.path.join(node.datadir, self.chain, 'wallets', wallet_name)
        assert not os.path.exists(not_created_wallet_file)

    def restore_preserves_existing_directory(self):
        node = self.nodes[3]
        wallet_dir = os.path.join(node.datadir, self.chain, 'wallets')
        backup_file = os.path.join(self.nodes[0].datadir, 'wallet.bak')
        invalid_path = os.path.join(node.datadir, 'x' * 256)
        for wallet_name in ('restore_existing', os.path.join(node.datadir, 'restore_absolute')):
            destination = os.path.join(wallet_dir, wallet_name)
            sentinel = os.path.join(destination, 'nested', 'sentinel')
            os.makedirs(os.path.dirname(sentinel))
            with open(sentinel, 'w', encoding='utf8') as f:
                f.write('preserve existing data')
            assert_raises_rpc_error(-4, 'Unexpected exception:', node.restorewallet, wallet_name, invalid_path)
            with open(sentinel, encoding='utf8') as f:
                assert_equal(f.read(), 'preserve existing data')
            assert_raises_rpc_error(-8, 'Backup file does not exist', node.restorewallet, wallet_name, backup_file + '.missing')
            existing_db = os.path.join(destination, 'wallet.dat')
            with open(existing_db, 'w', encoding='utf8') as f:
                f.write('not a wallet')
            assert_raises_rpc_error(-36, 'Database file exists', node.restorewallet, wallet_name, backup_file)
            assert os.path.isfile(sentinel)
            with open(existing_db, encoding='utf8') as f:
                assert_equal(f.read(), 'not a wallet')

    def restore_wallet_existent_name(self):
        node = self.nodes[3]
        backup_file = os.path.join(self.nodes[0].datadir, 'wallet.bak')
        wallet_name = "res0"
        wallet_file = os.path.join(node.datadir, self.chain, 'wallets', wallet_name)
        error_message = "Failed to restore wallet. Database file exists in '{}'.".format(os.path.join(wallet_file, "wallet.dat"))
        assert_raises_rpc_error(-36, error_message, node.restorewallet, wallet_name, backup_file)
        assert os.path.exists(wallet_file)

    def test_restore_existent_dir(self):
        self.log.info("Test restore on an existent empty directory")
        node = self.nodes[3]
        backup_file = os.path.join(self.nodes[0].datadir, 'wallet.bak')
        wallet_name = "restored_wallet"
        wallet_dir = node.chain_path / "wallets" / wallet_name
        os.mkdir(wallet_dir)
        res = node.restorewallet(wallet_name, backup_file)
        assert_equal(res['name'], wallet_name)
        node.unloadwallet(wallet_name)

        self.log.info("Test restore succeeds when the target directory contains non-wallet files")
        wallet_file = node.chain_path / "wallets" / wallet_name / "wallet.dat"
        os.remove(wallet_file)
        extra_file = node.chain_path / "wallets" / wallet_name / "not_a_wallet.txt"
        extra_file.touch()
        res = node.restorewallet(wallet_name, backup_file)
        assert_equal(res['name'], wallet_name)
        assert extra_file.exists() # extra file was not removed by mistake
        node.unloadwallet(wallet_name)

        self.log.info("Test restore failure due to existing db file in the destination directory")
        original_shasum = sha256sum_file(wallet_file)
        error_message = "Failed to restore wallet. Database file exists in '{}'.".format(wallet_dir / "wallet.dat")
        assert_raises_rpc_error(-36, error_message, node.restorewallet, wallet_name, backup_file)
        # Ensure the wallet file remains untouched
        assert wallet_dir.exists()
        assert_equal(original_shasum, sha256sum_file(wallet_file))

        self.log.info("Test restore succeeds when the .dat file in the destination has a different name")
        second_wallet = wallet_dir / "hidden_storage.dat"
        os.rename(wallet_dir / "wallet.dat", second_wallet)
        original_shasum = sha256sum_file(second_wallet)
        res = node.restorewallet(wallet_name, backup_file)
        assert_equal(res['name'], wallet_name)
        assert (wallet_dir / "hidden_storage.dat").exists()
        assert_equal(original_shasum, sha256sum_file(second_wallet))
        node.unloadwallet(wallet_name)

        # Clean for follow-up tests
        os.remove(wallet_file)

    def test_restore_into_unnamed_wallet(self):
        self.log.info("Test restore into a default unnamed wallet")
        # This is also useful to test the migration recovery after failure logic
        node = self.nodes[3]
        if not self.options.descriptors:
            node.unloadwallet("")
            os.rename(node.chain_path / "wallets" / "wallet.dat", node.chain_path / "wallets" / "default.wallet.dat")
        backup_file = os.path.join(self.nodes[0].datadir, 'wallet.bak')
        wallet_name = ""
        res = node.restorewallet(wallet_name, backup_file)
        assert_equal(res['name'], "")
        assert (node.chain_path / "wallets" / "wallet.dat").exists()
        # Clean for follow-up tests
        node.unloadwallet("")
        os.remove(node.chain_path / "wallets" / "wallet.dat")
        if not self.options.descriptors:
            os.rename(node.chain_path / "wallets" / "default.wallet.dat", node.chain_path / "wallets" / "wallet.dat")
            node.loadwallet("")


    def run_test(self):
        self.log.info("Generating initial blockchain")
        self.generate(self.nodes[0], 1)
        self.generate(self.nodes[1], 1)
        self.generate(self.nodes[2], 1)
        self.generate(self.nodes[3], COINBASE_MATURITY)

        assert_equal(self.nodes[0].getbalance(), 500)
        assert_equal(self.nodes[1].getbalance(), 500)
        assert_equal(self.nodes[2].getbalance(), 500)
        assert_equal(self.nodes[3].getbalance(), 0)

        self.log.info("Creating transactions")
        # Five rounds of sending each other transactions.
        for _ in range(5):
            self.do_one_round()

        self.log.info("Backing up")

        self.nodes[0].backupwallet(os.path.join(self.nodes[0].datadir, 'wallet.bak'))
        self.nodes[1].backupwallet(os.path.join(self.nodes[1].datadir, 'wallet.bak'))
        self.nodes[2].backupwallet(os.path.join(self.nodes[2].datadir, 'wallet.bak'))

        if not self.options.descriptors:
            self.nodes[0].dumpwallet(os.path.join(self.nodes[0].datadir, 'wallet.dump'))
            self.nodes[1].dumpwallet(os.path.join(self.nodes[1].datadir, 'wallet.dump'))
            self.nodes[2].dumpwallet(os.path.join(self.nodes[2].datadir, 'wallet.dump'))

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
        # 114 are mature, so the sum of all wallets should be 114 * 500 = 57000.
        assert_equal(total, 57000)

        ##
        # Test restoring spender wallets from backups
        ##
        self.log.info("Restoring wallets on node 3 using backup files")

        self.restore_invalid_wallet()
        self.restore_nonexistent_wallet()
        self.restore_preserves_existing_directory()
        self.restore_corrupt_bdb_wallet()

        backup_file_0 = os.path.join(self.nodes[0].datadir, 'wallet.bak')
        backup_file_1 = os.path.join(self.nodes[1].datadir, 'wallet.bak')
        backup_file_2 = os.path.join(self.nodes[2].datadir, 'wallet.bak')

        self.nodes[3].restorewallet("res0", backup_file_0)
        self.nodes[3].restorewallet("res1", backup_file_1)
        self.nodes[3].restorewallet("res2", backup_file_2)

        assert os.path.exists(os.path.join(self.nodes[3].datadir, self.chain, 'wallets', "res0"))
        assert os.path.exists(os.path.join(self.nodes[3].datadir, self.chain, 'wallets', "res1"))
        assert os.path.exists(os.path.join(self.nodes[3].datadir, self.chain, 'wallets', "res2"))

        res0_rpc = self.nodes[3].get_wallet_rpc("res0")
        res1_rpc = self.nodes[3].get_wallet_rpc("res1")
        res2_rpc = self.nodes[3].get_wallet_rpc("res2")

        assert_equal(res0_rpc.getbalance(), balance0)
        assert_equal(res1_rpc.getbalance(), balance1)
        assert_equal(res2_rpc.getbalance(), balance2)

        self.restore_wallet_existent_name()
        self.test_restore_existent_dir()
        self.test_restore_into_unnamed_wallet()

        if not self.options.descriptors:
            self.log.info("Restoring using dumped wallet")
            self.stop_three()
            self.erase_three()

            #start node2 with no chain
            shutil.rmtree(os.path.join(self.nodes[2].datadir, self.chain, 'blocks'))
            shutil.rmtree(os.path.join(self.nodes[2].datadir, self.chain, 'chainstate'))
            shutil.rmtree(os.path.join(self.nodes[2].datadir, self.chain, 'evodb'))
            shutil.rmtree(os.path.join(self.nodes[2].datadir, self.chain, 'llmq'))
            shutil.rmtree(os.path.join(self.nodes[2].datadir, self.chain, 'indexes'))

            self.start_three(["-nowallet"])
            # Create new wallets for the three nodes.
            # We will use this empty wallets to test the 'importwallet()' RPC command below.
            for node_num in range(3):
                self.nodes[node_num].createwallet(wallet_name=self.default_wallet_name, descriptors=self.options.descriptors, load_on_startup=True)

            assert_equal(self.nodes[0].getbalance(), 0)
            assert_equal(self.nodes[1].getbalance(), 0)
            assert_equal(self.nodes[2].getbalance(), 0)

            self.nodes[0].importwallet(os.path.join(self.nodes[0].datadir, 'wallet.dump'))
            self.nodes[1].importwallet(os.path.join(self.nodes[1].datadir, 'wallet.dump'))
            self.nodes[2].importwallet(os.path.join(self.nodes[2].datadir, 'wallet.dump'))

            self.sync_blocks()

            assert_equal(self.nodes[0].getbalance(), balance0)
            assert_equal(self.nodes[1].getbalance(), balance1)
            assert_equal(self.nodes[2].getbalance(), balance2)

        # Backup to source wallet file must fail
        sourcePaths = [
            os.path.join(self.nodes[0].datadir, self.chain, 'wallets', self.default_wallet_name, self.wallet_data_filename),
            os.path.join(self.nodes[0].datadir, self.chain, '.', 'wallets', self.default_wallet_name, self.wallet_data_filename),
            os.path.join(self.nodes[0].datadir, self.chain, 'wallets', self.default_wallet_name),
            os.path.join(self.nodes[0].datadir, self.chain, 'wallets')]

        for sourcePath in sourcePaths:
            assert_raises_rpc_error(-4, "backup failed", self.nodes[0].backupwallet, sourcePath)


if __name__ == '__main__':
    WalletBackupTest().main()
