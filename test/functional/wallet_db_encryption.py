#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.
"""Test Wallets with encrypted database"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
    assert_greater_than,
)


class WalletDBEncryptionTest(BitcoinTestFramework):
    PASSPHRASE = "WalletPassphrase"
    PASSPHRASE2 = "SecondWalletPassphrase"
    WRONG_PASSPHRASE = "NotTheRightPassphrase"
    PASSPHRASE_WITH_NULLS = "Passphrase\0With\0Nulls"

    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=True)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_no_legacy(self):
        if not self.is_bdb_compiled():
            return
        self.log.info("Test that legacy wallets do not support encrypted databases")
        assert_raises_rpc_error(-4, "Database encryption is only supported for descriptor wallets", self.nodes[0].createwallet, wallet_name="legacy_encdb", db_passphrase=self.PASSPHRASE, descriptors=False)

    def test_create_and_load(self):
        self.log.info("Testing that a wallet with encrypted database can be created")
        self.nodes[0].createwallet(wallet_name="basic_encrypted_db", db_passphrase=self.PASSPHRASE)
        wallet = self.nodes[0].get_wallet_rpc("basic_encrypted_db")
        info = wallet.getwalletinfo()
        assert_equal("encrypted_sqlite", info["format"])

        # Add some data to the wallet that we should see persisted
        addr = wallet.getnewaddress()
        txid_in = self.default_wallet.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        txid_out = wallet.sendtoaddress(self.default_wallet.getnewaddress(), 5)
        self.generate(self.nodes[0], 1)

        wallet.unloadwallet()

        self.log.info("Testing loading of a wallet with encrypted database")
        assert_raises_rpc_error(-4, "Database is encrypted but passphrase was not provided", self.nodes[0].loadwallet, filename="basic_encrypted_db")
        assert_raises_rpc_error(-4, "Unable to decrypt database, are you sure the passphrase is correct?", self.nodes[0].loadwallet, filename="basic_encrypted_db", db_passphrase=self.WRONG_PASSPHRASE)
        self.nodes[0].loadwallet(filename="basic_encrypted_db", db_passphrase=self.PASSPHRASE)
        info = wallet.getwalletinfo()
        assert_equal("encrypted_sqlite", info["format"])

        # Make sure that our persisted data is still here
        addr_info = wallet.getaddressinfo(addr)
        assert_equal(addr_info["ismine"], True)
        tx_in = wallet.gettransaction(txid_in)
        assert_equal(tx_in["amount"], 10)
        tx_out = wallet.gettransaction(txid_out)
        assert_equal(tx_out["amount"], -5)
        wallet.sendtoaddress(self.default_wallet.getnewaddress(), 2)

        wallet.unloadwallet()

        self.log.info("Test that listwalletdir lists wallets with encrypted dbs")
        wallets = [w["name"] for w in self.nodes[0].listwalletdir()["wallets"]]
        assert "basic_encrypted_db" in wallets

    def test_passphrases_with_nulls(self):
        self.log.info("Testing passphrases with nulls for wallets with encrypted databases")
        self.nodes[0].createwallet(wallet_name="encdb_with_nulls", db_passphrase=self.PASSPHRASE_WITH_NULLS)
        wallet = self.nodes[0].get_wallet_rpc("encdb_with_nulls")
        info = wallet.getwalletinfo()
        assert_equal("encrypted_sqlite", info["format"])
        wallet.unloadwallet()

        assert_raises_rpc_error(-4, "Unable to decrypt database, are you sure the passphrase is correct?", self.nodes[0].loadwallet, filename="encdb_with_nulls", db_passphrase=self.PASSPHRASE_WITH_NULLS.partition("\0")[0])
        self.nodes[0].loadwallet(filename="encdb_with_nulls", db_passphrase=self.PASSPHRASE_WITH_NULLS)

    def test_on_start(self):
        self.log.info("Test that wallets with encrypted db are ignored on startup")
        self.nodes[0].createwallet(wallet_name="startup_encdb", db_passphrase=self.PASSPHRASE)
        with self.nodes[0].assert_debug_log(expected_msgs=["Warning: Skipping -wallet path to encrypted wallet, use loadwallet to load it."]):
            self.stop_node(0)
            self.start_node(0, extra_args=["-wallet=startup_encdb"])
            # Need to clear stderr file so that test shutdown works
            self.nodes[0].stderr.seek(0)
            self.nodes[0].stderr.truncate(0)
        assert_equal(self.nodes[0].listwallets(), [self.default_wallet_name])
        self.nodes[0].loadwallet(filename="startup_encdb", db_passphrase=self.PASSPHRASE)

    def test_double_encrypted(self):
        self.log.info("Test that wallet encryption is not db encryption")
        self.nodes[0].createwallet(wallet_name="enc_wallet_not_db", passphrase=self.PASSPHRASE)
        wallet = self.nodes[0].get_wallet_rpc("enc_wallet_not_db")
        info = wallet.getwalletinfo()
        assert_equal(info["format"], "sqlite")
        assert_equal(info["unlocked_until"], 0)

        self.nodes[0].createwallet(wallet_name="enc_wallet_not_db2")
        wallet = self.nodes[0].get_wallet_rpc("enc_wallet_not_db2")
        wallet.encryptwallet(self.PASSPHRASE)
        info = wallet.getwalletinfo()
        assert_equal(info["format"], "sqlite")
        assert_equal(info["unlocked_until"], 0)

        self.log.info("Test that wallets with encrypted db can also be encrypted normally")
        self.nodes[0].createwallet(wallet_name="double_enc", db_passphrase=self.PASSPHRASE, passphrase=self.PASSPHRASE2)
        wallet = self.nodes[0].get_wallet_rpc("double_enc")
        info = wallet.getwalletinfo()
        assert_equal(info["format"], "encrypted_sqlite")
        assert_equal(info["unlocked_until"], 0)
        wallet.walletpassphrase(self.PASSPHRASE2, 10)
        assert_greater_than(wallet.getwalletinfo()["unlocked_until"], 0)
        wallet.walletlock()

        self.nodes[0].createwallet(wallet_name="double_enc2", db_passphrase=self.PASSPHRASE)
        wallet = self.nodes[0].get_wallet_rpc("double_enc2")
        wallet.encryptwallet(self.PASSPHRASE2)
        info = wallet.getwalletinfo()
        assert_equal(info["format"], "encrypted_sqlite")
        assert_equal(info["unlocked_until"], 0)
        wallet.walletpassphrase(self.PASSPHRASE2, 10)
        assert_greater_than(wallet.getwalletinfo()["unlocked_until"], 0)
        wallet.walletlock()

        self.log.info("Test that changing wallet passphrase does not affect database passphrase")
        wallet.walletpassphrase(self.PASSPHRASE2, 10)
        wallet.walletpassphrasechange(self.PASSPHRASE2, self.PASSPHRASE_WITH_NULLS)
        wallet.walletlock()
        assert_raises_rpc_error(-14, "Error: The wallet passphrase entered was incorrect.", wallet.walletpassphrase, self.PASSPHRASE, 10)
        assert_raises_rpc_error(-14, "Error: The wallet passphrase entered was incorrect.", wallet.walletpassphrase, self.PASSPHRASE2, 10)
        wallet.walletpassphrase(self.PASSPHRASE_WITH_NULLS, 10)
        wallet.unloadwallet()

        assert_raises_rpc_error(-4, "Wallet file verification failed. Unable to decrypt database, are you sure the passphrase is correct?", self.nodes[0].loadwallet, filename="double_enc2", db_passphrase=self.PASSPHRASE_WITH_NULLS)
        assert_raises_rpc_error(-4, "Wallet file verification failed. Unable to decrypt database, are you sure the passphrase is correct?", self.nodes[0].loadwallet, filename="double_enc2", db_passphrase=self.PASSPHRASE2)
        self.nodes[0].loadwallet(filename="double_enc2", db_passphrase=self.PASSPHRASE)

    def run_test(self):
        self.default_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 101)

        self.test_no_legacy()
        self.test_create_and_load()
        self.test_passphrases_with_nulls()
        self.test_on_start()
        self.test_double_encrypted()

if __name__ == '__main__':
    WalletDBEncryptionTest().main()
