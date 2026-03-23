#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test OP_NET tx rejection."""

from test_framework.address import output_key_to_p2tr
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_1,
    OP_1NEGATE,
    OP_CHECKSIGVERIFY,
    OP_DEPTH,
    OP_DUP,
    OP_ELSE,
    OP_ENDIF,
    OP_EQUALVERIFY,
    OP_HASH160,
    OP_HASH256,
    OP_IF,
    OP_NUMEQUAL,
    OP_TOALTSTACK,
    OP_TRUE,
    hash160,
    hash256,
    taproot_construct,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


# Real OP_NET interaction witness from testnet4 tx
TESTNET4_WITNESS = [
    bytes.fromhex("789e4a1d65ecd0ad2e21490a17eb8f81a91076db963cd180214201d6a041ce3d"),
    bytes.fromhex("89e5a3a4c88d419a00f88aac2092fcb907aab469702b5c34c37833b6042a1502"
                   "d7564f0f4fb629fb9a64661372774d610fd36a842b202866a6abd6c028331d4a"),
    bytes.fromhex("213e1f1fa83547079b478a72861957e78dbd3e5a64e46530eae5d9fa940518a3"
                   "19f921f1fb6a46bfb842743ff1b6cf44239667ad1610372bb273ce0ff8c25536"),
    bytes.fromhex("0c0200000600000000000000006b20d7de3cde7a99e905feaee890bbca860d43"
                   "54c0914c066899bdfcbb19eb2072e36b14bad1dd76cdc55be56557f33fe932f4"
                   "b6831cd1586b200d38a7bdf2225b4e0c014c818f84564ccd2bc3f8583a0e63de"
                   "6e29595c561cd276aa207ca38b4b57abfa5fe6ac13561b62fb681b2911367135"
                   "c9a57ee284ce403d365488ad205b5cd6f71fe325baa9b643d98b8fd601e92ea4"
                   "d34eecb7f46ded590ce5de13edada914ab0c0ed0a9539000670a3b7f9888e4e4"
                   "aad37d268874519c63026f704cb70000004dd7de3cde7a99e905feaee890bbca"
                   "860d4354c0914c066899bdfcbb19eb2072e3480cb29059c00008d4a0d4b8cb62"
                   "969d1b7bb9f1d98f8bf5a2e707b829263c19000000094f504e45542e4f524700"
                   "0000622cb671ad234f0e1e16227d5ff92b5006c9c5c49e8a45f78b8dbd1de64f"
                   "7103aef200911f4d1593908762444c0c03271b6890b85ec7d53c35119e5dfe37"
                   "a457fbd24b9f25d5a831e98009767369d8f3657ed4f8d4c71c7d8fd3fcd9d993"
                   "d15786849f4f3d1f8b0800000000000203db5e29d2cfc0c0c0b4ad70adb23f9f"
                   "9c98526dfc4fed00b693478fcceb72fddeddbb57f6997f21f3ba4f00e1e92946"
                   "28000000675168"),
    bytes.fromhex("c10d38a7bdf2225b4e0c014c818f84564ccd2bc3f8583a0e63de6e29595c561c"
                   "d2e18f575273dee601fc8d7286e59728f5e613d40df74e90f152801f9042e810"
                   "3d"),
]


class MempoolRejectOPNetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-datacarriersize=0"],       # node 0: reject datacarrier transaction
            ["-datacarriersize=100000"],  # node 1: allow datacarrier
        ]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.test_real_testnet4_witness()
        self.test_interaction_tapscript()
        self.test_deployment_tapscript()
        self.test_no_false_positives()

    def test_real_testnet4_witness(self):
        """Verify the filter catches a real OP_NET testnet4 witness stack."""
        self.log.info("Testing with real testnet4 OP_NET witness data")

        # Sanity-check the testnet4 witness structure:
        #   [0] salt 32B, [1] sig 64B, [2] sig 64B, [3] tapscript, [4] control block 65B
        assert_equal(len(TESTNET4_WITNESS[0]), 32)
        assert_equal(len(TESTNET4_WITNESS[1]), 64)
        assert_equal(len(TESTNET4_WITNESS[2]), 64)
        assert_equal(len(TESTNET4_WITNESS[4]), 65)

        # Confirm the "op" magic (0x02 0x6f 0x70) is in the tapscript at offset 201
        tapscript = TESTNET4_WITNESS[3]
        magic_offset = tapscript.find(b'\x02\x6f\x70')
        assert_equal(magic_offset, 201)
        self.log.info(f"  testnet4 tapscript: {len(tapscript)}B, magic at offset {magic_offset}")

        self._test_opnet_witness(TESTNET4_WITNESS[3])

    def test_interaction_tapscript(self):
        """Build an OP_NET interaction tapscript from scratch and verify filtering."""
        self.log.info("Testing with constructed OP_NET interaction tapscript")

        sender_pubkey = b'\x11' * 32
        salt_pubkey = b'\x22' * 32
        contract_secret = b'\x33' * 32

        opnet_script = CScript([
            b'\x00' * 12, OP_TOALTSTACK,              # header (12 bytes)
            b'\x00' * 32, OP_TOALTSTACK,              # challenge pubkey
            b'\x00' * 20, OP_TOALTSTACK,              # preimage
            sender_pubkey,                            # sender pubkey
            OP_DUP, OP_HASH256,
            hash256(sender_pubkey),                   # hashed sender pubkey
            OP_EQUALVERIFY, OP_CHECKSIGVERIFY,
            salt_pubkey,                              # contract salt pubkey
            OP_CHECKSIGVERIFY, OP_HASH160,
            hash160(contract_secret),                 # contract secret hash (20B)
            OP_EQUALVERIFY,
            OP_DEPTH, OP_1, OP_NUMEQUAL, OP_IF,
            b'op',                                    # OPNet magic
            OP_1NEGATE,
            b'\xaa' * 64,                             # calldata
            OP_ELSE, OP_1, OP_ENDIF,
        ])
        raw_script = bytes(opnet_script)
        magic_offset = raw_script.find(b'\x02\x6f\x70')
        assert magic_offset > 0, f"magic should be at non-zero offset, got {magic_offset}"
        self.log.info(f"  interaction tapscript: {len(raw_script)}B, magic at offset {magic_offset}")

        self._test_opnet_witness(raw_script)

    def test_deployment_tapscript(self):
        """Build an OP_NET deployment tapscript from scratch and verify filtering."""
        self.log.info("Testing with constructed OP_NET deployment tapscript")

        sender_pubkey = b'\x44' * 32
        salt_pubkey = b'\x55' * 32
        contract_salt = b'\x66' * 32

        opnet_script = CScript([
            b'\x00' * 12, OP_TOALTSTACK,              # header (12 bytes)
            b'\x00' * 32, OP_TOALTSTACK,              # challenge pubkey
            b'\x00' * 20, OP_TOALTSTACK,              # preimage
            sender_pubkey,                            # sender pubkey
            OP_DUP, OP_HASH256,
            hash256(sender_pubkey),                   # hashed sender pubkey
            OP_EQUALVERIFY, OP_CHECKSIGVERIFY,
            salt_pubkey,                              # contract salt pubkey
            OP_CHECKSIGVERIFY, OP_HASH256,
            hash256(contract_salt),                   # contract salt hash (32B)
            OP_EQUALVERIFY,
            OP_DEPTH, OP_1, OP_NUMEQUAL, OP_IF,
            b'op',                                    # OPNet magic
            OP_0, OP_1NEGATE,
            b'\xbb' * 64,                             # bytecode
            OP_ELSE, OP_1, OP_ENDIF,
        ])
        raw_script = bytes(opnet_script)
        magic_offset = raw_script.find(b'\x02\x6f\x70')
        assert magic_offset > 0, f"magic should be at non-zero offset, got {magic_offset}"
        self.log.info(f"  deployment tapscript: {len(raw_script)}B, magic at offset {magic_offset}")

        self._test_opnet_witness(raw_script)

    def _test_opnet_witness(self, raw_tapscript):
        """Given a raw tapscript, build a taproot output, fund it, spend with
        OP_NET 5-element witness, and verify the filter catches it."""

        internal_key = (1).to_bytes(32, 'big')
        opnet_script = CScript(raw_tapscript)
        dummy_script = CScript([OP_TRUE])
        tap = taproot_construct(internal_key, [
            ("opnet", opnet_script),
            ("dummy", dummy_script),
        ])

        opnet_leaf = tap.leaves["opnet"]
        control_block = bytes([opnet_leaf.version | tap.negflag]) + tap.internal_pubkey + opnet_leaf.merklebranch
        assert_equal(len(control_block), 65)

        # Fund the taproot output
        taproot_spk = tap.scriptPubKey
        utxo = self.wallet.get_utxo()
        funding_tx = CTransaction()
        funding_tx.vin = [CTxIn(COutPoint(int(utxo['txid'], 16), utxo['vout']))]
        funding_value = int(utxo['value'] * 100_000_000) - 1000
        funding_tx.vout = [CTxOut(funding_value, taproot_spk)]
        funding_tx.version = 2
        self.wallet.sign_tx(funding_tx)

        self.nodes[0].sendrawtransaction(funding_tx.serialize().hex())
        self.generate(self.nodes[0], 1)
        self.sync_all()

        # Build spending tx with 5-element OP_NET witness
        spend_tx = CTransaction()
        spend_tx.version = 2
        spend_tx.vin = [CTxIn(COutPoint(funding_tx.txid_int, 0))]
        spend_value = funding_value - 1000
        spend_tx.vout = [CTxOut(spend_value, taproot_spk)]

        spend_tx.wit.vtxinwit = [CTxInWitness()]
        spend_tx.wit.vtxinwit[0].scriptWitness.stack = [
            b'\x00' * 32,           # salt (32 bytes)
            b'\x00' * 64,           # sig_a (64 bytes)
            b'\x00' * 64,           # sig_b (64 bytes)
            raw_tapscript,          # tapscript
            control_block,          # control block (65 bytes)
        ]
        spend_hex = spend_tx.serialize().hex()

        # Node 0 (datacarriersize=0): should reject
        result = self.nodes[0].testmempoolaccept([spend_hex])[0]
        assert_equal(result['allowed'], False)
        assert_equal(result['reject-reason'], 'tokens-op-net')
        self.log.info("  -> correctly rejected by node 0")

        # Node 1 (datacarriersize=100000): should NOT reject for token reasons
        result = self.nodes[1].testmempoolaccept([spend_hex])[0]
        if not result['allowed']:
            assert result['reject-reason'] != 'tokens-op-net', \
                f"Node 1 should not reject as token, got: {result['reject-reason']}"
        self.log.info("  -> correctly allowed by node 1")

    def test_no_false_positives(self):
        """Normal taproot transactions must not be rejected."""
        self.log.info("Verifying normal taproot transactions are not rejected (no false positives)")
        normal_tx = self.wallet.create_self_transfer()
        normal_result = self.nodes[0].testmempoolaccept([normal_tx['hex']])[0]
        assert_equal(normal_result['allowed'], True)


if __name__ == '__main__':
    MempoolRejectOPNetTest(__file__).main()