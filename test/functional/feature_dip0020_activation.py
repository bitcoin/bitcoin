#!/usr/bin/env python3
# Copyright (c) 2015-2023 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.blocktools import create_block, create_coinbase
from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.p2p import P2PDataStore
from test_framework.script import CScript, OP_CAT, OP_DROP, OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error, softfork_active, satoshi_round

'''
feature_dip0020_activation.py

This test checks activation of DIP0020 opcodes
'''

DISABLED_OPCODE_ERROR = "non-mandatory-script-verify-flag (Attempted to use a disabled opcode)"


DIP0020_HEIGHT = 300
class DIP0020ActivationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            f'-testactivationheight=dip0020@{DIP0020_HEIGHT}',
            "-acceptnonstdtxn=1",
        ]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_test_block(self, txs, tip_hash, tip_height, tip_time):
        block = create_block(int(tip_hash, 16), create_coinbase(tip_height + 1), tip_time + 1)
        block.nVersion = 4
        block.vtx.extend(txs)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        return block

    def run_test(self):
        node = self.nodes[0]
        relayfee = satoshi_round(node.getnetworkinfo()["relayfee"])

        # We should have some coins already
        utxos = node.listunspent()
        assert len(utxos) > 0

        # Lock some coins using disabled opcodes
        utxo = utxos[len(utxos) - 1]
        value = int(satoshi_round(utxo["amount"] - relayfee) * COIN)
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(int(utxo["txid"], 16), utxo["vout"])))
        tx.vout.append(CTxOut(value, CScript([b'1', b'2', OP_CAT])))
        tx_signed_hex = node.signrawtransactionwithwallet(tx.serialize().hex())["hex"]
        txid = node.sendrawtransaction(tx_signed_hex)

        # This tx should be completely valid, should be included in mempool and mined in the next block
        assert txid in set(node.getrawmempool())
        self.generate(node, 1)
        assert txid not in set(node.getrawmempool())

        # Create spending tx
        value = int(value - relayfee * COIN)
        tx0 = CTransaction()
        tx0.vin.append(CTxIn(COutPoint(int(txid, 16), 0)))
        tx0.vout.append(CTxOut(value, CScript([OP_TRUE, OP_DROP] * 15 + [OP_TRUE])))
        tx0.rehash()
        tx0_hex = tx0.serialize().hex()
        tx0id = node.decoderawtransaction(tx0_hex)["txid"]

        # flush state to disk before potential crashes below
        self.nodes[0].gettxoutsetinfo()

        self.log.info("Transactions spending coins with new opcodes aren't accepted before DIP0020 activation")
        assert not softfork_active(node, 'dip0020')
        assert_raises_rpc_error(-26, DISABLED_OPCODE_ERROR, node.sendrawtransaction, tx0_hex)
        helper_peer = node.add_p2p_connection(P2PDataStore())
        helper_peer.send_txs_and_test([tx0], node, success=False, reject_reason=DISABLED_OPCODE_ERROR)
        tip = node.getblock(node.getbestblockhash(), 1)
        test_block = self.create_test_block([tx0], tip["hash"], tip["height"], tip["time"])
        helper_peer.send_blocks_and_test([test_block], node, success=False, reject_reason='block-validation-failed', expect_disconnect=True)

        self.log.info("Generate enough blocks to activate DIP0020 opcodes")
        self.generate(node, 97)
        assert not softfork_active(node, 'dip0020')
        self.generate(node, 1)
        assert softfork_active(node, 'dip0020')

        # flush state to disk before potential crashes below
        self.nodes[0].gettxoutsetinfo()

        # Still need 1 more block for mempool to accept txes spending new opcodes
        self.log.info("Transactions spending coins with new opcodes aren't accepted at DIP0020 activation block")
        assert_raises_rpc_error(-26, DISABLED_OPCODE_ERROR, node.sendrawtransaction, tx0_hex)
        helper_peer = node.add_p2p_connection(P2PDataStore())
        helper_peer.send_txs_and_test([tx0], node, success=False, reject_reason=DISABLED_OPCODE_ERROR)
        # A block containing new opcodes is accepted however
        tip = node.getblock(node.getbestblockhash(), 1)
        test_block = self.create_test_block([tx0], tip["hash"], tip["height"], tip["time"])
        helper_peer.send_blocks_and_test([test_block], node, success=True)
        # txes spending new opcodes still won't be accepted into mempool if we roll back to the previous tip
        node.invalidateblock(node.getbestblockhash())
        assert tx0id not in set(node.getrawmempool())
        self.generate(node, 1)

        self.log.info("Transactions spending coins with new opcodes are accepted one block after DIP0020 activation block")
        node.sendrawtransaction(tx0_hex)
        assert tx0id in set(node.getrawmempool())


if __name__ == '__main__':
    DIP0020ActivationTest().main()
