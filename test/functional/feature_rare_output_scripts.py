#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Check how rare output script types are treated regarding
   standardness and consensus-validity.
"""
import random

from test_framework.blocktools import (
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)
from test_framework.key import ECPubKey
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.script import sign_input_legacy
from test_framework.script_util import (
    key_to_p2pk_script,
    keys_to_multisig_script,
    output_key_to_p2tr_script,
    program_to_witness_script,
)
from test_framework.crypto.secp256k1 import GE
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import generate_keypair


class RareOutputScripts(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def generate_uncompressed_keypair(self, *, is_even):
        while True:
            privkey, pubkey_bytes = generate_keypair(compressed=False)
            pubkey = ECPubKey()
            pubkey.set(pubkey_bytes)
            if pubkey.p.y.is_even() == is_even:
                return privkey, pubkey

    def create_p2pk_spend(self, outpoint, scriptpubkey, privkey):
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(outpoint[0], 16), outpoint[1]))]
        tx.vout = [CTxOut(40000, self.wallet.get_scriptPubKey())]
        sign_input_legacy(tx, 0, scriptpubkey, privkey)
        return tx

    def create_empty_witness_tx(self, outpoint):
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(outpoint[0], 16), outpoint[1]))]
        tx.vout = [CTxOut(40000, self.wallet.get_scriptPubKey())]
        tx.wit.vtxinwit.append(CTxInWitness())
        return tx

    def test_hybrid_pubkeys(self):
        self.log.info("Check that creating hybrid pubkey P2PK outputs is standard")
        privkey_even, pubkey_even = self.generate_uncompressed_keypair(is_even=True)
        assert pubkey_even.p.y.is_even()
        pubkey_even_script = key_to_p2pk_script(b'\x06' + pubkey_even.get_bytes()[1:])

        privkey_odd, pubkey_odd = self.generate_uncompressed_keypair(is_even=False)
        assert not pubkey_odd.p.y.is_even()
        pubkey_odd_script = key_to_p2pk_script(b'\x07' + pubkey_odd.get_bytes()[1:])

        tx_even = self.wallet.send_to(from_node=self.node, scriptPubKey=pubkey_even_script, amount=50000)
        tx_odd = self.wallet.send_to(from_node=self.node, scriptPubKey=pubkey_odd_script, amount=50000)
        for tx in (tx_even, tx_odd):
            decode_res = self.node.decoderawtransaction(tx['hex'])
            assert_equal(decode_res['vout'][tx['sent_vout']]['scriptPubKey']['type'], "pubkey")

        self.log.info("Check that spending hybrid pubkey P2PK outputs is non-standard")
        spend_even_tx = self.create_p2pk_spend((tx_even['txid'], tx_even['sent_vout']), pubkey_even_script, privkey_even)
        spend_odd_tx = self.create_p2pk_spend((tx_odd['txid'], tx_odd['sent_vout']), pubkey_odd_script, privkey_odd)
        for tx in (spend_even_tx, spend_odd_tx):
            assert_raises_rpc_error(-26, "non-mandatory-script-verify-flag (Public key is neither compressed or uncompressed)",
                                    self.node.sendrawtransaction, tx.serialize().hex())

        self.log.info("Check that spending hybrid pubkey P2PK outputs is consensus-valid")
        self.generate(self.node, 1)
        tmpl = self.node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=tmpl, txlist=[spend_even_tx, spend_odd_tx])
        block.solve()
        self.node.submitblock(block.serialize().hex())

        last_block_txs = self.node.getblock(self.node.getbestblockhash())['tx']
        assert spend_even_tx.rehash() in last_block_txs
        assert spend_odd_tx.rehash() in last_block_txs

    def test_pubkeys_not_on_curve(self):
        while True:
            invalid_xonly_pub = random.randbytes(32)
            if not GE.is_valid_x(int.from_bytes(invalid_xonly_pub, 'big')):
                break

        self.log.info("Check that creating P2PK outputs with not-on-curve pubkey is standard")
        for compressed_prefix in [b'\x02', b'\x03']:
            invalid_compressed_pub = compressed_prefix + invalid_xonly_pub
            self.wallet.send_to(from_node=self.node, scriptPubKey=key_to_p2pk_script(invalid_compressed_pub), amount=21000)
        invalid_uncompressed_pub = b'\x04' + invalid_xonly_pub + random.randbytes(32)
        self.wallet.send_to(from_node=self.node, scriptPubKey=key_to_p2pk_script(invalid_uncompressed_pub), amount=21000)

        self.log.info("Check that creating P2MS outputs with not-on-curve pubkeys is standard")
        for n in (1, 2, 3):
            p2ms_script = keys_to_multisig_script([invalid_uncompressed_pub]*n)
            self.wallet.send_to(from_node=self.node, scriptPubKey=p2ms_script, amount=21000)

        self.log.info("Check that creating P2TR outputs with not-on-curve pubkey is standard")
        self.wallet.send_to(from_node=self.node, scriptPubKey=output_key_to_p2tr_script(invalid_xonly_pub), amount=21000)

    def test_future_segwit_versions(self):
        self.log.info("Check that creating future segwit outputs (2 <= version <= 16) is standard")
        funding_txs = []
        for future_ver in range(2, 16+1):
            witness_program = random.randbytes(random.randrange(2, 40+1))
            output_script = program_to_witness_script(future_ver, witness_program)
            tx = self.wallet.send_to(from_node=self.node, scriptPubKey=output_script, amount=50000)
            decode_res = self.node.decoderawtransaction(tx['hex'])
            assert_equal(decode_res['vout'][tx['sent_vout']]['scriptPubKey']['type'], "witness_unknown")
            funding_txs.append(tx)

        self.log.info("Check that spending future segwit outputs is non-standard")
        spending_txs = []
        for funding_tx in funding_txs:
            spending_tx = self.create_empty_witness_tx((funding_tx['txid'], funding_tx['sent_vout']))
            assert_raises_rpc_error(-26, "bad-txns-nonstandard-inputs",
                                    self.node.sendrawtransaction, spending_tx.serialize().hex())
            spending_txs.append(spending_tx)

        self.log.info("Check that spending future segwit outputs is consensus-valid")
        self.generate(self.node, 1)
        tmpl = self.node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        block = create_block(tmpl=tmpl, txlist=spending_txs)
        block.solve()
        self.node.submitblock(block.serialize().hex())

        last_block_txs = self.node.getblock(self.node.getbestblockhash())['tx']
        for spending_tx in spending_txs:
            assert spending_tx.rehash() in last_block_txs

    def run_test(self):
        self.node = self.nodes[0]
        self.wallet = MiniWallet(self.node)
        self.test_hybrid_pubkeys()
        self.test_pubkeys_not_on_curve()
        self.test_future_segwit_versions()


if __name__ == '__main__':
    RareOutputScripts().main()
