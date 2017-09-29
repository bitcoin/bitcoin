#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the rawtransaction RPCs.

Test the following RPCs:
   - createrawtransaction
   - signrawtransaction
   - sendrawtransaction
   - decoderawtransaction
   - getrawtransaction
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

# Create one-input, one-output, no-fee transaction:
class RawTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self, split=False):
        super().setup_network()
        connect_nodes_bi(self.nodes,0,2)

    def run_test(self):

        #prepare some coins for multiple *rawtransaction commands
        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(101)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.5)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),5.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),4.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),3.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),2.0)
        self.sync_all()
        self.nodes[0].generate(5)
        self.sync_all()

        unspent = self.nodes[2].listunspent()

        fivebtc  = [tx for tx in unspent if tx["amount"] == Decimal("5.0")][0]
        fourbtc  = [tx for tx in unspent if tx["amount"] == Decimal("4.0")][0]
        threebtc = [tx for tx in unspent if tx["amount"] == Decimal("3.0")][0]
        twobtc   = [tx for tx in unspent if tx["amount"] == Decimal("2.0")][0]

        #########################################
        # {create|verify|send}rawtransaction with valid or empty transaction #
        #########################################
        inputs  = [ {'txid' : fivebtc["txid"], 'vout' : fivebtc["vout"]} ]
        outputs = { self.nodes[0].getnewaddress(): 4.9 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)

        # This transaction is not signed and thus should not validate
        res = self.nodes[2].verifyrawtransactions([rawtx])
        assert_equal(res['valid'], False)
        assert_equal("mandatory-script-verify" in res['reason'], True)

        # Sign the transaction
        rawtx = self.nodes[2].signrawtransaction(rawtx)

        # This transaction is signed and valid, so verifyrawtransaction should succeed
        res = self.nodes[2].verifyrawtransactions([rawtx['hex']])
        assert_equal(res['valid'], True)
        assert_equal("appear to be valid" in res['reason'], True)

        # Verifying no transactions should succeed
        res = self.nodes[2].verifyrawtransactions([])
        assert_equal(res['valid'], True)

        res = self.nodes[2].verifyrawtransactions([], False)
        assert_equal(res['valid'], True)

        # Add transaction to mempool
        assert_equal(len(self.nodes[2].getrawmempool()), 0)
        self.nodes[2].sendrawtransaction(rawtx['hex'])

        # mempool should have one entry now
        assert_equal(len(self.nodes[2].getrawmempool()), 1)

        # verifyrawtransactions should fail now since the tx is already in the mempool
        res = self.nodes[2].verifyrawtransactions([rawtx['hex']])
        assert_equal(res['valid'], False)
        assert_equal("mempool" in res['reason'], True)

        # mempool should still have one entry (should not be touched by verifyrawtransaction)
        assert_equal(len(self.nodes[2].getrawmempool()), 1)

        # Mine the transaction
        self.sync_all()
        self.nodes[2].generate(1)
        self.sync_all()

        # Once confirmed, should no longer be spendable
        res = self.nodes[2].verifyrawtransactions([rawtx['hex']])
        assert_equal(res['valid'], False)
        assert_equal("chain" in res['reason'], True)

        #########################################
        # {create|verify|send}rawtransaction with verifying multiple tx/mempool ancestor #
        #########################################
        inputs  = [ {'txid' : fourbtc["txid"], 'vout' : fourbtc["vout"]} ]
        outputs = { self.nodes[2].getnewaddress(): 3.9 }
        rawtx0  = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx0  = self.nodes[2].signrawtransaction(rawtx0)

        # This transaction is signed and valid, so verifyrawtransaction should succeed
        res = self.nodes[2].verifyrawtransactions([rawtx0['hex']])
        assert_equal(res['valid'], True)

        txdec = self.nodes[2].decoderawtransaction(rawtx0["hex"])

        # Create another transaction dependent on the first
        inputs  = [ {'txid' : txdec["txid"], 'vout' : 0} ]
        outputs = { self.nodes[2].getnewaddress(): 3.8 }
        rawtx1  = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx1  = self.nodes[2].signrawtransaction(rawtx1, [{"txid": txdec["txid"], "vout": 0,
                                    "scriptPubKey": txdec["vout"][0]["scriptPubKey"]["hex"]}])

        txdec = self.nodes[2].decoderawtransaction(rawtx1["hex"])

        # Another, so we can test multiple mempool ancestors
        inputs  = [ {'txid' : txdec["txid"], 'vout' : 0} ]
        outputs = { self.nodes[2].getnewaddress(): 3.7 }
        rawtx2  = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx2  = self.nodes[2].signrawtransaction(rawtx2, [{"txid": txdec["txid"], "vout": 0,
                                    "scriptPubKey": txdec["vout"][0]["scriptPubKey"]["hex"]}])

        txdec = self.nodes[2].decoderawtransaction(rawtx2["hex"])

        # One last one to test that we can do local policy checks on transactions
        # that don't have mempool ancestors
        inputs  = [ {'txid' : txdec["txid"], 'vout' : 0} ]
        outputs = { self.nodes[2].getnewaddress(): 3.7 } # no fee
        rawtx3  = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx3  = self.nodes[2].signrawtransaction(rawtx3, [{"txid": txdec["txid"], "vout": 0,
                                    "scriptPubKey": txdec["vout"][0]["scriptPubKey"]["hex"]}])

        # We don't know about the first tx yet, so verifyrawtransaction should fail
        res = self.nodes[2].verifyrawtransactions([rawtx1['hex']])
        assert_equal(res['valid'], False)
        assert_equal("missingorspent" in res['reason'], True)

        res = self.nodes[2].verifyrawtransactions([rawtx2['hex']])
        assert_equal(res['valid'], False)
        assert_equal("missingorspent" in res['reason'], True)

        # Passing 0, 1, 2 or 0, 1 should work, though
        res = self.nodes[2].verifyrawtransactions([rawtx0['hex'], rawtx1['hex']])
        assert_equal(res['valid'], True)

        res = self.nodes[2].verifyrawtransactions([rawtx0['hex'], rawtx1['hex'], rawtx2['hex']])
        assert_equal(res['valid'], True)

        # Order shouldn't matter
        res = self.nodes[2].verifyrawtransactions([rawtx1['hex'], rawtx0['hex']])
        assert_equal(res['valid'], True)

        res = self.nodes[2].verifyrawtransactions([rawtx2['hex'], rawtx0['hex'], rawtx1['hex']])
        assert_equal(res['valid'], True)

        # Transaction with insufficient fee should be caught, but not if use_local_policy is false
        res = self.nodes[2].verifyrawtransactions([rawtx2['hex'], rawtx0['hex'], rawtx1['hex'], rawtx3['hex']])
        assert_equal(res['valid'], False)
        assert_equal("relay fee" in res['reason'], True)

        res = self.nodes[2].verifyrawtransactions([rawtx2['hex'], rawtx0['hex'], rawtx1['hex'], rawtx3['hex']], False)
        assert_equal(res['valid'], True)

        # Add the first transaction to the mempool
        self.nodes[2].sendrawtransaction(rawtx0['hex'])

        # Now that the first tx is in the mempool, verifyrawtransactions for the second should succeed
        res = self.nodes[2].verifyrawtransactions([rawtx1['hex']])
        assert_equal(res['valid'], True)

        # Third should still fail
        res = self.nodes[2].verifyrawtransactions([rawtx2['hex']])
        assert_equal(res['valid'], False)
        assert_equal("missingorspent" in res['reason'], True)

        # Add the second transaction to the mempool
        self.nodes[2].sendrawtransaction(rawtx1['hex'])

        # Third should work now
        res = self.nodes[2].verifyrawtransactions([rawtx2['hex']])
        assert_equal(res['valid'], True)

        #########################################
        # {create|verify}rawtransaction with violation of local rules #
        #########################################

        inputs  = [ {'txid' : threebtc["txid"], 'vout' : threebtc["vout"]} ]
        outputs = { self.nodes[2].getnewaddress(): 3.0 } # no fee

        nofee = self.nodes[2].createrawtransaction(inputs, outputs)
        nofee = self.nodes[2].signrawtransaction(nofee)

        # There is no fee, so verifyrawtransactions should fail if use_local_rules is default or true
        res = self.nodes[2].verifyrawtransactions([nofee['hex']])
        assert_equal(res['valid'], False)
        assert_equal("relay fee" in res['reason'], True)

        res = self.nodes[2].verifyrawtransactions([nofee['hex']], True)
        assert_equal(res['valid'], False)
        assert_equal("relay fee" in res['reason'], True)

        # But this transaction would be fine if it were mined into a block
        res = self.nodes[2].verifyrawtransactions([nofee['hex']], False)
        assert_equal(res['valid'], True)

        #########################################
        # {create|verify|send}rawtransaction with missing input #
        #########################################
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1}] #won't exists
        outputs = { self.nodes[0].getnewaddress() : 4.998 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx = self.nodes[2].signrawtransaction(rawtx)

        # This transaction is missing inputs and thus should not validate
        res = self.nodes[2].verifyrawtransactions([rawtx['hex']])
        assert_equal(res['valid'], False)
        assert_equal("missingorspent" in res['reason'], True)

        # This will raise an exception since there are missing inputs
        assert_raises_jsonrpc(-25, "Missing inputs", self.nodes[2].sendrawtransaction, rawtx['hex'])

        #########################
        # RAW TX MULTISIG TESTS #
        #########################
        # 2of2 test
        addr1 = self.nodes[2].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[2].validateaddress(addr1)
        addr2Obj = self.nodes[2].validateaddress(addr2)

        mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])

        #use balance deltas instead of absolute values
        bal = self.nodes[2].getbalance()

        # send 1.2 BTC to msig adr
        txId = self.nodes[0].sendtoaddress(mSigObj, 1.2)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[2].getbalance(), bal+Decimal('1.20000000')) #node2 has both keys of the 2of2 ms addr., tx should affect the balance


        # 2of3 test from different nodes
        bal = self.nodes[2].getbalance()
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()
        addr3 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[1].validateaddress(addr1)
        addr2Obj = self.nodes[2].validateaddress(addr2)
        addr3Obj = self.nodes[2].validateaddress(addr3)

        mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey'], addr3Obj['pubkey']])

        txId = self.nodes[0].sendtoaddress(mSigObj, 2.2)
        decTx = self.nodes[0].gettransaction(txId)
        rawTx = self.nodes[0].decoderawtransaction(decTx['hex'])
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        # verifyrawtransaction should fail since output was already spent
        res = self.nodes[0].verifyrawtransactions([decTx['hex']])
        assert_equal(res['valid'], False)
        assert_equal("in chain" in res['reason'], True)

        #THIS IS A INCOMPLETE FEATURE
        #NODE2 HAS TWO OF THREE KEY AND THE FUNDS SHOULD BE SPENDABLE AND COUNT AT BALANCE CALCULATION
        assert_equal(self.nodes[2].getbalance(), bal) #for now, assume the funds of a 2of3 multisig tx are not marked as spendable

        txDetails = self.nodes[0].gettransaction(txId, True)
        rawTx = self.nodes[0].decoderawtransaction(txDetails['hex'])
        vout = False
        for outpoint in rawTx['vout']:
            if outpoint['value'] == Decimal('2.20000000'):
                vout = outpoint
                break

        bal = self.nodes[0].getbalance()
        inputs = [{ "txid" : txId, "vout" : vout['n'], "scriptPubKey" : vout['scriptPubKey']['hex']}]
        outputs = { self.nodes[0].getnewaddress() : 2.19 }
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        rawTxPartialSigned = self.nodes[1].signrawtransaction(rawTx, inputs)
        assert_equal(rawTxPartialSigned['complete'], False) #node1 only has one key, can't comp. sign the tx

        # verifyrawtransaction should fail since output is not fully signed
        res = self.nodes[0].verifyrawtransactions([rawTxPartialSigned['hex']])
        assert_equal(res['valid'], False)
        assert_equal("mandatory-script-verify" in res['reason'], True)

        rawTxSigned = self.nodes[2].signrawtransaction(rawTx, inputs)
        assert_equal(rawTxSigned['complete'], True) #node2 can sign the tx compl., own two of three keys

        # verifyrawtransaction should succeed since output is fully signed
        res = self.nodes[0].verifyrawtransactions([rawTxSigned['hex']])
        assert_equal(res['valid'], True)

        self.nodes[2].sendrawtransaction(rawTxSigned['hex'])
        rawTx = self.nodes[0].decoderawtransaction(rawTxSigned['hex'])
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), bal+Decimal('50.00000000')+Decimal('2.19000000')) #block reward + tx

        # 2of2 test for combining transactions
        bal = self.nodes[2].getbalance()
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[1].validateaddress(addr1)
        addr2Obj = self.nodes[2].validateaddress(addr2)

        self.nodes[1].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])
        mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])
        mSigObjValid = self.nodes[2].validateaddress(mSigObj)

        txId = self.nodes[0].sendtoaddress(mSigObj, 2.2)
        decTx = self.nodes[0].gettransaction(txId)
        rawTx2 = self.nodes[0].decoderawtransaction(decTx['hex'])
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        assert_equal(self.nodes[2].getbalance(), bal) # the funds of a 2of2 multisig tx should not be marked as spendable

        txDetails = self.nodes[0].gettransaction(txId, True)
        rawTx2 = self.nodes[0].decoderawtransaction(txDetails['hex'])
        vout = False
        for outpoint in rawTx2['vout']:
            if outpoint['value'] == Decimal('2.20000000'):
                vout = outpoint
                break

        bal = self.nodes[0].getbalance()
        inputs = [{ "txid" : txId, "vout" : vout['n'], "scriptPubKey" : vout['scriptPubKey']['hex'], "redeemScript" : mSigObjValid['hex']}]
        outputs = { self.nodes[0].getnewaddress() : 2.19 }
        rawTx2 = self.nodes[2].createrawtransaction(inputs, outputs)
        rawTxPartialSigned1 = self.nodes[1].signrawtransaction(rawTx2, inputs)
        self.log.info(rawTxPartialSigned1)
        assert_equal(rawTxPartialSigned['complete'], False) #node1 only has one key, can't comp. sign the tx

        rawTxPartialSigned2 = self.nodes[2].signrawtransaction(rawTx2, inputs)
        self.log.info(rawTxPartialSigned2)
        assert_equal(rawTxPartialSigned2['complete'], False) #node2 only has one key, can't comp. sign the tx
        rawTxComb = self.nodes[2].combinerawtransaction([rawTxPartialSigned1['hex'], rawTxPartialSigned2['hex']])
        self.log.info(rawTxComb)
        self.nodes[2].sendrawtransaction(rawTxComb)
        rawTx2 = self.nodes[0].decoderawtransaction(rawTxComb)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), bal+Decimal('50.00000000')+Decimal('2.19000000')) #block reward + tx

        # getrawtransaction tests
        # 1. valid parameters - only supply txid
        txHash = rawTx["hash"]
        assert_equal(self.nodes[0].getrawtransaction(txHash), rawTxSigned['hex'])

        # 2. valid parameters - supply txid and 0 for non-verbose
        assert_equal(self.nodes[0].getrawtransaction(txHash, 0), rawTxSigned['hex'])

        # 3. valid parameters - supply txid and False for non-verbose
        assert_equal(self.nodes[0].getrawtransaction(txHash, False), rawTxSigned['hex'])

        # 4. valid parameters - supply txid and 1 for verbose.
        # We only check the "hex" field of the output so we don't need to update this test every time the output format changes.
        assert_equal(self.nodes[0].getrawtransaction(txHash, 1)["hex"], rawTxSigned['hex'])

        # 5. valid parameters - supply txid and True for non-verbose
        assert_equal(self.nodes[0].getrawtransaction(txHash, True)["hex"], rawTxSigned['hex'])

        # 6. invalid parameters - supply txid and string "Flase"
        assert_raises_jsonrpc(-3,"Invalid type", self.nodes[0].getrawtransaction, txHash, "Flase")

        # 7. invalid parameters - supply txid and empty array
        assert_raises_jsonrpc(-3,"Invalid type", self.nodes[0].getrawtransaction, txHash, [])

        # 8. invalid parameters - supply txid and empty dict
        assert_raises_jsonrpc(-3,"Invalid type", self.nodes[0].getrawtransaction, txHash, {})

        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : 1000}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        rawtx   = self.nodes[0].createrawtransaction(inputs, outputs)
        decrawtx= self.nodes[0].decoderawtransaction(rawtx)
        assert_equal(decrawtx['vin'][0]['sequence'], 1000)

        # 9. invalid parameters - sequence number out of range
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : -1}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        assert_raises_jsonrpc(-8, 'Invalid parameter, sequence number is out of range', self.nodes[0].createrawtransaction, inputs, outputs)

        # 10. invalid parameters - sequence number out of range
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : 4294967296}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        assert_raises_jsonrpc(-8, 'Invalid parameter, sequence number is out of range', self.nodes[0].createrawtransaction, inputs, outputs)

        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : 4294967294}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        rawtx   = self.nodes[0].createrawtransaction(inputs, outputs)
        decrawtx= self.nodes[0].decoderawtransaction(rawtx)
        assert_equal(decrawtx['vin'][0]['sequence'], 4294967294)

if __name__ == '__main__':
    RawTransactionsTest().main()
