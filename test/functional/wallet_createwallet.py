#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test createwallet arguments.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class CreateWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.supports_cli = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        node.generate(1) # Leave IBD for sethdseed

        self.nodes[0].createwallet(wallet_name='w0')
        w0 = node.get_wallet_rpc('w0')
        address1 = w0.getnewaddress()

        self.log.info("Test disableprivatekeys creation.")
        self.nodes[0].createwallet(wallet_name='w1', disable_private_keys=True)
        w1 = node.get_wallet_rpc('w1')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w1.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w1.getrawchangeaddress)
        w1.importpubkey(w0.getaddressinfo(address1)['pubkey'])

        self.log.info('Test that private keys cannot be imported')
        addr = w0.getnewaddress('', 'legacy')
        privkey = w0.dumpprivkey(addr)
        assert_raises_rpc_error(-4, 'Cannot import private keys to a wallet with private keys disabled', w1.importprivkey, privkey)
        result = w1.importmulti([{'scriptPubKey': {'address': addr}, 'timestamp': 'now', 'keys': [privkey]}])
        assert not result[0]['success']
        assert 'warning' not in result[0]
        assert_equal(result[0]['error']['code'], -4)
        assert_equal(result[0]['error']['message'], 'Cannot import private keys to a wallet with private keys disabled')

        self.log.info("Test blank creation with private keys disabled.")
        self.nodes[0].createwallet(wallet_name='w2', disable_private_keys=True, blank=True)
        w2 = node.get_wallet_rpc('w2')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w2.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w2.getrawchangeaddress)
        w2.importpubkey(w0.getaddressinfo(address1)['pubkey'])

        self.log.info("Test blank creation with private keys enabled.")
        self.nodes[0].createwallet(wallet_name='w3', disable_private_keys=False, blank=True)
        w3 = node.get_wallet_rpc('w3')
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getrawchangeaddress)
        # Import private key
        w3.importprivkey(w0.dumpprivkey(address1))
        # Imported private keys are currently ignored by the keypool
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getnewaddress)
        # Set the seed
        w3.sethdseed()
        assert_equal(w3.getwalletinfo()['keypoolsize'], 1)
        w3.getnewaddress()
        w3.getrawchangeaddress()

        self.log.info("Test blank creation with privkeys enabled and then encryption")
        self.nodes[0].createwallet(wallet_name='w4', disable_private_keys=False, blank=True)
        w4 = node.get_wallet_rpc('w4')
        assert_equal(w4.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getrawchangeaddress)
        # Encrypt the wallet. Nothing should change about the keypool
        w4.encryptwallet('pass')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w4.getrawchangeaddress)
        # Now set a seed and it should work. Wallet should also be encrypted
        w4.walletpassphrase('pass', 2)
        w4.sethdseed()
        w4.getnewaddress()
        w4.getrawchangeaddress()

        self.log.info("Test blank creation with privkeys disabled and then encryption")
        self.nodes[0].createwallet(wallet_name='w5', disable_private_keys=True, blank=True)
        w5 = node.get_wallet_rpc('w5')
        assert_equal(w5.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getrawchangeaddress)
        # Encrypt the wallet
        assert_raises_rpc_error(-16, "Error: wallet does not contain private keys, nothing to encrypt.", w5.encryptwallet, 'pass')
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w5.getrawchangeaddress)

        self.log.info('New blank and encrypted wallets can be created')
        self.nodes[0].createwallet(wallet_name='wblank', disable_private_keys=False, blank=True, passphrase='thisisapassphrase')
        wblank = node.get_wallet_rpc('wblank')
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", wblank.signmessage, "needanargument", "test")
        wblank.walletpassphrase('thisisapassphrase', 10)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", wblank.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", wblank.getrawchangeaddress)

        self.log.info('Test creating a new encrypted wallet.')
        # Born encrypted wallet is created (has keys)
        self.nodes[0].createwallet(wallet_name='w6', disable_private_keys=False, blank=False, passphrase='thisisapassphrase')
        w6 = node.get_wallet_rpc('w6')
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", w6.signmessage, "needanargument", "test")
        w6.walletpassphrase('thisisapassphrase', 10)
        w6.signmessage(w6.getnewaddress('', 'legacy'), "test")
        w6.keypoolrefill(1)
        # There should only be 1 key
        walletinfo = w6.getwalletinfo()
        assert_equal(walletinfo['keypoolsize'], 1)
        assert_equal(walletinfo['keypoolsize_hd_internal'], 1)
        # Allow empty passphrase, but there should be a warning
        resp = self.nodes[0].createwallet(wallet_name='w7', disable_private_keys=False, blank=False, passphrase='')
        assert_equal(resp['warning'], 'Empty string given as passphrase, wallet will not be encrypted.')
        w7 = node.get_wallet_rpc('w7')
        assert_raises_rpc_error(-15, 'Error: running with an unencrypted wallet, but walletpassphrase was called.', w7.walletpassphrase, '', 10)

        self.log.info('Test making a wallet with avoid reuse flag')
        self.nodes[0].createwallet('w8', False, False, '', True) # Use positional arguments to check for bug where avoid_reuse could not be set for wallets without needing them to be encrypted
        w8 = node.get_wallet_rpc('w8')
        assert_raises_rpc_error(-15, 'Error: running with an unencrypted wallet, but walletpassphrase was called.', w7.walletpassphrase, '', 10)
        assert_equal(w8.getwalletinfo()["avoid_reuse"], True)

        self.log.info('Using a passphrase with private keys disabled returns error')
        assert_raises_rpc_error(-4, 'Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.', self.nodes[0].createwallet, wallet_name='w9', disable_private_keys=True, passphrase='thisisapassphrase')

if __name__ == '__main__':
    CreateWalletTest().main()
