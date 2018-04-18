#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.mininode import CTransaction, NetworkThread
from test_framework.blocktools import create_coinbase, create_block
from test_framework.script import CScript
from io import BytesIO

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

'''
This test is meant to exercise NULLDUMMY softfork.
Connect to a single node.
Generate 2 blocks (save the coinbases for later).
Generate 427 more blocks.
[Policy/Consensus] Check that NULLDUMMY compliant transactions are accepted in the 430th block.
[Policy] Check that non-NULLDUMMY transactions are rejected before activation.
[Consensus] Check that the new NULLDUMMY rules are not enforced on the 431st block.
[Policy/Consensus] Check that the new NULLDUMMY rules are enforced on the 432nd block.
'''

class NULLDUMMYTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 1
        self.setup_clean_chain = True

    def setup_network(self):
        # Must set the blockversion for this test
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir,
                                 extra_args=[['-debug', '-whitelist=127.0.0.1']])

    def run_test(self):
        self.address = self.nodes[0].getnewaddress()
        self.ms_address = self.nodes[0].addmultisigaddress(1,[self.address])

        NetworkThread().start() # Start up network handling in another thread
        self.coinbase_blocks = self.nodes[0].generate(2) # Block 2
        coinbase_txid = []
        for i in self.coinbase_blocks:
            coinbase_txid.append(self.nodes[0].getblock(i)['tx'][0])
        self.nodes[0].generate(427) # Block 429
        self.lastblockhash = self.nodes[0].getbestblockhash()
        self.tip = int("0x" + self.lastblockhash, 0)
        self.lastblockheight = 429
        self.lastblocktime = get_mocktime() + 429

        print ("Test 1: NULLDUMMY compliant base transactions should be accepted to mempool and mined before activation [430]")
        test1txs = [self.create_transaction(self.nodes[0], coinbase_txid[0], self.ms_address, 49)]
        txid1 = self.tx_submit(self.nodes[0], test1txs[0])
        test1txs.append(self.create_transaction(self.nodes[0], txid1, self.ms_address, 48))
        txid2 = self.tx_submit(self.nodes[0], test1txs[1])
        self.block_submit(self.nodes[0], test1txs, True)

        print ("Test 2: Non-NULLDUMMY base multisig transaction should not be accepted to mempool before activation")
        test2tx = self.create_transaction(self.nodes[0], txid2, self.ms_address, 48)
        trueDummy(test2tx)
        txid4 = self.tx_submit(self.nodes[0], test2tx, NULLDUMMY_ERROR)

        print ("Test 3: Non-NULLDUMMY base transactions should be accepted in a block before activation [431]")
        self.block_submit(self.nodes[0], [test2tx], True)

        print ("Test 4: Non-NULLDUMMY base multisig transaction is invalid after activation")
        test4tx = self.create_transaction(self.nodes[0], txid4, self.address, 47)
        test6txs=[CTransaction(test4tx)]
        trueDummy(test4tx)
        self.tx_submit(self.nodes[0], test4tx, NULLDUMMY_ERROR)
        self.block_submit(self.nodes[0], [test4tx])

        print ("Test 6: NULLDUMMY compliant transactions should be accepted to mempool and in block after activation [432]")
        for i in test6txs:
            self.tx_submit(self.nodes[0], i)
        self.block_submit(self.nodes[0], test6txs, True)


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
            node.sendrawtransaction(bytes_to_hex_str(tx.serialize()), True)
        except JSONRPCException as exp:
            assert_equal(exp.error["message"], msg)
        else:
            assert_equal('', msg)
        return tx.hash


    def block_submit(self, node, txs, accept = False):
        block = create_block(self.tip, create_coinbase(self.lastblockheight + 1), self.lastblocktime + 1)
        block.nVersion = 4
        for tx in txs:
            tx.rehash()
            block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        node.submitblock(bytes_to_hex_str(block.serialize()))
        if (accept):
            assert_equal(node.getbestblockhash(), block.hash)
            self.tip = block.sha256
            self.lastblockhash = block.hash
            self.lastblocktime += 1
            self.lastblockheight += 1
        else:
            assert_equal(node.getbestblockhash(), self.lastblockhash)

if __name__ == '__main__':
    NULLDUMMYTest().main()
