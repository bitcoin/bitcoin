# Copyright (C) 2016 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

from __future__ import absolute_import, division, print_function, unicode_literals

import unittest
import random
import sys

import bitcoin
from bitcoin.core import *
from bitcoin.core.script import *
from bitcoin.wallet import *

if sys.version > '3':
    _bytes = bytes
else:
    _bytes = lambda x: bytes(bytearray(x))

# Test serialization


class Test_Segwit(unittest.TestCase):

    # Test BIP 143 vectors
    def test_p2wpkh_signaturehash(self):
        unsigned_tx  = x('0100000002fff7f7881a8099afa6940d42d1e7f6362bec38171ea3edf433541db4e4ad969f0000000000eeffffffef51e1b804cc89d182d279655c3aa89e815b1b309fe287d9b2b55d57b90ec68a0100000000ffffffff02202cb206000000001976a9148280b37df378db99f66f85c95a783a76ac7a6d5988ac9093510d000000001976a9143bde42dbee7e4dbe6a21b2d50ce2f0167faa815988ac11000000')
        scriptpubkey  = CScript(x('00141d0f172a0ecb48aee1be1f2687d2963ae33f71a1'))
        value         = int(6*COIN)

        address = CBech32BitcoinAddress.from_scriptPubKey(scriptpubkey)
        self.assertEqual(SignatureHash(address.to_redeemScript(), CTransaction.deserialize(unsigned_tx),
                1, SIGHASH_ALL, value, SIGVERSION_WITNESS_V0),
            x('c37af31116d1b27caf68aae9e3ac82f1477929014d5b917657d0eb49478cb670'))

    def test_p2sh_p2wpkh_signaturehash(self):
        unsigned_tx   = x('0100000001db6b1b20aa0fd7b23880be2ecbd4a98130974cf4748fb66092ac4d3ceb1a54770100000000feffffff02b8b4eb0b000000001976a914a457b684d7f0d539a46a45bbc043f35b59d0d96388ac0008af2f000000001976a914fd270b1ee6abcaea97fea7ad0402e8bd8ad6d77c88ac92040000')
        scriptpubkey  = CScript(x('a9144733f37cf4db86fbc2efed2500b4f4e49f31202387'))
        redeemscript  = CScript(x('001479091972186c449eb1ded22b78e40d009bdf0089'))
        value         = int(10*COIN)

        address = CBase58BitcoinAddress.from_scriptPubKey(redeemscript)
        self.assertEqual(SignatureHash(address.to_redeemScript(), CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_ALL, value, SIGVERSION_WITNESS_V0),
            x('64f3b0f4dd2bb3aa1ce8566d220cc74dda9df97d8490cc81d89d735c92e59fb6'))

    def test_p2wsh_signaturehash1(self):
        unsigned_tx   = x('0100000002fe3dc9208094f3ffd12645477b3dc56f60ec4fa8e6f5d67c565d1c6b9216b36e0000000000ffffffff0815cf020f013ed6cf91d29f4202e8a58726b1ac6c79da47c23d1bee0a6925f80000000000ffffffff0100f2052a010000001976a914a30741f8145e5acadf23f751864167f32e0963f788ac00000000')
        scriptpubkey1 = CScript(x('21036d5c20fa14fb2f635474c1dc4ef5909d4568e5569b79fc94d3448486e14685f8ac'))
        value1        = int(1.5625*COIN)
        scriptpubkey2 = CScript(x('00205d1b56b63d714eebe542309525f484b7e9d6f686b3781b6f61ef925d66d6f6a0'))
        value2        = int(49*COIN)
        scriptcode1   = CScript(x('21026dccc749adc2a9d0d89497ac511f760f45c47dc5ed9cf352a58ac706453880aeadab210255a9626aebf5e29c0e6538428ba0d1dcf6ca98ffdf086aa8ced5e0d0215ea465ac'))
        # This is the same script with everything up to the last executed OP_CODESEPARATOR, including that
        # OP_CODESEPARATOR removed
        scriptcode2   = CScript(x('210255a9626aebf5e29c0e6538428ba0d1dcf6ca98ffdf086aa8ced5e0d0215ea465ac'))

        self.assertEqual(SignatureHash(scriptcode1, CTransaction.deserialize(unsigned_tx),
                1, SIGHASH_SINGLE, value2, SIGVERSION_WITNESS_V0),
            x('82dde6e4f1e94d02c2b7ad03d2115d691f48d064e9d52f58194a6637e4194391'))
        self.assertEqual(SignatureHash(scriptcode2, CTransaction.deserialize(unsigned_tx),
                1, SIGHASH_SINGLE, value2, SIGVERSION_WITNESS_V0),
            x('fef7bd749cce710c5c052bd796df1af0d935e59cea63736268bcbe2d2134fc47'))

    def test_p2wsh_signaturehash2(self):
        unsigned_tx   = x('0100000002e9b542c5176808107ff1df906f46bb1f2583b16112b95ee5380665ba7fcfc0010000000000ffffffff80e68831516392fcd100d186b3c2c7b95c80b53c77e77c35ba03a66b429a2a1b0000000000ffffffff0280969800000000001976a914de4b231626ef508c9a74a8517e6783c0546d6b2888ac80969800000000001976a9146648a8cd4531e1ec47f35916de8e259237294d1e88ac00000000')
        scriptpubkey1 = CScript(x('0020ba468eea561b26301e4cf69fa34bde4ad60c81e70f059f045ca9a79931004a4d'))
        value1        = int(0.16777215*COIN)
        witnessscript1= CScript(x('0063ab68210392972e2eb617b2388771abe27235fd5ac44af8e61693261550447a4c3e39da98ac'))
        scriptcode1   = CScript(x('0063ab68210392972e2eb617b2388771abe27235fd5ac44af8e61693261550447a4c3e39da98ac'))
        scriptpubkey2 = CScript(x('0020d9bbfbe56af7c4b7f960a70d7ea107156913d9e5a26b0a71429df5e097ca6537'))
        value2        = int(0.16777215*COIN)
        witnessscript2= CScript(x('5163ab68210392972e2eb617b2388771abe27235fd5ac44af8e61693261550447a4c3e39da98ac'))
        scriptcode2   = CScript(x('68210392972e2eb617b2388771abe27235fd5ac44af8e61693261550447a4c3e39da98ac'))

        self.assertEqual(SignatureHash(scriptcode1, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_SINGLE|SIGHASH_ANYONECANPAY, value1, SIGVERSION_WITNESS_V0),
            x('e9071e75e25b8a1e298a72f0d2e9f4f95a0f5cdf86a533cda597eb402ed13b3a'))
        self.assertEqual(SignatureHash(scriptcode2, CTransaction.deserialize(unsigned_tx),
                1, SIGHASH_SINGLE|SIGHASH_ANYONECANPAY, value2, SIGVERSION_WITNESS_V0),
            x('cd72f1f1a433ee9df816857fad88d8ebd97e09a75cd481583eb841c330275e54'))

    def test_p2sh_p2wsh_signaturehash(self):
        unsigned_tx   = x('010000000136641869ca081e70f394c6948e8af409e18b619df2ed74aa106c1ca29787b96e0100000000ffffffff0200e9a435000000001976a914389ffce9cd9ae88dcc0631e88a821ffdbe9bfe2688acc0832f05000000001976a9147480a33f950689af511e6e84c138dbbd3c3ee41588ac00000000')

        scriptPubKey = CScript(x('a9149993a429037b5d912407a71c252019287b8d27a587'))
        value        = int(9.87654321*COIN)
        redeemScript = CScript(x('0020a16b5755f7f6f96dbd65f5f0d6ab9418b89af4b1f14a1bb8a09062c35f0dcb54'))
        witnessscript= CScript(x('56210307b8ae49ac90a048e9b53357a2354b3334e9c8bee813ecb98e99a7e07e8c3ba32103b28f0c28bfab54554ae8c658ac5c3e0ce6e79ad336331f78c428dd43eea8449b21034b8113d703413d57761b8b9781957b8c0ac1dfe69f492580ca4195f50376ba4a21033400f6afecb833092a9a21cfdf1ed1376e58c5d1f47de74683123987e967a8f42103a6d48b1131e94ba04d9737d61acdaa1322008af9602b3b14862c07a1789aac162102d8b661b0b3302ee2f162b09e07a55ad5dfbe673a9f01d9f0c19617681024306b56ae'))

        self.assertEqual(SignatureHash(witnessscript, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_ALL, value, SIGVERSION_WITNESS_V0),
            x('185c0be5263dce5b4bb50a047973c1b6272bfbd0103a89444597dc40b248ee7c'))
        self.assertEqual(SignatureHash(witnessscript, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_NONE, value, SIGVERSION_WITNESS_V0),
            x('e9733bc60ea13c95c6527066bb975a2ff29a925e80aa14c213f686cbae5d2f36'))
        self.assertEqual(SignatureHash(witnessscript, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_SINGLE, value, SIGVERSION_WITNESS_V0),
            x('1e1f1c303dc025bd664acb72e583e933fae4cff9148bf78c157d1e8f78530aea'))
        self.assertEqual(SignatureHash(witnessscript, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_ALL|SIGHASH_ANYONECANPAY, value, SIGVERSION_WITNESS_V0),
            x('2a67f03e63a6a422125878b40b82da593be8d4efaafe88ee528af6e5a9955c6e'))
        self.assertEqual(SignatureHash(witnessscript, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_NONE|SIGHASH_ANYONECANPAY, value, SIGVERSION_WITNESS_V0),
            x('781ba15f3779d5542ce8ecb5c18716733a5ee42a6f51488ec96154934e2c890a'))
        self.assertEqual(SignatureHash(witnessscript, CTransaction.deserialize(unsigned_tx),
                0, SIGHASH_SINGLE|SIGHASH_ANYONECANPAY, value, SIGVERSION_WITNESS_V0),
            x('511e8e52ed574121fc1b654970395502128263f62662e076dc6baf05c2e6a99b'))

    def test_checkblock(self):
        # (No witness) coinbase generated by Bitcoin Core
        str_coinbase = '01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff03520101ffffffff0100f2052a01000000232102960c90bc04a631cb17922e4f5d80ac75fd590a88b8baaa5a3d5086ac85e4d788ac00000000'
        # No witness transaction
        str_tx = '0100000001e3d0c1d051a3abe79d5951dcab86f71d8926aff5caed5ff9bd72cb593e4ebaf5010000006b483045022100a28e1c57e160296953e1af85c5034bb1b907c12c50367da75ba547874a8a8c52022040682e888ddce7fd5c72519a9f28f22f5164c8af864d33332bbc7f65596ad4ae012102db30394fd5cc8288bed607b9382338240c014a98c9c0febbfb380db74ceb30a3ffffffff020d920000000000001976a914ad781c8ffcc18b2155433cba2da4601180a66eef88aca3170000000000001976a914bacb1c4b0725e61e385c7093b4260533953c8e1688ac00000000'
        # SegWit transaction
        str_w_tx = '0100000000010115e180dc28a2327e687facc33f10f2a20da717e5548406f7ae8b4c811072f8560100000000ffffffff0100b4f505000000001976a9141d7cd6c75c2e86f4cbf98eaed221b30bd9a0b92888ac02483045022100df7b7e5cda14ddf91290e02ea10786e03eb11ee36ec02dd862fe9a326bbcb7fd02203f5b4496b667e6e281cc654a2da9e4f08660c620a1051337fa8965f727eb19190121038262a6c6cec93c2d3ecd6c6072efea86d02ff8e3328bbd0242b20af3425990ac00000000'
        witness_nonce = _bytes(random.getrandbits(8) for _ in range(32))

        coinbase = CMutableTransaction.deserialize(x(str_coinbase))
        coinbase.wit = CTxWitness([CTxInWitness(CScriptWitness([witness_nonce]))])

        tx_legacy = CTransaction.deserialize(x(str_tx))
        tx_segwit = CTransaction.deserialize(x(str_w_tx))

        witness_merkle_root = CBlock.build_witness_merkle_tree_from_txs((coinbase, tx_legacy, tx_segwit))[-1]

        commitment = Hash(witness_merkle_root + witness_nonce)
        commitment_script = bitcoin.core.WITNESS_COINBASE_SCRIPTPUBKEY_MAGIC + commitment
        coinbase.vout.append(CTxOut(0, CScript(commitment_script)))

        block = CBlock(2, b'\x00'*32, b'\x00'*32, 0, 0, 0, (coinbase, tx_legacy, tx_segwit))

        CheckBlock(block, False, True)
