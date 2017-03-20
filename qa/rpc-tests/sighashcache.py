#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.mininode import CTransaction, CTxOut, CTxIn, COutPoint, CTxInWitness
from test_framework.util import hex_str_to_bytes, start_node, bytes_to_hex_str
from test_framework.key import CECKey, CPubKey
from test_framework.script import CScript, OP_0, OP_1, OP_12, OP_14, OP_CHECKSIG, OP_CHECKMULTISIG, OP_CODESEPARATOR, OP_CHECKMULTISIGVERIFY, OP_CHECKSIGVERIFY, OP_HASH160, OP_EQUAL, SignatureHash, SIGHASH_ALL, hash160, sha256, SegwitVersion1SignatureHash, FindAndDelete
import time
from random import randint

'''
This is to test the correctness and performance of 2 types of sighash caches:
#8524: Intra/inter-input sighash midstate cache for segwit (BIP143)
#8654: Intra-input sighash reuse
'''

dummykey = hex_str_to_bytes("0300112233445566778899aabbccddeeff00112233445566778899aabbccddeeff")

# By default, we generate a transaction with 40 inputs and 40 outputs. Each input contains 14 sigOPs.
# scriptPubKey size for each output is 22000 bytes. The total uncached hashing size for SIGHASH_ALL is:
# 40 * 40 * 14 * (22200 + 32 + 4 + 4 + 8) = about 500MB

# Set 2 to reduce the testing time if we just want to test for correctness. Set to 1 for more accurate benchmarking
speedup = 2

default_nIn = 40 // speedup
default_nOut = default_nIn
default_outputsize = 22200 // speedup
default_amount = 200000
time_assertion = False # compare validation time with benchmarking test
verbose = False # print validation details

def recoverkey(sig, hash):
    r_size = sig[3]
    s_size = sig[5 + r_size]
    r_val = sig[4:4+r_size]
    s_val = sig[6+r_size:6+r_size+s_size]
    while (len(r_val) != 32):
        if (len(r_val) > 32):
            assert (r_val[0] == 0)
            r_val = r_val[1:]
        else:
            r_val = b'\x00' + r_val
    while (len(s_val) != 32):
        if (len(s_val) > 32):
            assert (s_val[0] == 0)
            s_val = s_val[1:]
        else:
            s_val = b'\x00' + s_val
    key = CECKey()
    key.set_compressed(True)
    key.recover(r_val, s_val, hash, len(hash), 0, 0)
    return key.get_pubkey()

class SigHashCacheTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True

    def setup_network(self):
        # Switch off STRICTENC so we could test strange nHashType
        self.nodes = [start_node(0, self.options.tmpdir, ["-promiscuousmempoolflags=8189"])]

    def generate_txpair(self, offset, witoffset = 250, nIn = default_nIn, nOut = default_nOut, outputsize = default_outputsize):
        # Generate a pair of transactions: non-segwit and segwit
        txpair = [CTransaction(), CTransaction()]
        for i in range(nIn):
            txpair[0].vin.append(CTxIn(COutPoint(self.txid,i+offset)))
            txpair[1].vin.append(CTxIn(COutPoint(self.txid,i+offset+witoffset)))
            txpair[1].wit.vtxinwit.append(CTxInWitness())
        for i in range(nOut):
            txpair[0].vout.append(CTxOut(1, b'\x00' * outputsize))
            txpair[1].vout.append(CTxOut(1, b'\x00' * outputsize))
        return txpair

    def validation_time(self, txpair):
        # sendrawtransaction and timing
        [tx, wtx] = txpair
        start = time.time()
        self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx.serialize_with_witness()), True)
        t = time.time() - start
        self.nodes[0].generate(1)
        start = time.time()
        self.nodes[0].sendrawtransaction(bytes_to_hex_str(wtx.serialize_with_witness()), True)
        wt = time.time() - start
        if (verbose):
            print ("**Non-witness**")
            print ("Transaction weight : " + str(len(tx.serialize_without_witness()) * 3 + len(tx.serialize_with_witness())))
            print ("Validation time    : " + str(t))
            print ("**Witness**")
            print ("Transaction weight : " + str(len(wtx.serialize_without_witness()) * 3 + len(wtx.serialize_with_witness())))
            print ("Validation time    : " + str(wt))
        self.nodes[0].generate(1)
        return [t, wt]

    def test_preparation(self):
        self.coinbase_blocks = self.nodes[0].generate(1)
        coinbase_txid = int("0x" + self.nodes[0].getblock(self.coinbase_blocks[0])['tx'][0], 0)
        self.nodes[0].generate(450) # to activate segwit
        self.key = CECKey()
        self.key.set_secretbytes(b"9")
        self.key.set_compressed(1)
        self.dummysig = []
        self.dummysig.append(self.key.sign(sha256(b'\x01')) + b'\x01')
        self.dummysig.append(self.key.sign(sha256(b'\x02')) + b'\x01')
        pubkey = CPubKey(self.key.get_pubkey())
        self.script = []
        scriptpubkey = []
        self.script.append(CScript([OP_14] + [pubkey] * 14 + [OP_14, OP_CHECKMULTISIG])) # 0, 250
        self.script.append(CScript([OP_1, pubkey] + [dummykey] * 13 + [OP_14, OP_CHECKMULTISIG])) # 500, 750
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY, OP_1, pubkey] + [dummykey] * 11 + [OP_12, OP_CHECKMULTISIGVERIFY, pubkey, OP_CHECKSIG])) # 1000, 1250
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY] * 13 + [pubkey, OP_CHECKSIG])) # 1500, 1750
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY, OP_CODESEPARATOR] * 13 + [pubkey, OP_CHECKSIG])) # 2000, 2250
        self.script.append(CScript([pubkey, OP_CHECKSIG])) # 2500, 2750
        self.script.append(CScript([pubkey, OP_CHECKSIGVERIFY] * 136 + [pubkey, OP_CHECKSIG])) # 3000, 3250 (Not valid for P2SH due to too big)
        self.script.append(CScript([OP_CHECKSIGVERIFY] * 14 + [self.dummysig[0]])) # 3500, 3750

        for i in self.script:
            scriptpubkey.append(CScript([OP_HASH160, hash160(i), OP_EQUAL])) # P2SH
            scriptpubkey.append(CScript([OP_0, sha256(i)])) # P2WSH

        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(coinbase_txid)))
        for i in scriptpubkey:
            for j in range(250):
                tx.vout.append(CTxOut(default_amount,i)) # 250 outputs for each script, non-segwit and segwit

        # Add bare outputs
        for i in range(250):
            tx.vout.append(CTxOut(default_amount,self.script[5])) # 4000-4249: 250 bare P2PK
        for i in range(25):
            # 4250-4274: 25 bare scriptPubKey with 137 CHECKSIGs. Pay more so it could pay enough fee when spending
            tx.vout.append(CTxOut(default_amount * 10,self.script[6]))
        for i in range(10):
            # 4275-4284: 10 bare FindAndDelete test
            tx.vout.append(CTxOut(default_amount, self.script[7]))

        # subScript for CODESEPERATOR tests
        self.csscript = []
        for i in range(14):
            self.csscript.append(CScript([pubkey, OP_CHECKSIGVERIFY, OP_CODESEPARATOR] * i + [pubkey, OP_CHECKSIG]))

        # subScript for FindAndDelete tests
        self.findanddeletescript = FindAndDelete(self.script[7], CScript([self.dummysig[0]]))

        signresult = self.nodes[0].signrawtransaction(bytes_to_hex_str(tx.serialize_without_witness()))['hex']
        self.txid = int("0x" + self.nodes[0].sendrawtransaction(signresult, True), 0)
        self.nodes[0].generate(1)

    def signtx(self, scripts, txpair, nIn, flags):
        sig = []
        wsig = []
        for i in (range(len(scripts))):
            sighash = SignatureHash(scripts[i], txpair[0], nIn, flags[i])[0]
            sig.append(self.key.sign(sighash) + chr(flags[i]).encode('latin-1'))
            wsighash = SegwitVersion1SignatureHash(scripts[i], txpair[1], nIn, flags[i], default_amount)
            wsig.append(self.key.sign(wsighash) + chr(flags[i]).encode('latin-1'))
        return [sig, wsig]

    def MS_14_of_14_different_ALL(self):
        print ("Test: 14-of-14 CHECKMULTISIG P2SH/P2WSH inputs with different variations of SIGHASH_ALL")
        script = self.script[0]
        txpair = self.generate_txpair(0)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx([script] * 14, txpair, i, range(4,18))
            txpair[0].vin[i].scriptSig = CScript([OP_0] + sig + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [b''] + wsig + [script]
        [t, wt] = self.validation_time(txpair)
        self.banchmark = t # For non-segwit this is equivalent to no cache
        if (time_assertion):
            assert(self.banchmark / wt > 4)

    def MS_14_of_14_same_ALL(self):
        script = self.script[0]
        print ("Test: 14-of-14 CHECKMULTISIG P2SH/P2WSH inputs with same SIGHASH_ALL")
        txpair = self.generate_txpair(50)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx([script], txpair, i, [SIGHASH_ALL])
            txpair[0].vin[i].scriptSig = CScript([OP_0] + [sig[0]] * 14 + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [b''] + [wsig[0]] * 14 + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t > 4)
            assert(self.banchmark / wt > 4)

    def MS_1_of_14_ALL(self):
        script = self.script[1]
        print ("Test: 1-of-14 CHECKMULTISIG P2SH/P2WSH inputs with SIGHASH_ALL")
        txpair = self.generate_txpair(500)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx([script], txpair, i, [SIGHASH_ALL])
            txpair[0].vin[i].scriptSig = CScript([OP_0, sig[0], script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [b'', wsig[0], script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t > 4)
            assert(self.banchmark / wt > 4)

    def mix_CHECKSIG_CHECKMULTISIG_same_ALL(self):
        script = self.script[2]
        print ("Test: CHECKSIG 1-of-13 CHECKMULTISIG CHECKSIG P2SH/P2WSH inputs with same SIGHASH_ALL")
        txpair = self.generate_txpair(1000)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx([script], txpair, i, [SIGHASH_ALL])
            txpair[0].vin[i].scriptSig = CScript([sig[0], OP_0, sig[0], sig[0], script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [wsig[0], b'', wsig[0], wsig[0], script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t > 4)
            assert(self.banchmark / wt > 4)

    def many_CHECKSIG_same_ALL(self):
        script = self.script[3]
        print ("Test: P2SH/P2WSH with 14 CHECKSIG with same SIGHASH_ALL")
        txpair = self.generate_txpair(1500)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx([script], txpair, i, [SIGHASH_ALL])
            txpair[0].vin[i].scriptSig = CScript([sig[0]] * 14 + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [wsig[0]] * 14 + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t > 4)
            assert(self.banchmark / wt > 4)

    def many_CHECKSIG_different_ALL(self):
        script = self.script[3]
        print ("Test: P2SH/P2WSH with 14 CHECKSIG with different variations of SIGHASH_ALL")
        txpair = self.generate_txpair(1550)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx([script] * 14, txpair, i, range(4,18))
            txpair[0].vin[i].scriptSig = CScript(sig + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = wsig + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t < 3)
            assert(t / self.banchmark < 3)
            assert(self.banchmark / wt > 4)

    def many_CHECKSIG_CODESEPERATOR_same_ALL(self):
        script = self.script[4]
        print ("Test: P2SH/P2WSH with 14 CHECKSIG CODESEPERATOR with same SIGHASH_ALL")
        txpair = self.generate_txpair(2000)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx(self.csscript, txpair, i, [SIGHASH_ALL] * 14)
            txpair[0].vin[i].scriptSig = CScript(sig + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = wsig + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t < 3)
            assert(t / self.banchmark < 3)
            assert(self.banchmark / wt > 4)

    def many_CHECKSIG_CODESEPERATOR_different_ALL(self):
        script = self.script[4]
        print ("Test: P2SH/P2WSH with 14 CHECKSIG CODESEPERATOR with different variations of SIGHASH_ALL")
        txpair = self.generate_txpair(2050)
        for i in range(default_nIn):
            [sig, wsig] = self.signtx(self.csscript, txpair, i, range(4,18))
            txpair[0].vin[i].scriptSig = CScript(sig + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = wsig + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t < 3)
            assert(t / self.banchmark < 3)
            assert(self.banchmark / wt > 4)

    def P2PK_ALL(self):
        script = self.script[5]
        print ("Test: Bare/segwit P2PK inputs with SIGHASH_ALL")
        txpair = self.generate_txpair(4000,-1250,250//speedup,250//speedup,350//speedup)
        for i in range(250//speedup):
            [sig, wsig] = self.signtx([script], txpair, i, [SIGHASH_ALL])
            txpair[0].vin[i].scriptSig = CScript(sig)
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [wsig[0], script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert (t / wt > 2)

    def many_CHECKSIG_random_flag(self):
        script = self.script[6]
        print ("Test: Bare/P2WSH inputs with 137 CHECKSIG with random SIGHASH")
        txpair = self.generate_txpair(4250,-1000,24//speedup,12//speedup,1000//speedup)
        # 24 inputs with only 12 outputs, so some SIGHASH_SINGLE will be unmatched
        for i in range(24//speedup):
            flags = []
            for j in range(137):
                flags.append(randint(0,255))
            [sig, wsig] = self.signtx([script] * 137, txpair, i, flags)
            txpair[0].vin[i].scriptSig = CScript(sig)
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = wsig + [script]
        self.validation_time(txpair)

    def FindAndDelete_reset(self):
        script = self.script[7]
        print ("Test: FindAndDelete in P2SH with sighash reset")
        '''
        The sighash cache is reset after every CHECKSIGVERIFY due to FindAndDelete.
        It should be as slow as no sighash cache.
        '''
        txpair = self.generate_txpair(3500)
        for i in range(default_nIn):
            sighash0 = SignatureHash(self.findanddeletescript, txpair[0], i, 1)[0]
            key0 = recoverkey(self.dummysig[0], sighash0)
            sighash1 = SignatureHash(script, txpair[0], i, 1)[0]
            key1 = recoverkey(self.dummysig[1], sighash1)
            assert (sighash0 != sighash1)
            wsighash = SegwitVersion1SignatureHash(script, txpair[1], i, 1, default_amount)
            wkey0 = recoverkey(self.dummysig[0], wsighash)
            wkey1 = recoverkey(self.dummysig[1], wsighash)

            txpair[0].vin[i].scriptSig = CScript([self.dummysig[1], key1, self.dummysig[0], key0] * 7 + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [self.dummysig[1], wkey1, self.dummysig[0], wkey0] *7 + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t < 3)
            assert(t / self.banchmark < 3)
            assert(self.banchmark / wt > 4)

    def FindAndDelete_noreset(self):
        script = self.script[7]
        print ("Test: FindAndDelete in P2SH without sighash reset")
        # No sighash reset due to FindAndDelete.
        txpair = self.generate_txpair(3550)
        for i in range(default_nIn // 2):
            sighash = SignatureHash(self.findanddeletescript, txpair[0], i, 1)[0]
            key = recoverkey(self.dummysig[0], sighash)
            wsighash = SegwitVersion1SignatureHash(script, txpair[1], i, 1, default_amount)
            wkey = recoverkey(self.dummysig[0], wsighash)
            txpair[0].vin[i].scriptSig = CScript([self.dummysig[0], key] * 14 + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [self.dummysig[0], wkey] * 14 + [script]
        for i in range(default_nIn // 2, default_nIn):
            sighash = SignatureHash(script, txpair[0], i, 1)[0]
            key = recoverkey(self.dummysig[1], sighash)
            wsighash = SegwitVersion1SignatureHash(script, txpair[1], i, 1, default_amount)
            wkey = recoverkey(self.dummysig[1], wsighash)
            txpair[0].vin[i].scriptSig = CScript([self.dummysig[1], key] * 14 + [script])
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [self.dummysig[1], wkey] * 14 + [script]
        [t, wt] = self.validation_time(txpair)
        if (time_assertion):
            assert(self.banchmark / t > 4)
            assert(self.banchmark / wt > 4)

    def FindAndDelete_bare(self):
        script = self.script[7]
        print ("Test: FindAndDelete in bare outputs")
        txpair = self.generate_txpair(4275, -300, 10, 10, 1)
        for i in range(5):
            sighash0 = SignatureHash(self.findanddeletescript, txpair[0], i, 1)[0]
            key0 = recoverkey(self.dummysig[0], sighash0)
            sighash1 = SignatureHash(script, txpair[0], i, 1)[0]
            key1 = recoverkey(self.dummysig[1], sighash1)
            assert (sighash0 != sighash1)
            wsighash = SegwitVersion1SignatureHash(script, txpair[1], i, 1, default_amount)
            wkey0 = recoverkey(self.dummysig[0], wsighash)
            wkey1 = recoverkey(self.dummysig[1], wsighash)
            txpair[0].vin[i].scriptSig = CScript([self.dummysig[1], key1, self.dummysig[0], key0] * 7)
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [self.dummysig[1], wkey1, self.dummysig[0], wkey0] * 7 + [script]
        for i in range(5, 10):
            sighash = SignatureHash(self.findanddeletescript, txpair[0], i, 1)[0]
            key = recoverkey(self.dummysig[0], sighash)
            wsighash = SegwitVersion1SignatureHash(script, txpair[1], i, 1, default_amount)
            wkey = recoverkey(self.dummysig[0], wsighash)
            txpair[0].vin[i].scriptSig = CScript([self.dummysig[0], key] * 14)
            txpair[1].wit.vtxinwit[i].scriptWitness.stack = [self.dummysig[0], wkey] * 14 + [script]
        [t, wt] = self.validation_time(txpair)

    def signing_test(self):
        print ("Test: signrawtransaction with different SIGHASH types")
        # Prepare for signing tests
        addresses = []
        addresses.append(self.nodes[0].getnewaddress())
        addresses.append(self.nodes[0].addmultisigaddress(3, [addresses[0]] * 3))
        addresses.append(self.nodes[0].addwitnessaddress(addresses[0]))
        addresses.append(self.nodes[0].addwitnessaddress(addresses[1]))

        outputs = {}
        for i in addresses:
            outputs[i] = 10
        self.signtest_txid = self.nodes[0].sendmany("", outputs)
        self.nodes[0].generate(1)

        hashtypes = ["ALL","NONE","SINGLE","ALL|ANYONECANPAY","NONE|ANYONECANPAY","SINGLE|ANYONECANPAY"]
        for i in range(6):
            inputs = []
            outputs = {}
            for j in range(5): # 5 because sendmany created a change output
                inputs.append({"txid":self.signtest_txid,"vout":j})
                outputs[self.nodes[0].getnewaddress()] = 1
            rawtx = self.nodes[0].createrawtransaction(inputs,outputs)
            signresult = self.nodes[0].signrawtransaction(rawtx,None,None,hashtypes[i])['complete']
            assert (signresult == True)

    def run_test(self):
        self.test_preparation()
        self.MS_14_of_14_different_ALL()
        self.MS_14_of_14_same_ALL()
        self.MS_1_of_14_ALL()
        self.mix_CHECKSIG_CHECKMULTISIG_same_ALL()
        self.many_CHECKSIG_same_ALL()
        self.many_CHECKSIG_different_ALL()
        self.many_CHECKSIG_CODESEPERATOR_same_ALL()
        self.many_CHECKSIG_CODESEPERATOR_different_ALL()
        self.P2PK_ALL()
        self.many_CHECKSIG_random_flag()
        self.FindAndDelete_reset()
        self.FindAndDelete_noreset()
        self.FindAndDelete_bare()
        self.signing_test()

if __name__ == '__main__':
    SigHashCacheTest().main()
