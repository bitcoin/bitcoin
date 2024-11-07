#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test descriptor wallet function."""

try:
    import sqlite3
except ImportError:
    pass

import concurrent.futures

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error
)
from test_framework.wallet_util import WalletUnlock


class WalletDescriptorTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-keypool=100']]
        self.wallet_names = []

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()
        self.skip_if_no_py_sqlite3()

    def test_concurrent_writes(self):
        self.log.info("Test sqlite concurrent writes are in the correct order")
        self.restart_node(0, extra_args=["-unsafesqlitesync=0"])
        self.nodes[0].createwallet(wallet_name="concurrency", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("concurrency")
        # First import a descriptor that uses hardened dervation so that topping up
        # Will require writing a ton to db
        wallet.importdescriptors([{"desc":descsum_create("wpkh(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/0h/0h/*h)"), "timestamp": "now", "active": True}])
        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as thread:
            topup = thread.submit(wallet.keypoolrefill, newsize=1000)

            # Then while the topup is running, we need to do something that will call
            # ChainStateFlushed which will trigger a write to the db, hopefully at the
            # same time that the topup still has an open db transaction.
            self.nodes[0].cli.gettxoutsetinfo()
            assert_equal(topup.result(), None)

        wallet.unloadwallet()

        # Check that everything was written
        wallet_db = self.nodes[0].wallets_path / "concurrency" / self.wallet_data_filename
        conn = sqlite3.connect(wallet_db)
        with conn:
            # Retrieve the bestblock_nomerkle record
            bestblock_rec = conn.execute("SELECT value FROM main WHERE hex(key) = '1262657374626C6F636B5F6E6F6D65726B6C65'").fetchone()[0]
            # Retrieve the number of descriptor cache records
            # Since we store binary data, sqlite's comparison operators don't work everywhere
            # so just retrieve all records and process them ourselves.
            db_keys = conn.execute("SELECT key FROM main").fetchall()
            cache_records = len([k[0] for k in db_keys if b"walletdescriptorcache" in k[0]])
        conn.close()

        assert_equal(bestblock_rec[5:37][::-1].hex(), self.nodes[0].getbestblockhash())
        assert_equal(cache_records, 1000)

    def run_test(self):
        if self.is_bdb_compiled():
            # Make a legacy wallet and check it is BDB
            self.nodes[0].createwallet(wallet_name="legacy1", descriptors=False)
            wallet_info = self.nodes[0].getwalletinfo()
            assert_equal(wallet_info['format'], 'bdb')
            self.nodes[0].unloadwallet("legacy1")
        else:
            self.log.warning("Skipping BDB test")

        # Make a descriptor wallet
        self.log.info("Making a descriptor wallet")
        self.nodes[0].createwallet(wallet_name="desc1", descriptors=True)

        # A descriptor wallet should have 100 addresses * 4 types = 400 keys
        self.log.info("Checking wallet info")
        wallet_info = self.nodes[0].getwalletinfo()
        assert_equal(wallet_info['format'], 'sqlite')
        assert_equal(wallet_info['keypoolsize'], 400)
        assert_equal(wallet_info['keypoolsize_hd_internal'], 400)
        assert 'keypoololdest' not in wallet_info

        # Check that getnewaddress works
        self.log.info("Test that getnewaddress and getrawchangeaddress work")
        addr = self.nodes[0].getnewaddress("", "legacy")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('pkh(')
        assert_equal(addr_info['hdkeypath'], 'm/44h/1h/0h/0/0')

        addr = self.nodes[0].getnewaddress("", "p2sh-segwit")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('sh(wpkh(')
        assert_equal(addr_info['hdkeypath'], 'm/49h/1h/0h/0/0')

        addr = self.nodes[0].getnewaddress("", "bech32")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('wpkh(')
        assert_equal(addr_info['hdkeypath'], 'm/84h/1h/0h/0/0')

        addr = self.nodes[0].getnewaddress("", "bech32m")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('tr(')
        assert_equal(addr_info['hdkeypath'], 'm/86h/1h/0h/0/0')

        # Check that getrawchangeaddress works
        addr = self.nodes[0].getrawchangeaddress("legacy")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('pkh(')
        assert_equal(addr_info['hdkeypath'], 'm/44h/1h/0h/1/0')

        addr = self.nodes[0].getrawchangeaddress("p2sh-segwit")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('sh(wpkh(')
        assert_equal(addr_info['hdkeypath'], 'm/49h/1h/0h/1/0')

        addr = self.nodes[0].getrawchangeaddress("bech32")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('wpkh(')
        assert_equal(addr_info['hdkeypath'], 'm/84h/1h/0h/1/0')

        addr = self.nodes[0].getrawchangeaddress("bech32m")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert addr_info['desc'].startswith('tr(')
        assert_equal(addr_info['hdkeypath'], 'm/86h/1h/0h/1/0')

        # Make a wallet to receive coins at
        self.nodes[0].createwallet(wallet_name="desc2", descriptors=True)
        recv_wrpc = self.nodes[0].get_wallet_rpc("desc2")
        send_wrpc = self.nodes[0].get_wallet_rpc("desc1")

        # Generate some coins
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, send_wrpc.getnewaddress())

        # Make transactions
        self.log.info("Test sending and receiving")
        addr = recv_wrpc.getnewaddress()
        send_wrpc.sendtoaddress(addr, 10)

        # Make sure things are disabled
        self.log.info("Test disabled RPCs")
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.importprivkey, "cVpF924EspNh8KjYsfhgY96mmxvT6DgdWiTYMtMjuM74hJaU5psW")
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.importpubkey, send_wrpc.getaddressinfo(send_wrpc.getnewaddress())["pubkey"])
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.importaddress, recv_wrpc.getnewaddress())
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.importmulti, [])
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.addmultisigaddress, 1, [recv_wrpc.getnewaddress()])
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.dumpprivkey, recv_wrpc.getnewaddress())
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.dumpwallet, 'wallet.dump')
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.importwallet, 'wallet.dump')
        assert_raises_rpc_error(-4, "Only legacy wallets are supported by this command", recv_wrpc.rpc.sethdseed)

        self.log.info("Test encryption")
        # Get the master fingerprint before encrypt
        info1 = send_wrpc.getaddressinfo(send_wrpc.getnewaddress())

        # Encrypt wallet 0
        send_wrpc.encryptwallet('pass')
        with WalletUnlock(send_wrpc, "pass"):
            addr = send_wrpc.getnewaddress()
            info2 = send_wrpc.getaddressinfo(addr)
            assert info1['hdmasterfingerprint'] != info2['hdmasterfingerprint']
        assert 'hdmasterfingerprint' in send_wrpc.getaddressinfo(send_wrpc.getnewaddress())
        info3 = send_wrpc.getaddressinfo(addr)
        assert_equal(info2['desc'], info3['desc'])

        self.log.info("Test that getnewaddress still works after keypool is exhausted in an encrypted wallet")
        for _ in range(500):
            send_wrpc.getnewaddress()

        self.log.info("Test that unlock is needed when deriving only hardened keys in an encrypted wallet")
        with WalletUnlock(send_wrpc, "pass"):
            send_wrpc.importdescriptors([{
                "desc": "wpkh(tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK/0h/*h)#y4dfsj7n",
                "timestamp": "now",
                "range": [0,10],
                "active": True
            }])
        # Exhaust keypool of 100
        for _ in range(100):
            send_wrpc.getnewaddress(address_type='bech32')
        # This should now error
        assert_raises_rpc_error(-12, "Keypool ran out, please call keypoolrefill first", send_wrpc.getnewaddress, '', 'bech32')

        self.log.info("Test born encrypted wallets")
        self.nodes[0].createwallet('desc_enc', False, False, 'pass', False, True)
        enc_rpc = self.nodes[0].get_wallet_rpc('desc_enc')
        enc_rpc.getnewaddress() # Makes sure that we can get a new address from a born encrypted wallet

        self.log.info("Test blank descriptor wallets")
        self.nodes[0].createwallet(wallet_name='desc_blank', blank=True, descriptors=True)
        blank_rpc = self.nodes[0].get_wallet_rpc('desc_blank')
        assert_raises_rpc_error(-4, 'This wallet has no available keys', blank_rpc.getnewaddress)

        self.log.info("Test descriptor wallet with disabled private keys")
        self.nodes[0].createwallet(wallet_name='desc_no_priv', disable_private_keys=True, descriptors=True)
        nopriv_rpc = self.nodes[0].get_wallet_rpc('desc_no_priv')
        assert_raises_rpc_error(-4, 'This wallet has no available keys', nopriv_rpc.getnewaddress)

        self.log.info("Test descriptor exports")
        self.nodes[0].createwallet(wallet_name='desc_export', descriptors=True)
        exp_rpc = self.nodes[0].get_wallet_rpc('desc_export')
        self.nodes[0].createwallet(wallet_name='desc_import', disable_private_keys=True, descriptors=True)
        imp_rpc = self.nodes[0].get_wallet_rpc('desc_import')

        addr_types = [('legacy', False, 'pkh(', '44h/1h/0h', -13),
                      ('p2sh-segwit', False, 'sh(wpkh(', '49h/1h/0h', -14),
                      ('bech32', False, 'wpkh(', '84h/1h/0h', -13),
                      ('bech32m', False, 'tr(', '86h/1h/0h', -13),
                      ('legacy', True, 'pkh(', '44h/1h/0h', -13),
                      ('p2sh-segwit', True, 'sh(wpkh(', '49h/1h/0h', -14),
                      ('bech32', True, 'wpkh(', '84h/1h/0h', -13),
                      ('bech32m', True, 'tr(', '86h/1h/0h', -13)]

        for addr_type, internal, desc_prefix, deriv_path, int_idx in addr_types:
            int_str = 'internal' if internal else 'external'

            self.log.info("Testing descriptor address type for {} {}".format(addr_type, int_str))
            if internal:
                addr = exp_rpc.getrawchangeaddress(address_type=addr_type)
            else:
                addr = exp_rpc.getnewaddress(address_type=addr_type)
            desc = exp_rpc.getaddressinfo(addr)['parent_desc']
            assert_equal(desc_prefix, desc[0:len(desc_prefix)])
            idx = desc.index('/') + 1
            assert_equal(deriv_path, desc[idx:idx + 9])
            if internal:
                assert_equal('1', desc[int_idx])
            else:
                assert_equal('0', desc[int_idx])

            self.log.info("Testing the same descriptor is returned for address type {} {}".format(addr_type, int_str))
            for i in range(0, 10):
                if internal:
                    addr = exp_rpc.getrawchangeaddress(address_type=addr_type)
                else:
                    addr = exp_rpc.getnewaddress(address_type=addr_type)
                test_desc = exp_rpc.getaddressinfo(addr)['parent_desc']
                assert_equal(desc, test_desc)

            self.log.info("Testing import of exported {} descriptor".format(addr_type))
            imp_rpc.importdescriptors([{
                'desc': desc,
                'active': True,
                'next_index': 11,
                'timestamp': 'now',
                'internal': internal
            }])

            for i in range(0, 10):
                if internal:
                    exp_addr = exp_rpc.getrawchangeaddress(address_type=addr_type)
                    imp_addr = imp_rpc.getrawchangeaddress(address_type=addr_type)
                else:
                    exp_addr = exp_rpc.getnewaddress(address_type=addr_type)
                    imp_addr = imp_rpc.getnewaddress(address_type=addr_type)
                assert_equal(exp_addr, imp_addr)

        self.log.info("Test that loading descriptor wallet containing legacy key types throws error")
        self.nodes[0].createwallet(wallet_name="crashme", descriptors=True)
        self.nodes[0].unloadwallet("crashme")
        wallet_db = self.nodes[0].wallets_path / "crashme" / self.wallet_data_filename
        conn = sqlite3.connect(wallet_db)
        with conn:
            # add "cscript" entry: key type is uint160 (20 bytes), value type is CScript (zero-length here)
            conn.execute('INSERT INTO main VALUES(?, ?)', (b'\x07cscript' + b'\x00'*20, b'\x00'))
        conn.close()
        assert_raises_rpc_error(-4, "Unexpected legacy entry in descriptor wallet found.", self.nodes[0].loadwallet, "crashme")

        self.test_concurrent_writes()


if __name__ == '__main__':
    WalletDescriptorTest(__file__).main()
