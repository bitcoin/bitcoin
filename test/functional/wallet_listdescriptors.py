#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listdescriptors RPC."""

from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
)
from test_framework.descriptors import (
    descsum_create,
)
from test_framework.extendedkey import ExtendedPrivateKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import generate_keypair


class ListDescriptorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    # do not create any wallet by default
    def init_wallet(self, *, node):
        return

    def run_test(self):
        node = self.nodes[0]
        assert_raises_rpc_error(-18, 'No wallet is loaded.', node.listdescriptors)

        self.log.info('Test the command for empty descriptors wallet.')
        node.createwallet(wallet_name='w2', blank=True)
        assert_equal(0, len(node.get_wallet_rpc('w2').listdescriptors()['descriptors']))

        self.log.info('Test the command for a default descriptors wallet.')
        node.createwallet(wallet_name='w3')
        result = node.get_wallet_rpc('w3').listdescriptors()
        assert_equal("w3", result['wallet_name'])
        assert_equal(8, len(result['descriptors']))
        assert_equal(8, len([d for d in result['descriptors'] if d['active']]))
        assert_equal(4, len([d for d in result['descriptors'] if d['internal']]))
        for item in result['descriptors']:
            assert_not_equal(item['desc'], '')
            assert_equal(item['next_index'], 0)
            assert_equal(item['range'], [0, 0])
            assert item['timestamp'] is not None

        self.log.info('Test that descriptor strings are returned in lexicographically sorted order.')
        descriptor_strings = [descriptor['desc'] for descriptor in result['descriptors']]
        assert_equal(descriptor_strings, sorted(descriptor_strings))

        self.log.info('Test descriptors with hardened derivations are listed in importable form.')
        extended_key = ExtendedPrivateKey.generate()
        xprv = extended_key.to_string()
        xprv_fingerprint = extended_key._fingerprint().hex()
        hardened_path = '/84h/1h/0h'
        xpub_acc = extended_key.derive_path(f"m{hardened_path}").pubkey().to_string()
        wallet = node.get_wallet_rpc('w2')
        wallet.importdescriptors([{
            'desc': descsum_create('wpkh(' + xprv + hardened_path + '/0/*)'),
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        expected = {
            'wallet_name': 'w2',
            'descriptors': [
                {'desc': descsum_create('wpkh([' + xprv_fingerprint + hardened_path + ']' + xpub_acc + '/0/*)'),
                 'timestamp': TIME_GENESIS_BLOCK,
                 'active': False,
                 'range': [0, 0],
                 'next': 0,
                 'next_index': 0},
            ],
        }
        assert_equal(expected, wallet.listdescriptors())
        assert_equal(expected, wallet.listdescriptors(False))

        self.log.info('Test list private descriptors')
        expected_private = {
            'wallet_name': 'w2',
            'descriptors': [
                {'desc': descsum_create('wpkh(' + xprv + hardened_path + '/0/*)'),
                 'timestamp': TIME_GENESIS_BLOCK,
                 'active': False,
                 'range': [0, 0],
                 'next': 0,
                 'next_index': 0},
            ],
        }
        assert_equal(expected_private, wallet.listdescriptors(True))

        self.log.info("Test listdescriptors with encrypted wallet")
        wallet.encryptwallet("pass")
        assert_equal(expected, wallet.listdescriptors())

        self.log.info('Test list private descriptors with encrypted wallet')
        assert_raises_rpc_error(-13, 'Please enter the wallet passphrase with walletpassphrase first.', wallet.listdescriptors, True)
        wallet.walletpassphrase(passphrase="pass", timeout=1000000)
        assert_equal(expected_private, wallet.listdescriptors(True))

        self.log.info('Test list private descriptors with watch-only wallet')
        node.createwallet(wallet_name='watch-only', disable_private_keys=True)
        watch_only_wallet = node.get_wallet_rpc('watch-only')
        watch_only_wallet.importdescriptors([{
            'desc': descsum_create('wpkh(' + xpub_acc + ')'),
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        assert_raises_rpc_error(-4, 'Can\'t get private descriptor string for watch-only wallets', watch_only_wallet.listdescriptors, True)

        self.log.info('Test non-active non-range combo descriptor')
        node.createwallet(wallet_name='w4', blank=True)
        wallet = node.get_wallet_rpc('w4')
        wallet.importdescriptors([{
            'desc': descsum_create('combo(' + node.get_deterministic_priv_key().key + ')'),
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        expected = {
            'wallet_name': 'w4',
            'descriptors': [
                {'active': False,
                 'desc': 'combo(0227d85ba011276cf25b51df6a188b75e604b38770a462b2d0e9fb2fc839ef5d3f)#np574htj',
                 'timestamp': TIME_GENESIS_BLOCK},
            ],
        }
        assert_equal(expected, wallet.listdescriptors())

        self.log.info('Test taproot descriptor do not have mixed hardened derivation marker')
        node.createwallet(wallet_name='w5', descriptors=True, disable_private_keys=True)
        wallet = node.get_wallet_rpc('w5')
        derivation_path = "/48'/1'/0'/2'"
        xprvs = [ExtendedPrivateKey.generate() for _ in range(0, 2)]
        xprv_fingerprints = [xprv._fingerprint().hex() for xprv in xprvs]
        desc_xpubs = [xprv.derive_path(f"m{derivation_path}").pubkey().to_string() for xprv in xprvs]
        wallet.importdescriptors([{
            'desc': descsum_create(f"tr([{xprv_fingerprints[0]}{derivation_path}]{desc_xpubs[0]}/0/*,and_v(v:pk([{xprv_fingerprints[1]}{derivation_path}]{desc_xpubs[1]}/0/*),older(65535)))"),
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        derivation_path_alternate = "/48h/1h/0h/2h"
        expected = {
            'wallet_name': 'w5',
            'descriptors': [
                {'active': False,
                 'desc': descsum_create(f'tr([{xprv_fingerprints[0]}{derivation_path_alternate}]{desc_xpubs[0]}/0/*,and_v(v:pk([{xprv_fingerprints[1]}{derivation_path_alternate}]{desc_xpubs[1]}/0/*),older(65535)))'),
                 'timestamp': TIME_GENESIS_BLOCK,
                 'range': [0, 0],
                 'next': 0,
                 'next_index': 0},
            ]
        }
        assert_equal(expected, wallet.listdescriptors())

        self.log.info('Test descriptor with missing private keys')
        node.createwallet(wallet_name='w6', blank=True)
        wallet = node.get_wallet_rpc('w6')

        xprvs = []
        xpubs = []
        for _ in range(0, 8):
            xprvs.append(ExtendedPrivateKey.generate().to_string())
            xpubs.append(ExtendedPrivateKey.generate().pubkey().to_string())

        _, pubkey_bytes = generate_keypair()
        pubkey = pubkey_bytes.hex()

        expected_descs = {
            descsum_create(f'tr({node.get_deterministic_priv_key().key},{{pk({pubkey}),pk([d34db33f/44h/0h/0h]{xpubs[0]}/0)}})'),
            descsum_create(f'wsh(and_v(v:ripemd160(095ff41131e5946f3c85f79e44adbcf8e27e080e),multi(1,{node.get_deterministic_priv_key().key},{xpubs[1]}/0)))'),
            descsum_create(f'tr({pubkey},pk(musig({xprvs[0]},{xpubs[2]})/7/8/*))'),
            descsum_create(f'tr({pubkey},pk(musig({xprvs[1]}/10,{xpubs[3]}/11)/*))'),
            descsum_create(f'tr({pubkey},{{pk(musig({xpubs[4]},{xprvs[2]})/12/*),pk(musig({xprvs[3]},{xpubs[4]})/13/*)}})'),
            descsum_create(f'tr({pubkey},{{pk(musig({xpubs[5]},{xpubs[6]})/12/*),pk(musig({xprvs[4]},{xpubs[7]})/13/*)}})')
        }

        descs_to_import = []
        for desc in expected_descs:
            descs_to_import.append({'desc': desc, 'timestamp': TIME_GENESIS_BLOCK})

        wallet.importdescriptors(descs_to_import)
        result = wallet.listdescriptors(True)
        actual_descs = [d['desc'] for d in result['descriptors']]

        assert_equal(len(actual_descs), len(expected_descs))
        for desc in actual_descs:
            if desc not in expected_descs:
                raise AssertionError(f"{desc} missing")



if __name__ == '__main__':
    ListDescriptorsTest(__file__).main()
