#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Migrating a wallet from legacy to descriptor."""

import os

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class WalletMigrationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_is_sqlite(self, wallet_name):
        wallet_file_path = os.path.join(self.nodes[0].datadir, "regtest/wallets", wallet_name, self.wallet_data_filename)
        with open(wallet_file_path, 'rb') as f:
            file_magic = f.read(16)
            assert_equal(file_magic, b'SQLite format 3\x00')
        assert_equal(self.nodes[0].get_wallet_rpc(wallet_name).getwalletinfo()["format"], "sqlite")

    def test_basic(self):
        default = self.nodes[0].get_wallet_rpc("")

        self.log.info("Test migration of a basic keys only wallet without balance")
        self.nodes[0].createwallet(wallet_name="basic0")
        basic0 = self.nodes[0].get_wallet_rpc("basic0")
        assert_equal(basic0.getwalletinfo()["descriptors"], False)

        addr = basic0.getnewaddress()
        change = basic0.getrawchangeaddress()

        assert_equal(basic0.getaddressinfo(addr)["ismine"], True)
        assert_equal(basic0.getaddressinfo(change)["ismine"], True)

        basic0.migratewallet()
        assert_equal(basic0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic0")

        addr_info = basic0.getaddressinfo(addr)
        assert_equal(addr_info["ismine"], True)
        assert_equal(basic0.getaddressinfo(change)["ismine"], True)
        assert_equal(addr_info["hdkeypath"], "m/0'/0'/0'")

        addr_info = basic0.getaddressinfo(basic0.getnewaddress("", "bech32"))
        assert_equal(addr_info["hdkeypath"], "m/84'/1'/0'/0/0")

        self.log.info("Test migration of a basic keys only wallet with a balance")
        self.nodes[0].createwallet(wallet_name="basic1")
        basic1 = self.nodes[0].get_wallet_rpc("basic1")
        assert_equal(basic1.getwalletinfo()["descriptors"], False)

        for i in range(0, 10):
            default.sendtoaddress(basic1.getnewaddress(), 1)

        self.nodes[0].generate(1)

        for i in range(0, 5):
            basic1.sendtoaddress(default.getnewaddress(), 0.5)

        self.nodes[0].generate(1)
        bal = basic1.getbalance()
        txs = basic1.listtransactions()

        basic1.migratewallet()
        assert_equal(basic1.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic1")
        assert_equal(basic1.getbalance(), bal)
        assert_equal(basic1.listtransactions(), txs)

    def test_multisig(self):
        default = self.nodes[0].get_wallet_rpc("")

        # Contrived case where all the multisig keys are in a single wallet
        self.log.info("Test migration of a wallet with all keys for a multisig")
        self.nodes[0].createwallet(wallet_name="multisig0")
        multisig0 = self.nodes[0].get_wallet_rpc("multisig0")
        assert_equal(multisig0.getwalletinfo()["descriptors"], False)
        addr1 = multisig0.getnewaddress()
        addr2 = multisig0.getnewaddress()
        addr3 = multisig0.getnewaddress()

        ms_info = multisig0.addmultisigaddress(2, [addr1, addr2, addr3])

        multisig0.migratewallet()
        assert_equal(multisig0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("multisig0")
        ms_addr_info = multisig0.getaddressinfo(ms_info["address"])
        assert_equal(ms_addr_info["ismine"], True)
        assert_equal(ms_addr_info["desc"], ms_info["descriptor"])

        pub1 = multisig0.getaddressinfo(addr1)["pubkey"]
        pub2 = multisig0.getaddressinfo(addr2)["pubkey"]

        # Some keys in multisig do not belong to this wallet
        self.log.info("Test migration of a wallet that has some keys in a multisig")
        self.nodes[0].createwallet(wallet_name="multisig1")
        multisig1 = self.nodes[0].get_wallet_rpc("multisig1")
        ms_info = multisig1.addmultisigaddress(2, [multisig1.getnewaddress(), pub1, pub2])
        ms_info2 = multisig1.addmultisigaddress(2, [multisig1.getnewaddress(), pub1, pub2])
        assert_equal(multisig1.getwalletinfo()["descriptors"], False)

        addr = ms_info["address"]
        txid = default.sendtoaddress(addr, 10)
        multisig1.importaddress(addr)
        self.nodes[0].generate(1)
        multisig1.gettransaction(txid)
        assert_equal(multisig1.getbalances()["watchonly"]["trusted"], 10)
        assert_equal("multisig0_watchonly" in self.nodes[0].listwallets(), False)
        assert_equal("multisig0_solvables" in self.nodes[0].listwallets(), False)

        # Migrating multisig1 should see the multisig is no longer part of multisig1
        # A new wallet multisig1_watchonly is created which has the multisig address
        # Transaction to multisig is in multisig1_watchonly and not multisig1
        multisig1.migratewallet()
        assert_equal(multisig1.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("multisig1")
        assert_equal(multisig1.getaddressinfo(ms_info["address"])["ismine"], False)
        assert_equal(multisig1.getaddressinfo(ms_info["address"])["iswatchonly"], False)
        assert_equal(multisig1.getaddressinfo(ms_info["address"])["solvable"], False)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", multisig1.gettransaction, txid)
        assert_equal(multisig1.getbalance(), 0)
        assert_equal(multisig1.listtransactions(), [])

        assert_equal("multisig1_watchonly" in self.nodes[0].listwallets(), True)
        ms1_watchonly = self.nodes[0].get_wallet_rpc("multisig1_watchonly")
        ms1_wallet_info = ms1_watchonly.getwalletinfo()
        assert_equal(ms1_wallet_info['descriptors'], True)
        assert_equal(ms1_wallet_info['private_keys_enabled'], False)
        self.assert_is_sqlite("multisig1_watchonly")
        assert_equal(ms1_watchonly.getaddressinfo(ms_info["address"])["ismine"], True)
        assert_equal(ms1_watchonly.getaddressinfo(ms_info["address"])["solvable"], True)
        ms1_watchonly.gettransaction(txid)
        assert_equal(ms1_watchonly.getbalance(), 10)

        # Migrating multisig1 should see the second multisig is no longer part of multisig1
        # A new wallet multisig1_solvables is created which has the second address
        # This should have no transactions
        assert_equal("multisig1_solvables" in self.nodes[0].listwallets(), True)
        ms1_solvable = self.nodes[0].get_wallet_rpc("multisig1_solvables")
        ms1_wallet_info = ms1_solvable.getwalletinfo()
        assert_equal(ms1_wallet_info['descriptors'], True)
        assert_equal(ms1_wallet_info['private_keys_enabled'], False)
        self.assert_is_sqlite("multisig1_solvables")
        assert_equal(ms1_solvable.getaddressinfo(ms_info["address"])["ismine"], False)
        assert_equal(ms1_solvable.getaddressinfo(ms_info["address"])["solvable"], False)
        assert_equal(ms1_solvable.getaddressinfo(ms_info2["address"])["ismine"], True)
        assert_equal(ms1_solvable.getaddressinfo(ms_info2["address"])["solvable"], True)
        assert_equal(ms1_solvable.getbalance(), 0)
        assert_equal(ms1_solvable.listtransactions(), [])


    def test_other_watchonly(self):
        default = self.nodes[0].get_wallet_rpc("")

        # Wallet with an imported address. Should be the same thing as the multisig test
        self.log.info("Test migration of a wallet with watchonly imports")
        self.nodes[0].createwallet(wallet_name="imports0")
        imports0 = self.nodes[0].get_wallet_rpc("imports0")
        assert_equal(imports0.getwalletinfo()["descriptors"], False)

        addr = default.getnewaddress()
        imports0.importaddress(addr)
        txid = default.sendtoaddress(addr, 10)
        self.nodes[0].generate(1)

        imports0.gettransaction(txid)
        bal = imports0.getbalance(include_watchonly=True)

        txid2 = default.sendtoaddress(imports0.getnewaddress(), 10)
        self.nodes[0].generate(1)
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 2)

        imports0.migratewallet()
        assert_equal(imports0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("imports0")
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, txid)
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 1)
        imports0.gettransaction(txid2)

        assert_equal("imports0_watchonly" in self.nodes[0].listwallets(), True)
        watchonly = self.nodes[0].get_wallet_rpc("imports0_watchonly")
        watchonly_info = watchonly.getwalletinfo()
        assert_equal(watchonly_info["descriptors"], True)
        self.assert_is_sqlite("imports0_watchonly")
        assert_equal(watchonly_info["private_keys_enabled"], False)
        watchonly.gettransaction(txid)
        assert_equal(watchonly.getbalance(), bal)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", watchonly.gettransaction, txid2)

    def test_no_privkeys(self):
        default = self.nodes[0].get_wallet_rpc("")

        # Migrating an actual watchonly wallet should not create a new watchonly wallet
        self.log.info("Test migration of a pure watchonly wallet")
        self.nodes[0].createwallet(wallet_name="watchonly0", disable_private_keys=True)
        watchonly0 = self.nodes[0].get_wallet_rpc("watchonly0")
        info = watchonly0.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["private_keys_enabled"], False)

        addr = default.getnewaddress()
        desc = default.getaddressinfo(addr)["desc"]
        res = watchonly0.importmulti([
            {
                "desc": desc,
                "watchonly": True,
                "timestamp": "now",
            }])
        assert_equal(res[0]['success'], True)
        default.sendtoaddress(addr, 10)
        self.nodes[0].generate(1)

        watchonly0.migratewallet()
        assert_equal("watchonly0_watchonly" in self.nodes[0].listwallets(), False)
        info = watchonly0.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["private_keys_enabled"], False)
        self.assert_is_sqlite("watchonly0")

        # Migrating a wallet with pubkeys added to the keypool
        self.log.info("Test migration of a pure watchonly wallet with pubkeys in keypool")
        self.nodes[0].createwallet(wallet_name="watchonly1", disable_private_keys=True)
        watchonly1 = self.nodes[0].get_wallet_rpc("watchonly1")
        info = watchonly1.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["private_keys_enabled"], False)

        addr1 = default.getnewaddress(address_type="bech32")
        addr2 = default.getnewaddress(address_type="bech32")
        desc1 = default.getaddressinfo(addr1)["desc"]
        desc2 = default.getaddressinfo(addr2)["desc"]
        res = watchonly1.importmulti([
            {
                "desc": desc1,
                "keypool": True,
                "timestamp": "now",
            },
            {
                "desc": desc2,
                "keypool": True,
                "timestamp": "now",
            }
        ])
        assert_equal(res[0]["success"], True)
        assert_equal(res[1]["success"], True)
        # Before migrating, we can fetch addr1 from the keypool
        assert_equal(watchonly1.getnewaddress(address_type="bech32"), addr1)

        watchonly1.migratewallet()
        info = watchonly1.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["private_keys_enabled"], False)
        self.assert_is_sqlite("watchonly1")
        # After migrating, the "keypool" is empty
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", watchonly1.getnewaddress)

    def run_test(self):
        self.nodes[0].generate(101)

        # TODO: Test the actual records in the wallet for these tests too. The behavior may be correct, but the data written may not be what we actually want
        self.test_basic()
        self.test_multisig()
        self.test_other_watchonly()
        self.test_no_privkeys()

if __name__ == '__main__':
    WalletMigrationTest().main()
