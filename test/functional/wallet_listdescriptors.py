#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listdescriptors RPC."""

from test_framework.descriptors import (
    descsum_create
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class ListDescriptorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    # do not create any wallet by default
    def init_wallet(self, i):
        return

    def run_test(self):
        node = self.nodes[0]
        assert_raises_rpc_error(-18, 'No wallet is loaded.', node.listdescriptors)

        self.log.info('Test that the command is not available for legacy wallets.')
        node.createwallet(wallet_name='w1', descriptors=False)
        assert_raises_rpc_error(-4, 'listdescriptors is not available for non-descriptor wallets', node.listdescriptors)

        self.log.info('Test the command for empty descriptors wallet.')
        node.createwallet(wallet_name='w2', blank=True, descriptors=True)
        assert_equal(0, len(node.get_wallet_rpc('w2').listdescriptors()))

        self.log.info('Test the command for a default descriptors wallet.')
        node.createwallet(wallet_name='w3', descriptors=True)
        result = node.get_wallet_rpc('w3').listdescriptors()
        assert_equal(6, len(result))
        assert_equal(6, len([d for d in result if d['active']]))
        assert_equal(3, len([d for d in result if d['internal']]))
        for item in result:
            assert item['desc'] != ''
            assert item['next'] == 0
            assert item['range'] == [0, 0]
            assert item['timestamp'] is not None

        self.log.info('Test non-active non-range combo descriptor')
        node.createwallet(wallet_name='w4', blank=True, descriptors=True)
        wallet = node.get_wallet_rpc('w4')
        wallet.importdescriptors([{
            'desc': descsum_create('combo(' + node.get_deterministic_priv_key().key + ')'),
            'timestamp': 1296688602,
        }])
        expected = [{'active': False,
                     'desc': 'combo(0227d85ba011276cf25b51df6a188b75e604b38770a462b2d0e9fb2fc839ef5d3f)#np574htj',
                     'timestamp': 1296688602}]
        assert_equal(expected, wallet.listdescriptors())


if __name__ == '__main__':
    ListDescriptorsTest().main()
