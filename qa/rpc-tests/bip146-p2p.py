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
from test_framework.key import SECP256K1_ORDER, SECP256K1_ORDER_HALF
from io import BytesIO
import time

LOW_S_ERROR = "64: non-mandatory-script-verify-flag (Non-canonical signature: S value is unnecessarily high)"
NULLDUMMY_ERROR = "64: non-mandatory-script-verify-flag (Dummy CHECKMULTISIG argument must be zero)"

def trueDummy(tx):
    scriptSig = CScript(tx.vin[0].scriptSig)
    newscript = []
    for i in scriptSig:
        if (len(newscript) == 0):
            assert(len(i) == 0)
            newscript.append(b'\x51')
        else:
            newscript.append(i)
    tx.vin[0].scriptSig = CScript(newscript)
    tx.rehash()

def isCanonicalSig(sig):
    '''
    A canonical signature consists of:
    <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
    '''
    if (len(sig) < 9 or len(sig) > 73 or sig[0] != 0x30):
        return False
    if (sig[1] != len(sig) - 3):
        return False
    lenR = sig[3]
    if (len(sig) < 8 + lenR):
        return False
    lenS = sig[5 + lenR]
    if (len(sig) != lenR + lenS + 7):
        return False
    if (lenR == 0 or lenR > 33 or sig[2] != 0x02 or sig[4] & 0x80):
        return False
    if (lenS == 0 or lenS > 33 or sig[4 + lenR] != 0x02 or sig[6 + lenR] & 0x80):
        return False
    if (lenR > 1 and sig[4] == 0x00 and not sig[5] & 0x80):
        return False
    if (lenS > 1 and sig[lenS + 6] == 0x00 and not sig[lenS + 7] & 0x80):
        return False
    return True

def highSifyTx(tx):
    scriptSig = CScript(tx.vin[0].scriptSig)
    newscript = []
    for i in scriptSig:
        if (isCanonicalSig(i)):
            i = highSifySig(i)
        newscript.append(i)
    tx.vin[0].scriptSig = CScript(newscript)
    tx.rehash()

def highSifySig(sig):
    assert(isCanonicalSig(sig))
    rsize = sig[3]
    s = int.from_bytes(sig[6+rsize:-1], byteorder='big')
    assert(s <= SECP256K1_ORDER_HALF)
    s = SECP256K1_ORDER - s
    highsbytes = s.to_bytes(33, byteorder='big')
    tsizebyte = (rsize + 37).to_bytes(1, byteorder='big')
    sig = b'\x30' + tsizebyte + sig[2:5+rsize] + b'\x21' + highsbytes + sig[-1:]
    return sig


'''
This test is meant to exercise BIP146.
Connect to a single node.
Generate 4 blocks (save the coinbases for later).
Generate 425 more blocks.
[Policy/Consensus] Check that LOW_S and NULLDUMMY compliant transactions are accepted in the 430th block.
[Policy] Check that HIGH_S and non-NULLDUMMY transactions are rejected before activation.
[Consensus] Check that the new LOW_S and NULLDUMMY rules are not enforced on the 431st block.
[Policy/Consensus] Check that the new LOW_S and NULLDUMMY rules are enforced on the 432nd block.
'''

class BIP146Test(ComparisonTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 1

    def setup_network(self):
        # Must set the blockversion for this test
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir,
                                 extra_args=[['-debug', '-whitelist=127.0.0.1', '-walletprematurewitness']])

    def run_test(self):
        self.address = self.nodes[0].getnewaddress()
        self.ms_address = self.nodes[0].addmultisigaddress(1,[self.address])
        self.wit_address = self.nodes[0].addwitnessaddress(self.address)
        self.wit_ms_address = self.nodes[0].addwitnessaddress(self.ms_address)

        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        self.coinbase_blocks = self.nodes[0].generate(4) # Block 4
        coinbase_txid = []
        for i in self.coinbase_blocks:
            coinbase_txid.append(self.nodes[0].getblock(i)['tx'][0])
        self.nodes[0].generate(425) # Block 429
        self.lastblockhash = self.nodes[0].getbestblockhash()
        self.tip = int("0x" + self.lastblockhash, 0)
        self.lastblockheight = 429
        self.lastblocktime = int(time.time()) + 429

        print ("Test 1: LOW_S and NULLDUMMY compliant base transactions should be accepted to mempool and mined before activation [430]")
        test1txs = [self.create_transaction(self.nodes[0], coinbase_txid[0], self.ms_address, 49)]
        txid1 = self.tx_submit(self.nodes[0], test1txs[0])
        test1txs.append(self.create_transaction(self.nodes[0], txid1, self.ms_address, 48))
        txid2 = self.tx_submit(self.nodes[0], test1txs[1])
        test1txs.append(self.create_transaction(self.nodes[0], coinbase_txid[1], self.wit_ms_address, 49))
        txid3 = self.tx_submit(self.nodes[0], test1txs[2])
        self.block_submit(self.nodes[0], test1txs, False, True)

        print ("Test 2: HIGH_S P2PK transaction should not be accepted to mempool before activation")
        test2tx = self.create_transaction(self.nodes[0], coinbase_txid[2], self.wit_address, 49)
        highSifyTx(test2tx)
        txid4 = test2tx.hash
        self.tx_submit(self.nodes[0], test2tx, LOW_S_ERROR)

        print ("Test 3: non-NULLDUMMY base multisig transaction should not be accepted to mempool before activation")
        test3tx = self.create_transaction(self.nodes[0], txid2, self.ms_address, 48)
        test4tx = CTransaction(test3tx)
        trueDummy(test3tx)
        self.tx_submit(self.nodes[0], test3tx, NULLDUMMY_ERROR)

        print ("Test 4: HIGH_S base multisig transaction should not be accepted to mempool before activation")
        highSifyTx(test4tx)
        self.tx_submit(self.nodes[0], test4tx, LOW_S_ERROR)

        print ("Test 5: HIGH_S and non-NULLDUMMY base transactions should be accepted in a block before activation [431]")
        highSifyTx(test3tx) # make it both HIGH_S and non-NULLDUMMY
        txid5 = test3tx.hash
        self.block_submit(self.nodes[0], [test2tx, test3tx], False, True)

        print ("Test 6: HIGH_S P2PK transaction is invalid after activation")
        test6tx = self.create_transaction(self.nodes[0], coinbase_txid[3], self.address, 49)
        test12txs = [CTransaction(test6tx)]
        highSifyTx(test6tx)
        self.tx_submit(self.nodes[0], test6tx, LOW_S_ERROR)
        self.block_submit(self.nodes[0], [test6tx])

        print ("Test 7: Non-NULLDUMMY base multisig transaction is invalid after activation")
        test7tx = self.create_transaction(self.nodes[0], txid5, self.address, 47)
        test8tx = CTransaction(test7tx)
        test12txs.append(CTransaction(test7tx))
        trueDummy(test7tx)
        self.tx_submit(self.nodes[0], test7tx, NULLDUMMY_ERROR)
        self.block_submit(self.nodes[0], [test7tx])

        print ("Test 8: HIGH_S base multisig transaction is invalid after activation")
        highSifyTx(test8tx)
        self.tx_submit(self.nodes[0], test8tx, LOW_S_ERROR)
        self.block_submit(self.nodes[0], [test8tx])

        print ("Test 9: HIGH_S P2WPKH transaction is invalid after activation")
        test9tx = self.create_transaction(self.nodes[0], txid4, self.address, 48)
        test12txs.append(CTransaction(test9tx))
        test9tx.wit.vtxinwit[0].scriptWitness.stack[0] = highSifySig(test9tx.wit.vtxinwit[0].scriptWitness.stack[0])
        self.tx_submit(self.nodes[0], test9tx, LOW_S_ERROR)
        self.block_submit(self.nodes[0], [test9tx], True)

        print ("Test 10: HIGH_S P2WSH multisig transaction is invalid after activation")
        test10tx = self.create_transaction(self.nodes[0], txid3, self.wit_address, 48)
        test11tx = CTransaction(test10tx)
        test12txs.append(CTransaction(test10tx))
        test10tx.wit.vtxinwit[0].scriptWitness.stack[1] = highSifySig(test10tx.wit.vtxinwit[0].scriptWitness.stack[1])
        self.tx_submit(self.nodes[0], test10tx, LOW_S_ERROR)
        self.block_submit(self.nodes[0], [test10tx], True)

        print ("Test 11: Non-NULLDUMMY P2WSH multisig transaction invalid after activation")
        test11tx.wit.vtxinwit[0].scriptWitness.stack[0] = b'\x01'
        self.tx_submit(self.nodes[0], test11tx, NULLDUMMY_ERROR)
        self.block_submit(self.nodes[0], [test11tx], True)

        print ("Test 12: LOW_S and NULLDUMMY compliant base/witness transactions should be accepted to mempool and mined after activation [432]")
        for i in test12txs:
            self.tx_submit(self.nodes[0], i)
        self.block_submit(self.nodes[0], test12txs, True, True)


    def create_transaction(self, node, txid, to_address, amount):
        inputs = [{ "txid" : txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = node.createrawtransaction(inputs, outputs)
        signresult = node.signrawtransaction(rawtx)
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(signresult['hex']))
        tx.deserialize(f)
        return tx


    def tx_submit(self, node, tx, msg = ""):
        tx.rehash()
        try:
            node.sendrawtransaction(bytes_to_hex_str(tx.serialize_with_witness()), True)
        except JSONRPCException as exp:
            assert_equal(exp.error["message"], msg)
        return tx.hash


    def block_submit(self, node, txs, witness = False, accept = False):
        block = create_block(self.tip, create_coinbase(self.lastblockheight + 1), self.lastblocktime + 1)
        block.nVersion = 4
        for tx in txs:
            tx.rehash()
            block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        witness and add_witness_commitment(block)
        block.rehash()
        block.solve()
        node.submitblock(bytes_to_hex_str(block.serialize(True)))
        if (accept):
            assert_equal(node.getbestblockhash(), block.hash)
            self.tip = block.sha256
            self.lastblockhash = block.hash
            self.lastblocktime += 1
            self.lastblockheight += 1
        else:
            assert_equal(node.getbestblockhash(), self.lastblockhash)

if __name__ == '__main__':
    BIP146Test().main()