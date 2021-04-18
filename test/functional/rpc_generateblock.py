#!/usr/bin/env python3
# Copyright (c) 2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''Test generateblock rpc.
'''

from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class GenerateBlockTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        self.log.info('Generate an empty block to address')
        address = node.getnewaddress()
        hash = node.generateblock(output=address, transactions=[])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['addresses'][0], address)

        self.log.info('Generate an empty block to a descriptor')
        hash = node.generateblock('addr(' + address + ')', [])['hash']
        block = node.getblock(blockhash=hash, verbosity=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['addresses'][0], address)

        self.log.info('Generate an empty block to a combo descriptor with compressed pubkey')
        combo_key = '0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798'
        combo_address = 'bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080'
        hash = node.generateblock('combo(' + combo_key + ')', [])['hash']
        block = node.getblock(hash, 2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['addresses'][0], combo_address)

        self.log.info('Generate an empty block to a combo descriptor with uncompressed pubkey')
        combo_key = '0408ef68c46d20596cc3f6ddf7c8794f71913add807f1dc55949fa805d764d191c0b7ce6894c126fce0babc6663042f3dde9b0cf76467ea315514e5a6731149c67'
        combo_address = 'mkc9STceoCcjoXEXe6cm66iJbmjM6zR9B2'
        hash = node.generateblock('combo(' + combo_key + ')', [])['hash']
        block = node.getblock(hash, 2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['addresses'][0], combo_address)

        # Generate 110 blocks to spend
        node.generatetoaddress(110, address)

        # Generate some extra mempool transactions to verify they don't get mined
        for _ in range(10):
            node.sendtoaddress(address, 0.001)

        self.log.info('Generate block with txid')
        txid = node.sendtoaddress(address, 1)
        hash = node.generateblock(address, [txid])['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 2)
        assert_equal(block['tx'][1], txid)

        self.log.info('Generate block with raw tx')
        utxos = node.listunspent(addresses=[address])
        raw = node.createrawtransaction([{'txid':utxos[0]['txid'], 'vout':utxos[0]['vout']}],[{address:1}])
        signed_raw = node.signrawtransactionwithwallet(raw)['hex']
        hash = node.generateblock(address, [signed_raw])['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 2)
        txid = block['tx'][1]
        assert_equal(node.gettransaction(txid)['hex'], signed_raw)

        self.log.info('Fail to generate block with out of order txs')
        raw1 = node.createrawtransaction([{'txid':txid, 'vout':0}],[{address:0.9999}])
        signed_raw1 = node.signrawtransactionwithwallet(raw1)['hex']
        txid1 = node.sendrawtransaction(signed_raw1)
        raw2 = node.createrawtransaction([{'txid':txid1, 'vout':0}],[{address:0.999}])
        signed_raw2 = node.signrawtransactionwithwallet(raw2)['hex']
        assert_raises_rpc_error(-25, 'TestBlockValidity failed: bad-txns-inputs-missingorspent', node.generateblock, address, [signed_raw2, txid1])

        self.log.info('Fail to generate block with txid not in mempool')
        missing_txid = '0000000000000000000000000000000000000000000000000000000000000000'
        assert_raises_rpc_error(-5, 'Transaction ' + missing_txid + ' not in mempool.', node.generateblock, address, [missing_txid])

        self.log.info('Fail to generate block with invalid raw tx')
        invalid_raw_tx = '0000'
        assert_raises_rpc_error(-22, 'Transaction decode failed for ' + invalid_raw_tx, node.generateblock, address, [invalid_raw_tx])

        self.log.info('Fail to generate block with invalid address/descriptor')
        assert_raises_rpc_error(-5, 'Invalid address or descriptor', node.generateblock, '1234', [])

        self.log.info('Fail to generate block with a ranged descriptor')
        ranged_descriptor = 'pkh(tpubD6NzVbkrYhZ4XgiXtGrdW5XDAPFCL9h7we1vwNCpn8tGbBcgfVYjXyhWo4E1xkh56hjod1RhGjxbaTLV3X4FyWuejifB9jusQ46QzG87VKp/0/*)'
        assert_raises_rpc_error(-8, 'Ranged descriptor not accepted. Maybe pass through deriveaddresses first?', node.generateblock, ranged_descriptor, [])

        self.log.info('Fail to generate block with a descriptor missing a private key')
        child_descriptor = 'pkh(tpubD6NzVbkrYhZ4XgiXtGrdW5XDAPFCL9h7we1vwNCpn8tGbBcgfVYjXyhWo4E1xkh56hjod1RhGjxbaTLV3X4FyWuejifB9jusQ46QzG87VKp/0\'/0)'
        assert_raises_rpc_error(-5, 'Cannot derive script without private keys', node.generateblock, child_descriptor, [])

if __name__ == '__main__':
    GenerateBlockTest().main()
