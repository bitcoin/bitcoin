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
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_not_equal,
    assert_equal,
    assert_raises_rpc_error,
)


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
        wallet.importdescriptors([{
            'desc': "tr([1dce71b2/48'/1'/0'/2']tpubDEeP3GefjqbaDTTaVAF5JkXWhoFxFDXQ9KuhVrMBViFXXNR2B3Lvme2d2AoyiKfzRFZChq2AGMNbU1qTbkBMfNv7WGVXLt2pnYXY87gXqcs/0/*,and_v(v:pk([c658b283/48'/1'/0'/2']tpubDFL5wzgPBYK5pZ2Kh1T8qrxnp43kjE5CXfguZHHBrZSWpkfASy5rVfj7prh11XdqkC1P3kRwUPBeX7AHN8XBNx8UwiprnFnEm5jyswiRD4p/0/*),older(65535)))#xl20m6md",
            'timestamp': TIME_GENESIS_BLOCK,
        }])
        expected = {
            'wallet_name': 'w5',
            'descriptors': [
                {'active': False,
                 'desc': 'tr([1dce71b2/48h/1h/0h/2h]tpubDEeP3GefjqbaDTTaVAF5JkXWhoFxFDXQ9KuhVrMBViFXXNR2B3Lvme2d2AoyiKfzRFZChq2AGMNbU1qTbkBMfNv7WGVXLt2pnYXY87gXqcs/0/*,and_v(v:pk([c658b283/48h/1h/0h/2h]tpubDFL5wzgPBYK5pZ2Kh1T8qrxnp43kjE5CXfguZHHBrZSWpkfASy5rVfj7prh11XdqkC1P3kRwUPBeX7AHN8XBNx8UwiprnFnEm5jyswiRD4p/0/*),older(65535)))#m4uznndk',
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

        expected_descs = {
            descsum_create('tr(' + node.get_deterministic_priv_key().key +
                ',{pk(03cdabb7f2dce7bfbd8a0b9570c6fd1e712e5d64045e9d6b517b3d5072251dc204)' +
                ',pk([d34db33f/44h/0h/0h]tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/0)})'),
            descsum_create('wsh(and_v(v:ripemd160(095ff41131e5946f3c85f79e44adbcf8e27e080e),multi(1,' + node.get_deterministic_priv_key().key +
                ',tpubD6NzVbkrYhZ4WaWSyoBvQwbpLkojyoTZPRsgXELWz3Popb3qkjcJyJUGLnL4qHHoQvao8ESaAstxYSnhyswJ76uZPStJRJCTKvosUCJZL5B/0)))'),
            descsum_create('tr(03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,pk(musig(tprv8ZgxMBicQKsPeNLUGrbv3b7qhUk1LQJZAGMuk9gVuKh9sd4BWGp1eMsehUni6qGb8bjkdwBxCbgNGdh2bYGACK5C5dRTaif9KBKGVnSezxV,tpubD6NzVbkrYhZ4XcACN3PEwNjRpR1g4tZjBVk5pdMR2B6dbd3HYhdGVZNKofAiFZd9okBserZvv58A6tBX4pE64UpXGNTSesfUW7PpW36HuKz)/7/8/*))'),
            descsum_create('tr(03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,pk(musig(tprv8ZgxMBicQKsPeNLUGrbv3b7qhUk1LQJZAGMuk9gVuKh9sd4BWGp1eMsehUni6qGb8bjkdwBxCbgNGdh2bYGACK5C5dRTaif9KBKGVnSezxV/10,tpubD6NzVbkrYhZ4XcACN3PEwNjRpR1g4tZjBVk5pdMR2B6dbd3HYhdGVZNKofAiFZd9okBserZvv58A6tBX4pE64UpXGNTSesfUW7PpW36HuKz/11)/*))'),
            descsum_create('tr(03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,{pk(musig(tpubD6NzVbkrYhZ4Wo2WcFSgSqRD9QWkGxddo6WSqsVBx7uQ8QEtM7WncKDRjhFEexK119NigyCsFygA4b7sAPQxqebyFGAZ9XVV1BtcgNzbCRR,tprv8ZgxMBicQKsPen4PGtDwURYnCtVMDejyE8vVwMGhQWfVqB2FBPdekhTacDW4vmsKTsgC1wsncVqXiZdX2YFGAnKoLXYf42M78fQJFzuDYFN)/12/*),pk(musig(tprv8ZgxMBicQKsPeNLUGrbv3b7qhUk1LQJZAGMuk9gVuKh9sd4BWGp1eMsehUni6qGb8bjkdwBxCbgNGdh2bYGACK5C5dRTaif9KBKGVnSezxV,tpubD6NzVbkrYhZ4XWb6fGPjyhgLxapUhXszv7ehQYrQWDgDX4nYWcNcbgWcM2RhYo9s2mbZcfZJ8t5LzYcr24FK79zVybsw5Qj3Rtqug8jpJMy)/13/*)})'),
            descsum_create('tr(03dff1d77f2a671c5f36183726db2341be58feae1da2deced843240f7b502ba659,{pk(musig(tpubD6NzVbkrYhZ4Wo2WcFSgSqRD9QWkGxddo6WSqsVBx7uQ8QEtM7WncKDRjhFEexK119NigyCsFygA4b7sAPQxqebyFGAZ9XVV1BtcgNzbCRR,tpubD6NzVbkrYhZ4Wc3i6L6N1Pp7cyVeyMcdLrFGXGDGzCfdCa5F4Zs3EY46N72Ws8QDEUYBVwXfDfda2UKSseSdU1fsBegJBhGCZyxkf28bkQ6)/12/*),pk(musig(tprv8ZgxMBicQKsPeNLUGrbv3b7qhUk1LQJZAGMuk9gVuKh9sd4BWGp1eMsehUni6qGb8bjkdwBxCbgNGdh2bYGACK5C5dRTaif9KBKGVnSezxV,tpubD6NzVbkrYhZ4XWb6fGPjyhgLxapUhXszv7ehQYrQWDgDX4nYWcNcbgWcM2RhYo9s2mbZcfZJ8t5LzYcr24FK79zVybsw5Qj3Rtqug8jpJMy)/13/*)})')
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
