#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test encoding scripts via encodescript RPC command."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, bytes_to_hex_str


class EncodeScriptTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def encodescript_script_sig(self):
        signature = '304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c509001'
        push_signature = '48' + signature
        public_key = '03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2'
        push_public_key = '21' + public_key

        # 1) P2PK scriptSig
        # <signature>
        script_asm = signature
        script_hex = push_signature
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 2) P2PKH scriptSig
        # <sig> <pubkey>
        script_asm = signature + ' ' + public_key
        script_hex = push_signature + push_public_key
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 3) multisig scriptSig
        # OP_0 <A signature> <B signature>
        script_asm = '0 ' + signature + ' ' + signature
        script_hex = '00' + push_signature + push_signature
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 4) P2SH scriptSig
        rpc_result = self.nodes[0].encodescript('1 0')
        assert_equal('5100', rpc_result['hex'])

        # 5) empty scriptSig
        rpc_result = self.nodes[0].encodescript('')
        assert_equal('', rpc_result['hex'])

    def encodescript_script_pub_key(self):
        public_key = '03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2'
        push_public_key = '21' + public_key
        public_key_hash = '5dd1d3a048119c27b28293056724d9522f26d945'
        push_public_key_hash = '14' + public_key_hash
        uncompressed_public_key = '04b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb25e01fc8fde47c96c98a4f3a8123e33a38a50cf9025cc8c4494a518f991792bb7'
        push_uncompressed_public_key = '41' + uncompressed_public_key

        # 1) P2PK scriptPubKey
        # <pubkey> OP_CHECKSIG
        script_asm = public_key + ' OP_CHECKSIG'
        script_hex = push_public_key + 'ac'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 2) P2PKH scriptPubKey
        # OP_DUP OP_HASH160 <PubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
        script_asm = 'OP_DUP OP_HASH160 ' + \
            public_key_hash + ' OP_EQUALVERIFY OP_CHECKSIG'
        script_hex = '76a9' + push_public_key_hash + '88ac'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 3) multisig scriptPubKey
        # <m> <A pubkey> <B pubkey> <C pubkey> <n> OP_CHECKMULTISIG
        script_asm = '2 ' + public_key + ' ' + public_key + ' ' + \
            public_key + ' 3 OP_CHECKMULTISIG'
        script_hex = '52' + push_public_key + \
            push_public_key + push_public_key + '53ae'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 4) P2SH scriptPubKey
        # OP_HASH160 <Hash160(redeemScript)> OP_EQUAL.
        script_asm = 'OP_HASH160 ' + public_key_hash + ' OP_EQUAL'
        script_hex = 'a9' + push_public_key_hash + '87'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 5) null data scriptPubKey
        # use a signature look-alike here to make sure that we do not decode random data as a signature.
        # this matters if/when signature sighash decoding comes along.
        # would want to make sure that no such decoding takes place in this case.
        signature_imposter = \
            '48304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c509001'
        # OP_RETURN <data>
        script_asm = 'OP_RETURN ' + signature_imposter[2:]
        script_hex = '6a' + signature_imposter
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 6) a CLTV redeem script
        # OP_IF
        #   <receiver-pubkey> OP_CHECKSIGVERIFY
        # OP_ELSE
        #   <lock-until> OP_CHECKLOCKTIMEVERIFY OP_DROP
        # OP_ENDIF
        # <sender-pubkey> OP_CHECKSIG
        script_asm = 'OP_IF ' + public_key + \
            ' OP_CHECKSIGVERIFY OP_ELSE 500000 OP_CHECKLOCKTIMEVERIFY OP_DROP OP_ENDIF ' + \
            public_key + ' OP_CHECKSIG'
        script_hex = '63' + push_public_key + \
            'ad670320a107b17568' + push_public_key + 'ac'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 7) P2PK scriptPubKey
        # <pubkey> OP_CHECKSIG
        script_asm = uncompressed_public_key + ' OP_CHECKSIG'
        script_hex = push_uncompressed_public_key + 'ac'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 8) multisig scriptPubKey with an uncompressed pubkey
        # <m> <A pubkey> <B pubkey> <n> OP_CHECKMULTISIG
        script_asm = '2 ' + public_key + ' ' + \
            uncompressed_public_key + ' 2 OP_CHECKMULTISIG'
        script_hex = '52' + push_public_key + push_uncompressed_public_key + '52ae'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 9) P2WPKH scriptpubkey
        # 0 <PubKeyHash>
        script_asm = '0 ' + public_key_hash
        script_hex = '00' + push_public_key_hash
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

    def encodescript_data(self):
        """Test different conventions in script parsing"""
        rawhex = '2c2e6a2c2e'
        text = 'Random ASCII string'

        # 1) Splice raw hex script
        script_asm = '0 ' + '0x' + rawhex
        script_hex = '002c2e6a2c2e'
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

        # 2) Insert quoted string
        script_asm = 'OP_RETURN ' + '\'' + text + '\''
        script_hex = '6a' + '13' + bytes_to_hex_str(bytes(text, 'ascii'))
        rpc_result = self.nodes[0].encodescript(script_asm)
        assert_equal(script_hex, rpc_result['hex'])

    def run_test(self):
        self.encodescript_script_sig()
        self.encodescript_script_pub_key()
        self.encodescript_data()


if __name__ == '__main__':
    EncodeScriptTest().main()
