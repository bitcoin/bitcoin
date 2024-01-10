#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test generate* RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class RPCGenerateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.test_generatetoaddress()
        self.test_generate()
        self.test_generateblock()

    def test_generatetoaddress(self):
        self.generatetoaddress(self.nodes[0], 1, 'mneYUmWYsuk7kySiURxCi3AGxrAqZxLgPZ')
        assert_raises_rpc_error(-5, "Invalid address", self.generatetoaddress, self.nodes[0], 1, '3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy')

    def test_generateblock(self):
        node = self.nodes[0]
        miniwallet = MiniWallet(node)

        self.log.info('Mine an empty block to address and return the hex')
        address = miniwallet.get_address()
        generated_block = self.generateblock(node, output=address, transactions=[], submit=False)
        node.submitblock(hexdata=generated_block['hex'])
        assert_equal(generated_block['hash'], node.getbestblockhash())

        self.log.info('Generate an empty block to address')
        hash = self.generateblock(node, output=address, transactions=[])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], address)

        self.log.info('Generate an empty block to a descriptor')
        hash = self.generateblock(node, 'addr(' + address + ')', [])['hash']
        block = node.getblock(blockhash=hash, verbosity=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], address)

        self.log.info('Generate an empty block to a combo descriptor with compressed pubkey')
        combo_key = '0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798'
        combo_address = 'bcrt1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080'
        hash = self.generateblock(node, 'combo(' + combo_key + ')', [])['hash']
        block = node.getblock(hash, 2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], combo_address)

        self.log.info('Generate an empty block to a combo descriptor with uncompressed pubkey')
        combo_key = '0408ef68c46d20596cc3f6ddf7c8794f71913add807f1dc55949fa805d764d191c0b7ce6894c126fce0babc6663042f3dde9b0cf76467ea315514e5a6731149c67'
        combo_address = 'mkc9STceoCcjoXEXe6cm66iJbmjM6zR9B2'
        hash = self.generateblock(node, 'combo(' + combo_key + ')', [])['hash']
        block = node.getblock(hash, 2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], combo_address)

        # Generate some extra mempool transactions to verify they don't get mined
        for _ in range(10):
            miniwallet.send_self_transfer(from_node=node)

        self.log.info('Generate block with txid')
        txid = miniwallet.send_self_transfer(from_node=node)['txid']
        hash = self.generateblock(node, address, [txid])['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 2)
        assert_equal(block['tx'][1], txid)

        self.log.info('Generate block with raw tx')
        rawtx = miniwallet.create_self_transfer()['hex']
        hash = self.generateblock(node, address, [rawtx])['hash']

        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 2)
        txid = block['tx'][1]
        assert_equal(node.getrawtransaction(txid=txid, verbose=False, blockhash=hash), rawtx)

        self.log.info('Fail to generate block with out of order txs')
        txid1 = miniwallet.send_self_transfer(from_node=node)['txid']
        utxo1 = miniwallet.get_utxo(txid=txid1)
        rawtx2 = miniwallet.create_self_transfer(utxo_to_spend=utxo1)['hex']
        assert_raises_rpc_error(-25, 'TestBlockValidity failed: bad-txns-inputs-missingorspent', self.generateblock, node, address, [rawtx2, txid1])

        self.log.info('Fail to generate block with txid not in mempool')
        missing_txid = '0000000000000000000000000000000000000000000000000000000000000000'
        assert_raises_rpc_error(-5, 'Transaction ' + missing_txid + ' not in mempool.', self.generateblock, node, address, [missing_txid])

        self.log.info('Fail to generate block with invalid raw tx')
        invalid_raw_tx = '0000'
        assert_raises_rpc_error(-22, 'Transaction decode failed for ' + invalid_raw_tx, self.generateblock, node, address, [invalid_raw_tx])

        self.log.info('Fail to generate block with invalid address/descriptor')
        assert_raises_rpc_error(-5, 'Invalid address or descriptor', self.generateblock, node, '1234', [])

        self.log.info('Fail to generate block with a ranged descriptor')
        ranged_descriptor = 'pkh(tpubD6NzVbkrYhZ4XgiXtGrdW5XDAPFCL9h7we1vwNCpn8tGbBcgfVYjXyhWo4E1xkh56hjod1RhGjxbaTLV3X4FyWuejifB9jusQ46QzG87VKp/0/*)'
        assert_raises_rpc_error(-8, 'Ranged descriptor not accepted. Maybe pass through deriveaddresses first?', self.generateblock, node, ranged_descriptor, [])

        self.log.info('Fail to generate block with a descriptor missing a private key')
        child_descriptor = 'pkh(tpubD6NzVbkrYhZ4XgiXtGrdW5XDAPFCL9h7we1vwNCpn8tGbBcgfVYjXyhWo4E1xkh56hjod1RhGjxbaTLV3X4FyWuejifB9jusQ46QzG87VKp/0\'/0)'
        assert_raises_rpc_error(-5, 'Cannot derive script without private keys', self.generateblock, node, child_descriptor, [])

    def test_generate(self):
        message = (
            "generate\n\n"
            "has been replaced by the -generate "
            "cli option. Refer to -help for more information.\n"
        )

        self.log.info("Test rpc generate raises with message to use cli option")
        assert_raises_rpc_error(-32601, message, self.nodes[0].rpc.generate)

        self.log.info("Test rpc generate help prints message to use cli option")
        assert_equal(message, self.nodes[0].help("generate"))

        self.log.info("Test rpc generate is a hidden command not discoverable in general help")
        assert message not in self.nodes[0].help()


if __name__ == "__main__":
    RPCGenerateTest().main()
