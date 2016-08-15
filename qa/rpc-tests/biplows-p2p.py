#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.mininode import CTransaction, NetworkThread, ToHex
from test_framework.blocktools import create_coinbase, create_block, add_witness_commitment
from test_framework.comptool import TestManager
from test_framework.script import CScript
from io import BytesIO
import time

LOW_S_ERROR = "64: non-mandatory-script-verify-flag (Non-canonical signature: S value is unnecessarily high)"

def highSifyTx(tx):
    scriptSig = CScript(tx.vin[0].scriptSig)
    newscript = []
    for i in scriptSig:
        if (len(newscript) == 0):
            newscript.append(highSifySig(i))
        else:
            newscript.append(i)
    tx.vin[0].scriptSig = CScript(newscript)

def highSifySig(sig):
    '''
    Transform lowS signature to highS
    A canonical signature consists of:
    <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
    '''
    assert(len(sig) >= 9)
    rsize = sig[3]
    ssize = sig[5 + rsize]
    assert_equal(rsize+ssize+7, len(sig))
    lows = int.from_bytes(sig[6+rsize:-1], byteorder='big')
    assert(lows <= 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0)
    highs = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141 - lows
    highsbytes = (highs).to_bytes(33, byteorder='big')
    tsizebyte = (sig[1] + 33 - ssize).to_bytes(1, byteorder='big')
    newsig = b'\x30' + tsizebyte + sig[2:4+rsize] + b'\x02\x21' + highsbytes + sig[-1:]
    return newsig


'''
This test is meant to exercise BIP_LOW_S.
Connect to a single node.
Mine 3 (version 0x20000002) blocks (save the coinbases for later).
Generate 426 more version 0x20000002 blocks.
[Policy/Consensus] Check that LOW_S transaction is accepted in the 430th block.
[Policy] Check that HIGH_S transaction is rejected.
[Policy] Check that LOW_S transaction is accepted.
[Consensus] Check that the new LOW_S rules are not enforced on the 431st block.
[Policy/Consensus] Check that the new LOW_S rules are enforced on the 432nd block.
'''

class BIPLOWSTest(ComparisonTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 1

    def setup_network(self):
        # Must set the blockversion for this test
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir,
                                 extra_args=[['-debug', '-whitelist=127.0.0.1', '-blockversion=536870914', '-walletprematurewitness']])

    def run_test(self):
        self.nodeaddress = self.nodes[0].getnewaddress()
        self.nodewitaddress = self.nodes[0].addwitnessaddress(self.nodeaddress)

        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.coinbase_blocks = self.nodes[0].generate(3) # Block 3
        ctxid1 = self.nodes[0].getblock(self.coinbase_blocks[0])['tx'][0]
        ctxid2 = self.nodes[0].getblock(self.coinbase_blocks[1])['tx'][0]
        ctxid3 = self.nodes[0].getblock(self.coinbase_blocks[2])['tx'][0]
        self.nodes[0].generate(426) # Block 429
        self.last_block_time = int(time.time()) + 430

        print ("Test 1: LOW_S base transaction should be accepted to mempool and mined before activation [430]")
        spendtx = self.create_transaction(self.nodes[0], ctxid1, self.nodewitaddress, 1.0)
        txid1 = self.nodes[0].sendrawtransaction(ToHex(spendtx), True)
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[0].gettransaction(txid1)['blockhash'])
        self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)

        print ("Test 2: HIGH_S base transaction should not be accepted to mempool before activation")
        spendtx = self.create_transaction(self.nodes[0], ctxid2, self.nodewitaddress, 1.0)
        highSifyTx(spendtx)
        spendtx.rehash()
        try:
            self.nodes[0].sendrawtransaction(ToHex(spendtx), True)
        except JSONRPCException as exp:
            assert_equal(exp.error["message"], LOW_S_ERROR)
        else:
            assert(False)


        print ("Test 3: HIGH_S base transaction should be accepted in a block before activation [431]")
        block = create_block(self.tip, create_coinbase(431), self.last_block_time)
        block.nVersion = 4
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        self.nodes[0].submitblock(ToHex(block))
        assert_equal(self.nodes[0].getbestblockhash(), block.hash)
        self.tip = block.sha256
        lastblockhash = block.hash


        print ("Test 4: Block with HIGH_S base transaction should be rejected after activation")
        spendtx = self.create_transaction(self.nodes[0], ctxid3, self.nodewitaddress, 1.0)
        lowspendtxhex = ToHex(spendtx)
        highSifyTx(spendtx)
        spendtx.rehash()
        block = create_block(self.tip, create_coinbase(432), self.last_block_time + 1)
        block.nVersion = 4
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        self.nodes[0].submitblock(ToHex(block))
        assert_equal(self.nodes[0].getbestblockhash(), lastblockhash)


        print ("Test 5: HIGH_S witness transaction should not be accepted to mempool")
        spendwittx = self.create_transaction(self.nodes[0], txid1, self.nodewitaddress, 0.9)
        lowspendwittxhex = bytes_to_hex_str(spendwittx.serialize_with_witness())
        spendwittx.wit.vtxinwit[0].scriptWitness.stack[0] = highSifySig(spendwittx.wit.vtxinwit[0].scriptWitness.stack[0])
        spendwittx.rehash()
        highspendwittxhex = bytes_to_hex_str(spendwittx.serialize_with_witness())
        try:
            self.nodes[0].sendrawtransaction(highspendwittxhex, True)
        except JSONRPCException as exp:
            assert_equal(exp.error["message"], LOW_S_ERROR)
        else:
            assert(False)


        print ("Test 6: Block with HIGH_S witness transaction should be rejected after activation")
        block = create_block(self.tip, create_coinbase(432), self.last_block_time + 1)
        block.nVersion = 4
        block.vtx.append(spendwittx)
        block.hashMerkleRoot = block.calc_merkle_root()
        add_witness_commitment(block)
        block.rehash()
        block.solve()
        self.nodes[0].submitblock(bytes_to_hex_str(block.serialize(True)))
        assert_equal(self.nodes[0].getbestblockhash(), lastblockhash)


        print ("Test 7: LOW_S base/witness transactions should be accepted to mempool and mined after activation [432]")
        txid2 = self.nodes[0].sendrawtransaction(lowspendtxhex, True)
        txid3 = self.nodes[0].sendrawtransaction(lowspendwittxhex, True)
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[0].gettransaction(txid2)['blockhash'])
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[0].gettransaction(txid3)['blockhash'])


    def create_transaction(self, node, txid, to_address, amount):
        inputs = [{ "txid" : txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = node.createrawtransaction(inputs, outputs)
        signresult = node.signrawtransaction(rawtx)
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(signresult['hex']))
        tx.deserialize(f)
        return tx

if __name__ == '__main__':
    BIPLOWSTest().main()