#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test NULLDUMMY softfork.

Connect to a single node.
Generate 2 blocks (save the coinbases for later).
Generate 427 more blocks.
[Policy/Consensus] Check that NULLDUMMY compliant transactions are accepted in the 430th block.
[Policy] Check that non-NULLDUMMY transactions are rejected before activation.
[Consensus] Check that the new NULLDUMMY rules are not enforced on the 431st block.
[Policy/Consensus] Check that the new NULLDUMMY rules are enforced on the 432nd block.
"""
import time

from test_framework.blocktools import NORMAL_GBT_REQUEST_PARAMS, create_block, create_transaction, add_witness_commitment
from test_framework.messages import CTransaction
from test_framework.script import CScript
from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

NULLDUMMY_ERROR = "non-mandatory-script-verify-flag (Dummy CHECKMULTISIG argument must be zero)"

def trueDummy(tx):
    scriptSig = CScript(tx.vin[0].scriptSig)
    newscript = []
    for i in scriptSig:
        if (len(newscript) == 0):
            assert len(i) == 0
            newscript.append(b'\x51')
        else:
            newscript.append(i)
    tx.vin[0].scriptSig = CScript(newscript)
    tx.rehash()

class NULLDUMMYTest(WidecoinTestFramework):

    def set_test_params(self):
        # Need two nodes only so GBT doesn't complain that it's not connected
        self.num_nodes = 2
        self.setup_clean_chain = True
        # This script tests NULLDUMMY activation, which is part of the 'segwit' deployment, so we go through
        # normal segwit activation here (and don't use the default always-on behaviour).
        self.extra_args = [[
            '-segwitheight=432',
            '-addresstype=legacy',
        ]] * 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].createwallet(wallet_name='wmulti', disable_private_keys=True)
        wmulti = self.nodes[0].get_wallet_rpc('wmulti')
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.address = w0.getnewaddress()
        self.pubkey = w0.getaddressinfo(self.address)['pubkey']
        self.ms_address = wmulti.addmultisigaddress(1, [self.pubkey])['address']
        self.wit_address = w0.getnewaddress(address_type='p2sh-segwit')
        self.wit_ms_address = wmulti.addmultisigaddress(1, [self.pubkey], '', 'p2sh-segwit')['address']
        if not self.options.descriptors:
            # Legacy wallets need to import these so that they are watched by the wallet. This is unnecssary (and does not need to be tested) for descriptor wallets
            wmulti.importaddress(self.ms_address)
            wmulti.importaddress(self.wit_ms_address)

        self.coinbase_blocks = self.nodes[0].generate(2)  # Block 2
        coinbase_txid = []
        for i in self.coinbase_blocks:
            coinbase_txid.append(self.nodes[0].getblock(i)['tx'][0])
        self.nodes[0].generate(427)  # Block 429
        self.lastblockhash = self.nodes[0].getbestblockhash()
        self.lastblockheight = 429
        self.lastblocktime = int(time.time()) + 429

        self.log.info("Test 1: NULLDUMMY compliant base transactions should be accepted to mempool and mined before activation [430]")
        test1txs = [create_transaction(self.nodes[0], coinbase_txid[0], self.ms_address, amount=49)]
        txid1 = self.nodes[0].sendrawtransaction(test1txs[0].serialize_with_witness().hex(), 0)
        test1txs.append(create_transaction(self.nodes[0], txid1, self.ms_address, amount=48))
        txid2 = self.nodes[0].sendrawtransaction(test1txs[1].serialize_with_witness().hex(), 0)
        test1txs.append(create_transaction(self.nodes[0], coinbase_txid[1], self.wit_ms_address, amount=49))
        txid3 = self.nodes[0].sendrawtransaction(test1txs[2].serialize_with_witness().hex(), 0)
        self.block_submit(self.nodes[0], test1txs, False, True)

        self.log.info("Test 2: Non-NULLDUMMY base multisig transaction should not be accepted to mempool before activation")
        test2tx = create_transaction(self.nodes[0], txid2, self.ms_address, amount=47)
        trueDummy(test2tx)
        assert_raises_rpc_error(-26, NULLDUMMY_ERROR, self.nodes[0].sendrawtransaction, test2tx.serialize_with_witness().hex(), 0)

        self.log.info("Test 3: Non-NULLDUMMY base transactions should be accepted in a block before activation [431]")
        self.block_submit(self.nodes[0], [test2tx], False, True)

        self.log.info("Test 4: Non-NULLDUMMY base multisig transaction is invalid after activation")
        test4tx = create_transaction(self.nodes[0], test2tx.hash, self.address, amount=46)
        test6txs = [CTransaction(test4tx)]
        trueDummy(test4tx)
        assert_raises_rpc_error(-26, NULLDUMMY_ERROR, self.nodes[0].sendrawtransaction, test4tx.serialize_with_witness().hex(), 0)
        self.block_submit(self.nodes[0], [test4tx])

        self.log.info("Test 5: Non-NULLDUMMY P2WSH multisig transaction invalid after activation")
        test5tx = create_transaction(self.nodes[0], txid3, self.wit_address, amount=48)
        test6txs.append(CTransaction(test5tx))
        test5tx.wit.vtxinwit[0].scriptWitness.stack[0] = b'\x01'
        assert_raises_rpc_error(-26, NULLDUMMY_ERROR, self.nodes[0].sendrawtransaction, test5tx.serialize_with_witness().hex(), 0)
        self.block_submit(self.nodes[0], [test5tx], True)

        self.log.info("Test 6: NULLDUMMY compliant base/witness transactions should be accepted to mempool and in block after activation [432]")
        for i in test6txs:
            self.nodes[0].sendrawtransaction(i.serialize_with_witness().hex(), 0)
        self.block_submit(self.nodes[0], test6txs, True, True)

    def block_submit(self, node, txs, witness=False, accept=False):
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(tmpl['previousblockhash'], self.lastblockhash)
        assert_equal(tmpl['height'], self.lastblockheight + 1)
        block = create_block(tmpl=tmpl, ntime=self.lastblocktime + 1)
        for tx in txs:
            tx.rehash()
            block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        witness and add_witness_commitment(block)
        block.rehash()
        block.solve()
        assert_equal(None if accept else 'block-validation-failed', node.submitblock(block.serialize().hex()))
        if (accept):
            assert_equal(node.getbestblockhash(), block.hash)
            self.lastblockhash = block.hash
            self.lastblocktime += 1
            self.lastblockheight += 1
        else:
            assert_equal(node.getbestblockhash(), self.lastblockhash)

if __name__ == '__main__':
    NULLDUMMYTest().main()
