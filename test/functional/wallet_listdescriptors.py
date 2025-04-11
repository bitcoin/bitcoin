#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listdescriptors RPC."""

from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
)
from test_framework.descriptors import (
    descsum_create,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
    assert_raises_rpc_error,
)


class ListDescriptorsTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    # do not create any wallet by default
    def init_wallet(self, *, node):
        return

    def test_listdescriptors_with_partial_privkeys(self):
        self.nodes[0].createwallet(wallet_name="partialkeys_descs", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("partialkeys_descs")

        self.log.info("Test listdescriptors(private=true) with descriptors containing partial private keys")
        descs = [
            descsum_create("wsh(multi(2,tprv8ZgxMBicQKsPdzuc344mDaeUk5zseMcRK9Hst8xodskNu3YbQG5NxLa2X17PUU5yXQhptiBE7F5W5cgEmsfQg4Y21Y18w4DJhLxSb8CurDf,tpubD6NzVbkrYhZ4YiCvExLvH4yh1k3jFGf5irm6TsrArY8GYdEhYVdztQTBtTirmRc6XfSJpH9tayUdnngaJZKDaa2zbqEY29DfcGZW8iRVGUY))"),
            descsum_create("wsh(thresh(2,pk(tprv8ZgxMBicQKsPdzuc344mDaeUk5zseMcRK9Hst8xodskNu3YbQG5NxLa2X17PUU5yXQhptiBE7F5W5cgEmsfQg4Y21Y18w4DJhLxSb8CurDf),s:pk(tpubD6NzVbkrYhZ4YiCvExLvH4yh1k3jFGf5irm6TsrArY8GYdEhYVdztQTBtTirmRc6XfSJpH9tayUdnngaJZKDaa2zbqEY29DfcGZW8iRVGUY),sln:older(2)))"),
            descsum_create("tr(03d1d1110030000000000120000000000000000000000000001370010912cd08cc,pk(cNKAo2ZRsaWKcP481cEfj3astPyBrfq56JBtLeRhHUvTSuk2z4MR))"),
            descsum_create("tr(cNKAo2ZRsaWKcP481cEfj3astPyBrfq56JBtLeRhHUvTSuk2z4MR,pk(03d1d1110030000000000120000000000000000000000000001370010912cd08cc))")
        ]
        importdescriptors_request = list(map(lambda desc: {"desc": desc, "timestamp": "now"}, descs))
        res = wallet.importdescriptors(importdescriptors_request)
        for imported_desc in res:
            assert_equal(imported_desc["success"], True)
        res = wallet.listdescriptors(private=True)
        for listed_desc in res["descriptors"]:
            # descriptors are not returned in the order they were present in the import request
            assert_equal(listed_desc["desc"], next(desc for desc in descs if listed_desc["desc"] == desc))

    def run_test(self):
        node = self.nodes[0]
        assert_raises_rpc_error(-18, 'No wallet is loaded.', node.listdescriptors)

        if self.is_bdb_compiled():
            self.log.info('Test that the command is not available for legacy wallets.')
            node.createwallet(wallet_name='w1', descriptors=False)
            assert_raises_rpc_error(-4, 'listdescriptors is not available for non-descriptor wallets', node.listdescriptors)

        self.log.info('Test the command for empty descriptors wallet.')
        node.createwallet(wallet_name='w2', blank=True, descriptors=True)
        assert_equal(0, len(node.get_wallet_rpc('w2').listdescriptors()['descriptors']))

        self.log.info('Test the command for a default descriptors wallet.')
        node.createwallet(wallet_name='w3', descriptors=True)
        result = node.get_wallet_rpc('w3').listdescriptors()
        assert_equal("w3", result['wallet_name'])
        assert_equal(8, len(result['descriptors']))
        assert_equal(8, len([d for d in result['descriptors'] if d['active']]))
        assert_equal(4, len([d for d in result['descriptors'] if d['internal']]))
        for item in result['descriptors']:
            assert_not_equal(item['desc'], '')
            assert item['next_index'] == 0
            assert item['range'] == [0, 0]
            assert item['timestamp'] is not None

        self.log.info('Test that descriptor strings are returned in lexicographically sorted order.')
        descriptor_strings = [descriptor['desc'] for descriptor in result['descriptors']]
        assert_equal(descriptor_strings, sorted(descriptor_strings))

        self.log.info('Test descriptors with hardened derivations are listed in importable form.')
        xprv = 'tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg'
        xpub_acc = 'tpubDCMVLhErorrAGfApiJSJzEKwqeaf2z3NrkVMxgYQjZLzMjXMBeRw2muGNYbvaekAE8rUFLftyEar4LdrG2wXyyTJQZ26zptmeTEjPTaATts'
        hardened_path = '/84h/1h/0h'
        wallet = node.get_wallet_rpc('w2')
        wallet.importdescriptors([{
            'desc': descsum_create('wpkh(' + xprv + hardened_path + '/0/*)'),
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        expected = {
            'wallet_name': 'w2',
            'descriptors': [
                {'desc': descsum_create('wpkh([80002067' + hardened_path + ']' + xpub_acc + '/0/*)'),
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
        node.createwallet(wallet_name='watch-only', descriptors=True, disable_private_keys=True)
        watch_only_wallet = node.get_wallet_rpc('watch-only')
        watch_only_wallet.importdescriptors([{
            'desc': descsum_create('wpkh(' + xpub_acc + ')'),
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        assert_raises_rpc_error(-4, 'Can\'t get descriptor string', watch_only_wallet.listdescriptors, True)

        self.log.info('Test non-active non-range combo descriptor')
        node.createwallet(wallet_name='w4', blank=True, descriptors=True)
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
            ]
        }
        assert_equal(expected, wallet.listdescriptors())
        self.test_listdescriptors_with_partial_privkeys()


if __name__ == '__main__':
    ListDescriptorsTest(__file__).main()
