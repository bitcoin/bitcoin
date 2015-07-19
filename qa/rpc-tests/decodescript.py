#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class DecodeScriptTest(BitcoinTestFramework):
    """Tests decoding scripts via RPC command "decodescript"."""

    def setup_chain(self):
        print('Initializing test directory ' + self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 1)

    def setup_network(self, split=False):
        self.nodes = start_nodes(1, self.options.tmpdir)
        self.is_network_split = False

    def decodescript_script_sig(self):
        signature = '304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c509001'
        push_signature = '48' + signature
        public_key = '03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2'
        push_public_key = '21' + public_key

        # below are test cases for all of the standard transaction types

        # 1) P2PK scriptSig
        # the scriptSig of a public key scriptPubKey simply pushes a signature onto the stack
        rpc_result = self.nodes[0].decodescript(push_signature)
        assert_equal(signature, rpc_result['asm'])

        # 2) P2PKH scriptSig
        rpc_result = self.nodes[0].decodescript(push_signature + push_public_key)
        assert_equal(signature + ' ' + public_key, rpc_result['asm'])

        # 3) multisig scriptSig
        # this also tests the leading portion of a P2SH multisig scriptSig
        # OP_0 <A sig> <B sig>
        rpc_result = self.nodes[0].decodescript('00' + push_signature + push_signature)
        assert_equal('0 ' + signature + ' ' + signature, rpc_result['asm'])

        # 4) P2SH scriptSig
        # an empty P2SH redeemScript is valid and makes for a very simple test case.
        # thus, such a spending scriptSig would just need to pass the outer redeemScript
        # hash test and leave true on the top of the stack.
        rpc_result = self.nodes[0].decodescript('5100')
        assert_equal('1 0', rpc_result['asm'])

        # 5) null data scriptSig - no such thing because null data scripts can not be spent.
        # thus, no test case for that standard transaction type is here.

    def decodescript_script_pub_key(self):
        public_key = '03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2'
        push_public_key = '21' + public_key
        public_key_hash = '11695b6cd891484c2d49ec5aa738ec2b2f897777'
        push_public_key_hash = '14' + public_key_hash

        # below are test cases for all of the standard transaction types

        # 1) P2PK scriptPubKey
        # <pubkey> OP_CHECKSIG
        rpc_result = self.nodes[0].decodescript(push_public_key + 'ac')
        assert_equal(public_key + ' OP_CHECKSIG', rpc_result['asm'])

        # 2) P2PKH scriptPubKey
        # OP_DUP OP_HASH160 <PubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
        rpc_result = self.nodes[0].decodescript('76a9' + push_public_key_hash + '88ac')
        assert_equal('OP_DUP OP_HASH160 ' + public_key_hash + ' OP_EQUALVERIFY OP_CHECKSIG', rpc_result['asm'])

        # 3) multisig scriptPubKey
        # <m> <A pubkey> <B pubkey> <C pubkey> <n> OP_CHECKMULTISIG
        # just imagine that the pub keys used below are different.
        # for our purposes here it does not matter that they are the same even though it is unrealistic.
        rpc_result = self.nodes[0].decodescript('52' + push_public_key + push_public_key + push_public_key + '53ae')
        assert_equal('2 ' + public_key + ' ' + public_key + ' ' + public_key +  ' 3 OP_CHECKMULTISIG', rpc_result['asm'])

        # 4) P2SH scriptPubKey
        # OP_HASH160 <Hash160(redeemScript)> OP_EQUAL.
        # push_public_key_hash here should actually be the hash of a redeem script.
        # but this works the same for purposes of this test.
        rpc_result = self.nodes[0].decodescript('a9' + push_public_key_hash + '87')
        assert_equal('OP_HASH160 ' + public_key_hash + ' OP_EQUAL', rpc_result['asm'])

        # 5) null data scriptPubKey
        # use a signature look-alike here to make sure that we do not decode random data as a signature.
        # this matters if/when signature sighash decoding comes along.
        # would want to make sure that no such decoding takes place in this case.
        signature_imposter = '48304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c509001'
        # OP_RETURN <data>
        rpc_result = self.nodes[0].decodescript('6a' + signature_imposter)
        assert_equal('OP_RETURN ' + signature_imposter[2:], rpc_result['asm'])

        # 6) a CLTV redeem script. redeem scripts are in-effect scriptPubKey scripts, so adding a test here.
        # OP_NOP2 is also known as OP_CHECKLOCKTIMEVERIFY.
        # just imagine that the pub keys used below are different.
        # for our purposes here it does not matter that they are the same even though it is unrealistic.
        #
        # OP_IF
        #   <receiver-pubkey> OP_CHECKSIGVERIFY
        # OP_ELSE
        #   <lock-until> OP_NOP2 OP_DROP
        # OP_ENDIF
        # <sender-pubkey> OP_CHECKSIG
        #
        # lock until block 500,000
        rpc_result = self.nodes[0].decodescript('63' + push_public_key + 'ad670320a107b17568' + push_public_key + 'ac')
        assert_equal('OP_IF ' + public_key + ' OP_CHECKSIGVERIFY OP_ELSE 500000 OP_NOP2 OP_DROP OP_ENDIF ' + public_key + ' OP_CHECKSIG', rpc_result['asm'])

    def run_test(self):
        self.decodescript_script_sig()
        self.decodescript_script_pub_key()

if __name__ == '__main__':
    DecodeScriptTest().main()

