#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.mininode import CTransaction, CTxOut, CTxIn, COutPoint, CTxInWitness
from test_framework.util import *
from test_framework.script import CScript, OP_0, OP_1, OP_3, OP_CHECKMULTISIG, OP_HASH160, OP_EQUAL, hash160, sha256

# This is to test the sighash limit policy

class SigHashLimitTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True

    def setup_network(self):
        # Create 2 nodes. One enforces standardness rules and one does not.
        self.nodes = [start_node(0, self.options.tmpdir, ["-acceptnonstdtxn=0"])]
        self.nodes.append(start_node(1, self.options.tmpdir))
        connect_nodes(self.nodes[0], 1)

    def test_preparation(self):
        print ("Testing sighash limit policy")
        # Generate a block and get the coinbase txid.
        self.coinbase_blocks = self.nodes[1].generate(1)
        coinbase_txid = int("0x" + self.nodes[1].getblock(self.coinbase_blocks[0])['tx'][0], 0)
        self.nodes[1].generate(100)

        '''
        # By design, it is impossible to create a normal transaction below 400,000 weight,
        # while having excessive SigHash size.
        #
        # Here it creates a 0-of-3 multisig script, which is counted as 3 SigHashOp in P2SH.
        # Also create a witness program, which should be ignored in SigHashOp counting.
        '''
        self.script = CScript([OP_0, OP_0, OP_0, OP_0, OP_0, OP_3, OP_CHECKMULTISIG])
        self.p2sh = CScript([OP_HASH160, hash160(self.script), OP_EQUAL])
        self.p2wsh = CScript([OP_0, sha256(self.script)])

        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(coinbase_txid)))
        for i in range(1000):
            tx.vout.append(CTxOut(4500000, self.p2sh))
        for i in range(100):
            tx.vout.append(CTxOut(4500000, self.p2wsh))

        tx.rehash()
        signresults = self.nodes[1].signrawtransaction(bytes_to_hex_str(tx.serialize_with_witness()))['hex']
        self.txid = int("0x" + self.nodes[1].sendrawtransaction(signresults, True), 0)
        self.nodes[1].generate(1)
        sync_blocks(self.nodes)

        self.p2shcount = 0      # P2SH outputs start from 0
        self.p2wshcount = 1000   # P2WSH outputs start from 500

    def non_segwit_test(self):
        '''
        tx1:
            Size = 4 + 1 + 49 * 122 + 3 + 1674 * 32 + 4 = 59558
            Weight = 59558 * 4 = 238232
            Hashable size = 59558 - 8 * 122 = 58582
            SigHashOp = 3 * 122 = 366
            SigHashSize = 366 * 58582 = 21441012
            SigHashSize per Weight = 21441012 / 238232 = 90.0006

        tx2:
            Size = 4 + 1 + 49 * 122 + 3 + 1673 * 32 + 4 = 59526
            Weight = 59526 * 4 = 238104
            Hashable size = 59526 - 8 * 122 = 58550
            SigHashOp = 3 * 122 = 366
            SigHashSize = 366 * 58550 = 21429300
            SigHashSize per Weight = 21429300 / 238104 = 89.9997
        '''
        tx1 = CTransaction()
        tx2 = CTransaction()
        for i in range(122):
            tx1.vin.append(CTxIn(COutPoint(self.txid,self.p2shcount),CScript([self.script])))
            tx2.vin.append(CTxIn(COutPoint(self.txid,self.p2shcount + 1),CScript([self.script])))
            self.p2shcount += 2
        for i in range(1673):
            tx1.vout.append(CTxOut(1000, self.p2sh))
            tx2.vout.append(CTxOut(1000, self.p2sh))
        tx1.vout.append(CTxOut(1000, self.p2sh))    # Add one more output to tx1
        tx1.rehash()
        tx2.rehash()
        self.submit_pair(tx1, tx2)

    def segwit_test(self):
        '''
        tx1:
            Witness-stripped size = 4 + 1 + 49 * 122 + 41 * 20 + 3 + 1791 * 32 + 4 = 64122
            Witness size = 2 + 1 * 122 + 9 * 20 = 304
            Weight = 64122 * 4 + 304 = 256792
            Hashable size = 64122 - 8 * 122 = 63146
            SigHashOp = 3 * 122 = 366
            SigHashSize = 366 * 63146 = 23111436
            SigHashSize per Weight = 23111436 / 256792 = 90.0006

        tx2:
            Witness-stripped size = 4 + 1 + 49 * 122 + 41 * 20 + 3 + 1790 * 32 + 4 = 64090
            Witness size = 2 + 1 * 122 + 9 * 20 = 304
            Weight = 64090 * 4 + 304 = 256664
            Hashable size = 64090 - 8 * 122 = 63114
            SigHashOp = 3 * 122 = 366
            SigHashSize = 366 * 63114 = 23099724
            SigHashSize per Weight = 23099724 / 256664 = 89.9999
        '''
        tx1 = CTransaction()
        tx2 = CTransaction()
        for i in range(20):
            tx1.vin.append(CTxIn(COutPoint(self.txid,self.p2wshcount)))
            tx1.wit.vtxinwit.append(CTxInWitness())
            tx1.wit.vtxinwit[i].scriptWitness.stack = [self.script]
            tx2.vin.append(CTxIn(COutPoint(self.txid,self.p2wshcount + 1)))
            tx2.wit.vtxinwit.append(CTxInWitness())
            tx2.wit.vtxinwit[i].scriptWitness.stack = [self.script]
            self.p2wshcount += 2
        for i in range(122):
            tx1.vin.append(CTxIn(COutPoint(self.txid,self.p2shcount),CScript([self.script])))
            tx2.vin.append(CTxIn(COutPoint(self.txid,self.p2shcount + 1),CScript([self.script])))
            self.p2shcount += 2
        for i in range(1790):
            tx1.vout.append(CTxOut(1000, self.p2sh))
            tx2.vout.append(CTxOut(1000, self.p2sh))
        tx1.vout.append(CTxOut(1000, self.p2sh))    # Add one more output to tx1
        tx1.rehash()
        tx2.rehash()
        self.submit_pair(tx1, tx2)

    def submit_pair(self, tx1, tx2):
        # Non-standard node should accept both tx1 and tx2. Standard node should accept only tx2
        try:
            self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx1.serialize_with_witness()), True)
        except JSONRPCException as exp:
            assert_equal(exp.error["message"], "64: bad-txns-nonstandard-too-much-sighashing")
        else:
            assert(False)
        self.nodes[1].sendrawtransaction(bytes_to_hex_str(tx1.serialize_with_witness()), True)
        self.nodes[0].sendrawtransaction(bytes_to_hex_str(tx2.serialize_with_witness()), True)
        self.nodes[1].sendrawtransaction(bytes_to_hex_str(tx2.serialize_with_witness()), True)
        self.nodes[1].generate(1)
        sync_blocks(self.nodes)

    def run_test(self):
        self.test_preparation()
        self.non_segwit_test()          # Test non-segwit P2SH before segwit activation
        self.nodes[0].generate(400)     # Activate segwit
        sync_blocks(self.nodes)
        self.non_segwit_test()          # Test non-segwit P2SH after segwit activation
        self.segwit_test()              # Test non-segwit P2SH mixed with P2WSH

if __name__ == '__main__':
    SigHashLimitTest().main()