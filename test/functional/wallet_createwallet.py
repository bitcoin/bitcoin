#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test createwallet arguments.
"""

from test_framework.address import key_to_p2wpkh
from test_framework.descriptors import descsum_create
from test_framework.key import ECKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import bytes_to_wif, generate_wif_key

EMPTY_PASSPHRASE_MSG = "Empty string given as passphrase, wallet will not be encrypted."
LEGACY_WALLET_MSG = "Wallet created successfully. The legacy wallet type is being deprecated and support for creating and opening legacy wallets will be removed in the future."


class CreateWalletTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-deprecatedrpc=walletwarningfield"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        self.generate(node, 1) # Leave IBD for sethdseed

        self.log.info("Run createwallet with invalid parameters.")
        # Run createwallet with invalid parameters. This must not prevent a new wallet with the same name from being created with the correct parameters.
        assert_raises_rpc_error(-4, "Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.",
            self.nodes[0].createwallet, wallet_name='w0', disable_private_keys=True, passphrase="passphrase")

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
        eckey = ECKey()
        eckey.generate()
        privkey = bytes_to_wif(eckey.get_bytes())
        assert_raises_rpc_error(-4, 'Cannot import private keys to a wallet with private keys disabled', w1.importprivkey, privkey)
        if self.options.descriptors:
            result = w1.importdescriptors([{'desc': descsum_create('wpkh(' + privkey + ')'), 'timestamp': 'now'}])
        else:
            result = w1.importmulti([{'scriptPubKey': {'address': key_to_p2wpkh(eckey.get_pubkey().get_bytes())}, 'timestamp': 'now', 'keys': [privkey]}])
        assert not result[0]['success']
        assert 'warnings' not in result[0]
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
        w3.importprivkey(generate_wif_key())
        # Imported private keys are currently ignored by the keypool
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", w3.getnewaddress)
        # Set the seed
        if self.options.descriptors:
            w3.importdescriptors([{
                'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPcwuZGKp8TeWppSuLMiLe2d9PupB14QpPeQsqoj3LneJLhGHH13xESfvASyd4EFLJvLrG8b7DrLxEuV7hpF9uUc6XruKA1Wq/0h/*)'),
                'timestamp': 'now',
                'active': True
            },
            {
                'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPcwuZGKp8TeWppSuLMiLe2d9PupB14QpPeQsqoj3LneJLhGHH13xESfvASyd4EFLJvLrG8b7DrLxEuV7hpF9uUc6XruKA1Wq/1h/*)'),
                'timestamp': 'now',
                'active': True,
                'internal': True
            }])
        else:
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
        w4.walletpassphrase('pass', 60)
        if self.options.descriptors:
            w4.importdescriptors([{
                'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPcwuZGKp8TeWppSuLMiLe2d9PupB14QpPeQsqoj3LneJLhGHH13xESfvASyd4EFLJvLrG8b7DrLxEuV7hpF9uUc6XruKA1Wq/0h/*)'),
                'timestamp': 'now',
                'active': True
            },
            {
                'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPcwuZGKp8TeWppSuLMiLe2d9PupB14QpPeQsqoj3LneJLhGHH13xESfvASyd4EFLJvLrG8b7DrLxEuV7hpF9uUc6XruKA1Wq/1h/*)'),
                'timestamp': 'now',
                'active': True,
                'internal': True
            }])
        else:
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
        wblank.walletpassphrase('thisisapassphrase', 60)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", wblank.getnewaddress)
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", wblank.getrawchangeaddress)

        self.log.info('Test creating a new encrypted wallet.')
        # Born encrypted wallet is created (has keys)
        self.nodes[0].createwallet(wallet_name='w6', disable_private_keys=False, blank=False, passphrase='thisisapassphrase')
        w6 = node.get_wallet_rpc('w6')
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first.", w6.signmessage, "needanargument", "test")
        w6.walletpassphrase('thisisapassphrase', 60)
        w6.signmessage(w6.getnewaddress('', 'legacy'), "test")
        w6.keypoolrefill(1)
        # There should only be 1 key for legacy, 3 for descriptors
        walletinfo = w6.getwalletinfo()
        keys = 4 if self.options.descriptors else 1
        assert_equal(walletinfo['keypoolsize'], keys)
        assert_equal(walletinfo['keypoolsize_hd_internal'], keys)
        # Allow empty passphrase, but there should be a warning
        resp = self.nodes[0].createwallet(wallet_name='w7', disable_private_keys=False, blank=False, passphrase='')
        assert_equal(resp["warning"], EMPTY_PASSPHRASE_MSG if self.options.descriptors else f"{EMPTY_PASSPHRASE_MSG}\n{LEGACY_WALLET_MSG}")
        assert_equal(resp["warnings"], [EMPTY_PASSPHRASE_MSG] if self.options.descriptors else [EMPTY_PASSPHRASE_MSG, LEGACY_WALLET_MSG])

        w7 = node.get_wallet_rpc('w7')
        assert_raises_rpc_error(-15, 'Error: running with an unencrypted wallet, but walletpassphrase was called.', w7.walletpassphrase, '', 60)

        self.log.info('Test making a wallet with avoid reuse flag')
        self.nodes[0].createwallet('w8', False, False, '', True) # Use positional arguments to check for bug where avoid_reuse could not be set for wallets without needing them to be encrypted
        w8 = node.get_wallet_rpc('w8')
        assert_raises_rpc_error(-15, 'Error: running with an unencrypted wallet, but walletpassphrase was called.', w7.walletpassphrase, '', 60)
        assert_equal(w8.getwalletinfo()["avoid_reuse"], True)

        self.log.info('Using a passphrase with private keys disabled returns error')
        assert_raises_rpc_error(-4, 'Passphrase provided but private keys are disabled. A passphrase is only used to encrypt private keys, so cannot be used for wallets with private keys disabled.', self.nodes[0].createwallet, wallet_name='w9', disable_private_keys=True, passphrase='thisisapassphrase')

        if self.is_bdb_compiled():
            self.log.info("Test legacy wallet deprecation")
            result = self.nodes[0].createwallet(wallet_name="legacy_w0", descriptors=False, passphrase=None)
            assert_equal(result, {
                "name": "legacy_w0",
                "warning": LEGACY_WALLET_MSG,
                "warnings": [LEGACY_WALLET_MSG],
            })
            result = self.nodes[0].createwallet(wallet_name="legacy_w1", descriptors=False, passphrase="")
            assert_equal(result, {
                "name": "legacy_w1",
                "warning": f"{EMPTY_PASSPHRASE_MSG}\n{LEGACY_WALLET_MSG}",
                "warnings": [EMPTY_PASSPHRASE_MSG, LEGACY_WALLET_MSG],
            })

        self.log.info('Test "warning" field deprecation, i.e. not returned without -deprecatedrpc=walletwarningfield')
        self.restart_node(0, extra_args=[])
        result = self.nodes[0].createwallet(wallet_name="w7_again", disable_private_keys=False, blank=False, passphrase="")
        assert "warning" not in result


if __name__ == '__main__':
    CreateWalletTest().main()
