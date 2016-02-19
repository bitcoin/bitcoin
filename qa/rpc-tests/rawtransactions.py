#!/usr/bin/env python2
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test re-org scenarios with a mempool that contains transactions
# that spend (directly or indirectly) coinbase transactions.
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from binascii import unhexlify,hexlify

# Create one-input, one-output, no-fee transaction:
class RawTransactionsTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir)

        #connect to a local machine for debugging
        #url = "http://bitcoinrpc:DP6DvqZtqXarpeNWyN3LZTFchCCyCUuHwNF7E8pX99x1@%s:%d" % ('127.0.0.1', 18332)
        #proxy = AuthServiceProxy(url)
        #proxy.url = url # store URL on proxy for info
        #self.nodes.append(proxy)

        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)

        self.is_network_split=False
        self.sync_all()

    def run_test(self):

        #prepare some coins for multiple *rawtransaction commands
        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(101)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.5)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),5.0)
        self.sync_all()
        self.nodes[0].generate(5)
        self.sync_all()

        #########################################
        # sendrawtransaction with missing input #
        #########################################
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1}] #won't exists
        outputs = { self.nodes[0].getnewaddress() : 4.998 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx   = self.nodes[2].signrawtransaction(rawtx)

        # check missing input also using verifyrawtransactions
        err   = self.nodes[2].verifyrawtransactions([rawtx['hex']])
        assert(err is not None)
        assert(err['index'] == 0)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('bad-txns-inputs-missingorspent'))

        # check sendrawtransaction
        errorString = ""
        try:
            rawtx   = self.nodes[2].sendrawtransaction(rawtx['hex'])
        except JSONRPCException,e:
            errorString = e.error['message']

        assert("Missing inputs" in errorString)

        #################################
        # validaterawtransactions tests #
        #################################
        inputs  = []
        outputs = { self.nodes[0].getnewaddress() : 4.998 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)

        # check that unfunded transaction is invalid
        err   = self.nodes[2].verifyrawtransactions([rawtx])
        assert(err is not None)
        assert(err['index'] == 0)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('bad-txns-vin-empty'))

        rawtx   = self.nodes[2].fundrawtransaction(rawtx)
        rawtx   = rawtx['hex']

        # check that unsigned transaction is invalid
        err   = self.nodes[2].verifyrawtransactions([rawtx])
        assert(err is not None)
        assert(err['index'] == 0)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('mandatory-script-verify-flag-failed (Operation not valid with the current stack size)'))

        rawtx   = self.nodes[2].signrawtransaction(rawtx)
        rawtx   = rawtx['hex']

        # check that transaction is fully valid
        err   = self.nodes[2].verifyrawtransactions([rawtx])
        assert(err is None)

        # check that duplicate transaction causes missing/spent inputs error
        # in second transaction
        err   = self.nodes[2].verifyrawtransactions([rawtx,rawtx])
        assert(err is not None)
        assert(err['index'] == 1)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('bad-txns-inputs-missingorspent'))

        # corrupt the transaction output to invalidate the signature, will result in EVAL_FALSE
        rawtx = bytearray(unhexlify(rawtx))
        rawtx[-10] = rawtx[-10] ^ 0xff
        rawtx = hexlify(rawtx)
        err   = self.nodes[2].verifyrawtransactions([rawtx])
        assert(err['index'] == 0)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('mandatory-script-verify-flag-failed (Script evaluated without error but finished with a false/empty top stack element)'))

        # loose coinbase transaction should be rejected
        rawtx = '01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000'
        err   = self.nodes[2].verifyrawtransactions([rawtx])
        assert(err['index'] == 0)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('coinbase'))

        # passing no transactions at all is always successful, albeit boring
        err   = self.nodes[2].verifyrawtransactions([])
        assert(err is None)

        # invalid option name
        try:
            err   = self.nodes[2].verifyrawtransactions([],{'check_transaction_color':False})
            assert(false) # invalid option must cause exception
        except JSONRPCException,e:
            assert(e.error['code'] == -8) # RPC_INVALID_PARAMETER

        #########################
        # RAW TX MULTISIG TESTS #
        #########################
        # 2of2 test
        addr1 = self.nodes[2].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[2].validateaddress(addr1)
        addr2Obj = self.nodes[2].validateaddress(addr2)

        mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])
        mSigObjValid = self.nodes[2].validateaddress(mSigObj)

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
        mSigObjValid = self.nodes[2].validateaddress(mSigObj)

        txId = self.nodes[0].sendtoaddress(mSigObj, 2.2)
        decTx = self.nodes[0].gettransaction(txId)
        rawTx = self.nodes[0].decoderawtransaction(decTx['hex'])
        sPK = rawTx['vout'][0]['scriptPubKey']['hex']
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

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
        
        rawTxSigned = self.nodes[2].signrawtransaction(rawTx, inputs)
        assert_equal(rawTxSigned['complete'], True) #node2 can sign the tx compl., own two of three keys

        newTxId = self.nodes[2].sendrawtransaction(rawTxSigned['hex'])

        # after syncing up mempools, check that verifyrawtransactions against
        # mempool and against chain will now diverge when an output from the
        # previously constructed transaction is used
        self.sync_all()
        inputs = [{ "txid" : newTxId, "vout" : 0 }]
        outputs = { self.nodes[0].getnewaddress() : 2.19 }
        rawTx2 = self.nodes[0].createrawtransaction(inputs, outputs)
        rawTx2 = self.nodes[0].signrawtransaction(rawTx2)
        rawTx2 = rawTx2['hex']
        err   = self.nodes[0].verifyrawtransactions([rawTx2],{'include_mempool':True})
        assert(err is None)
        err   = self.nodes[0].verifyrawtransactions([rawTx2],{'include_mempool':False})
        assert(err is not None)
        assert(err['code'] == 16)
        assert(err['reason'].startswith('bad-txns-inputs-missingorspent'))

        # mine the transaction into a block and check end result
        rawTx = self.nodes[0].decoderawtransaction(rawTxSigned['hex'])
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), bal+Decimal('50.00000000')+Decimal('2.19000000')) #block reward + tx

if __name__ == '__main__':
    RawTransactionsTest().main()
