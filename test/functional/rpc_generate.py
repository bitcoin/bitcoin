#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test generate* RPCs."""

from concurrent.futures import ThreadPoolExecutor
from decimal import Decimal

from test_framework.blocktools import MAX_BLOCK_SIGOPS, get_witness_script
from test_framework.messages import MINIMUM_BLOCK_RESERVED_WEIGHT
from test_framework.script import CScript, OP_CHECKMULTISIG, OP_CHECKSIG
from test_framework.script_util import keys_to_multisig_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class RPCGenerateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.test_generatetoaddress()
        self.test_generate()
        self.test_generateblock()
        self.test_generateblock_sigops()
        self.test_generateblock_weight()

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

        self.log.info('Generate blocks with a payout matching the witness commitment prefix')
        witness_commitment = get_witness_script(witness_root=0, witness_nonce=0).hex()
        payout_script = get_witness_script(witness_root=0, witness_nonce=1).hex()
        address_script = miniwallet.get_output_script().hex()
        for payout_scripts in ([address_script, payout_script], [payout_script, address_script]):
            generated_block = self.generateblock(node, output=[f'raw({script})' for script in payout_scripts], transactions=[], submit=False)
            assert_equal(node.submitblock(generated_block['hex']), None)
            assert_equal(node.getbestblockhash(), generated_block['hash'])
            coinbase_outputs = node.getblock(generated_block['hash'], 2)['tx'][0]['vout']
            assert_equal([out['scriptPubKey']['hex'] for out in coinbase_outputs], [*payout_scripts, witness_commitment])

        self.log.info('Generate an empty block without address and fallback to OP_RETURN')
        hash = self.generateblock(node, transactions=[])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['asm'], 'OP_RETURN')

        self.log.info('Generate an empty block to address')
        hash = self.generateblock(node, output=address, transactions=[])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], address)

        self.log.info('Generate an empty block to a list of addresses')
        address2 = miniwallet.get_address()
        hash = self.generateblock(node, output=[address, address2], transactions=[])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], address)
        assert_equal(block['tx'][0]['vout'][1]['scriptPubKey']['address'], address2)

        self.log.info('Generate an empty block to a descriptor')
        hash = self.generateblock(node, 'addr(' + address + ')', [])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], address)

        self.log.info('Generate an empty block to a list of descriptors')
        hash = self.generateblock(node, ['addr(' + address + ')', 'addr('+ address2 + ')'], [])['hash']
        block = node.getblock(blockhash=hash, verbose=2)
        assert_equal(len(block['tx']), 1)
        assert_equal(block['tx'][0]['vout'][0]['scriptPubKey']['address'], address)
        assert_equal(block['tx'][0]['vout'][1]['scriptPubKey']['address'], address2)

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

        # Generate some extra mempool transactions to verify that they are mined if no txs are provided
        for _ in range(10):
            miniwallet.send_self_transfer(from_node=node)
        self.log.info("Generate a block with mempool txs")
        hash = self.generateblock(node, address)['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 11)

        # Generate some extra mempool transactions to verify they don't get mined
        for _ in range(10):
            miniwallet.send_self_transfer(from_node=node)

        self.log.info('Generate block with txid')
        txid = miniwallet.send_self_transfer(from_node=node)['txid']
        hash = self.generateblock(node, address, [txid])['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 2)
        assert_equal(block['tx'][1], txid)

        self.log.info('Generate block with multiple txid')
        # Generate some transactions to get mined
        txs = []
        for _ in range(3):
            txs.append(miniwallet.send_self_transfer(from_node=node)['txid'])
        # Generate some extra mempool transactions to verify they don't get mined
        for _ in range(10):
            miniwallet.send_self_transfer(from_node=node)
        hash = self.generateblock(node, address, txs)['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 4)
        for i, tx in enumerate(txs):
            assert_equal(block['tx'][i+1], tx)
        # Clean the mempool for next test by mining a block
        self.generateblock(node, address)

        self.log.info('Generate block with raw tx')
        rawtx = miniwallet.create_self_transfer()['hex']
        hash = self.generateblock(node, address, [rawtx])['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 2)
        txid = block['tx'][1]
        assert_equal(node.getrawtransaction(txid=txid, verbose=False, blockhash=hash), rawtx)

        self.log.info('Generate empty block')
        # put some txs into the mempool
        for _ in range(10):
            miniwallet.send_self_transfer(from_node=node)
        hash = self.generateblock(node, address, [])['hash']
        block = node.getblock(hash, 1)
        assert_equal(len(block['tx']), 1) # it contains the coinbase tx
        # Clean the mempool for next test by mining a block
        self.generateblock(node, address)

        # Ensure that generateblock can be called concurrently by many threads.
        self.log.info('Generate blocks in parallel')
        def generate_50_blocks(n):
            return [n.generateblock(output=address, transactions=[]) for _ in range(50)]

        rpcs = [node.cli for _ in range(6)]
        with ThreadPoolExecutor(max_workers=len(rpcs)) as threads:
            list(threads.map(generate_50_blocks, rpcs))

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

    def test_generateblock_sigops(self):
        self.log.info('Account for coinbase sigops when selecting mempool transactions')
        node = self.nodes[0]
        self.generateblock(node)
        miniwallet = MiniWallet(node)
        _, pubkey = generate_keypair()
        sigops_txid = miniwallet.send_to(
            from_node=node, scriptPubKey=keys_to_multisig_script([pubkey]), amount=1_000_000, fee=10_000,
        )['txid']
        zero_sigops_txid = miniwallet.send_self_transfer(from_node=node, confirmed_only=True)['txid']

        # The payout leaves room for 19 legacy sigops. The bare multisig output adds 20.
        multisig_ops = [OP_CHECKMULTISIG] * (MAX_BLOCK_SIGOPS // 20 - 1)
        payout_script = CScript([*multisig_ops, OP_CHECKSIG])
        block_hash = self.generateblock(node, output=f'raw({payout_script.hex()})')['hash']
        assert_equal(node.getblock(block_hash)['tx'][1:], [zero_sigops_txid])
        assert_equal(node.getrawmempool(), [sigops_txid])

        self.log.info('Reject coinbase payouts that exceed the block sigops limit on their own')
        payout_script = CScript([*multisig_ops, OP_CHECKSIG, OP_CHECKMULTISIG])
        assert_raises_rpc_error(-8, 'exceeds the maximum block sigops cost', self.generateblock, node, output=f'raw({payout_script.hex()})')

    def test_generateblock_weight(self):
        self.log.info('Account for coinbase payouts when selecting mempool transactions')
        node = self.nodes[0]
        self.generateblock(node)
        max_weight = 4000
        self.restart_node(0, extra_args=[f'-blockmaxweight={max_weight}', f'-blockreservedweight={MINIMUM_BLOCK_RESERVED_WEIGHT}'])

        miniwallet = MiniWallet(node)
        low_fee = miniwallet.send_self_transfer(from_node=node, confirmed_only=True, fee_rate=Decimal('0.001'))['txid']
        high_fee = miniwallet.send_self_transfer(from_node=node, confirmed_only=True, fee_rate=Decimal('0.002'))['txid']

        # These payouts leave room for one of the two mempool transactions.
        payouts = ['raw(51)'] * 64
        block_hash = self.generateblock(node, output=payouts)['hash']
        block = node.getblock(block_hash)
        assert_greater_than_or_equal(max_weight, block['weight'])
        assert_equal(block['tx'][1:], [high_fee])
        assert_equal(node.getrawmempool(), [low_fee])

        self.log.info('Reject coinbase payouts that exceed the block weight limit on their own')
        assert_raises_rpc_error(-8, 'exceeds the maximum block weight', self.generateblock, node, output=['raw(51)'] * 100)

    def test_generate(self):
        message = (
            "generate\n\n"
            "has been replaced by the -generate "
            "cli option. Refer to -help for more information.\n"
        )

        if not self.options.usecli:
            self.log.info("Test rpc generate raises with message to use cli option")
            assert_raises_rpc_error(-32601, message, self.nodes[0]._rpc.generate)

            self.log.info("Test rpc generate help prints message to use cli option")
            assert_equal(message, self.nodes[0].help("generate"))

        self.log.info("Test rpc generate is a hidden command not discoverable in general help")
        assert message not in self.nodes[0].help()


if __name__ == "__main__":
    RPCGenerateTest(__file__).main()
