#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the fundrawtransaction RPC."""

from decimal import Decimal
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    connect_nodes_bi,
    count_bytes,
    find_vout_for_address,
)


def get_unspent(listunspent, amount):
    for utx in listunspent:
        if utx['amount'] == amount:
            return utx
    raise AssertionError('Could not find unspent with amount={}'.format(amount))

class RawTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self, split=False):
        self.setup_nodes()

        connect_nodes_bi(self.nodes, 0, 1)
        connect_nodes_bi(self.nodes, 1, 2)
        connect_nodes_bi(self.nodes, 0, 2)
        connect_nodes_bi(self.nodes, 0, 3)

    def run_test(self):
        min_relay_tx_fee = self.nodes[0].getnetworkinfo()['relayfee']
        # This test is not meant to test fee estimation and we'd like
        # to be sure all txs are sent at a consistent desired feerate
        for node in self.nodes:
            node.settxfee(min_relay_tx_fee)

        # if the fee's positive delta is higher than this value tests will fail,
        # neg. delta always fail the tests.
        # The size of the signature of every input may be at most 2 bytes larger
        # than a minimum sized signature.

        #            = 2 bytes * minRelayTxFeePerByte
        feeTolerance = 2 * min_relay_tx_fee/1000

        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(121)
        self.sync_all()

        # ensure that setting changePosition in fundraw with an exact match is handled properly
        rawmatch = self.nodes[2].createrawtransaction([], {self.nodes[2].getnewaddress():50})
        rawmatch = self.nodes[2].fundrawtransaction(rawmatch, {"changePosition":1, "subtractFeeFromOutputs":[0]})
        assert_equal(rawmatch["changepos"], -1)

        watchonly_address = self.nodes[0].getnewaddress()
        watchonly_pubkey = self.nodes[0].getaddressinfo(watchonly_address)["pubkey"]
        watchonly_amount = Decimal(200)
        self.nodes[3].importpubkey(watchonly_pubkey, "", True)
        watchonly_txid = self.nodes[0].sendtoaddress(watchonly_address, watchonly_amount)

        # Lock UTXO so nodes[0] doesn't accidentally spend it
        watchonly_vout = find_vout_for_address(self.nodes[0], watchonly_txid, watchonly_address)
        self.nodes[0].lockunspent(False, [{"txid": watchonly_txid, "vout": watchonly_vout}])

        self.nodes[0].sendtoaddress(self.nodes[3].getnewaddress(), watchonly_amount / 10)

        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.5)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 5.0)

        self.nodes[0].generate(1)
        self.sync_all()

        ###############
        # simple test #
        ###############
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert(len(dec_tx['vin']) > 0) #test that we have enough inputs

        ##############################
        # simple test with two coins #
        ##############################
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.2 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert(len(dec_tx['vin']) > 0) #test if we have enough inputs

        ##############################
        # simple test with two coins #
        ##############################
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.6 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert(len(dec_tx['vin']) > 0)
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')


        ################################
        # simple test with two outputs #
        ################################
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.6, self.nodes[1].getnewaddress() : 2.5 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        assert(len(dec_tx['vin']) > 0)
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')


        #########################################################################
        # test a fundrawtransaction with a VIN greater than the required amount #
        #########################################################################
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        assert_equal(fee + totalOut, utx['amount']) #compare vin total and totalout+fee


        #####################################################################
        # test a fundrawtransaction with which will not get a change output #
        #####################################################################
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : Decimal(5.0) - fee - feeTolerance }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        assert_equal(rawtxfund['changepos'], -1)
        assert_equal(fee + totalOut, utx['amount']) #compare vin total and totalout+fee


        ####################################################
        # test a fundrawtransaction with an invalid option #
        ####################################################
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(-3, "Unexpected key foo", self.nodes[2].fundrawtransaction, rawtx, {'foo':'bar'})

        # reserveChangeKey was deprecated and is now removed
        assert_raises_rpc_error(-3, "Unexpected key reserveChangeKey", lambda: self.nodes[2].fundrawtransaction(hexstring=rawtx, options={'reserveChangeKey': True}))

        ############################################################
        # test a fundrawtransaction with an invalid change address #
        ############################################################
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(-5, "changeAddress must be a valid bitcoin address", self.nodes[2].fundrawtransaction, rawtx, {'changeAddress':'foobar'})

        ############################################################
        # test a fundrawtransaction with a provided change address #
        ############################################################
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        change = self.nodes[2].getnewaddress()
        assert_raises_rpc_error(-8, "changePosition out of bounds", self.nodes[2].fundrawtransaction, rawtx, {'changeAddress':change, 'changePosition':2})
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx, {'changeAddress': change, 'changePosition': 0})
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        out = dec_tx['vout'][0]
        assert_equal(change, out['scriptPubKey']['addresses'][0])

        #########################################################
        # test a fundrawtransaction with a provided change type #
        #########################################################
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[2].fundrawtransaction, rawtx, {'change_type': None})
        assert_raises_rpc_error(-5, "Unknown change type ''", self.nodes[2].fundrawtransaction, rawtx, {'change_type': ''})
        rawtx = self.nodes[2].fundrawtransaction(rawtx, {'change_type': 'bech32'})
        dec_tx = self.nodes[2].decoderawtransaction(rawtx['hex'])
        assert_equal('witness_v0_keyhash', dec_tx['vout'][rawtx['changepos']]['scriptPubKey']['type'])

        #########################################################################
        # test a fundrawtransaction with a VIN smaller than the required amount #
        #########################################################################
        utx = get_unspent(self.nodes[2].listunspent(), 1)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)

        # 4-byte version + 1-byte vin count + 36-byte prevout then script_len
        rawtx = rawtx[:82] + "0100" + rawtx[84:]

        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])
        assert_equal("00", dec_tx['vin'][0]['scriptSig']['hex'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for i, out in enumerate(dec_tx['vout']):
            totalOut += out['value']
            if out['scriptPubKey']['addresses'][0] in outputs:
                matchingOuts+=1
            else:
                assert_equal(i, rawtxfund['changepos'])

        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])
        assert_equal("00", dec_tx['vin'][0]['scriptSig']['hex'])

        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)


        ###########################################
        # test a fundrawtransaction with two VINs #
        ###########################################
        utx = get_unspent(self.nodes[2].listunspent(), 1)
        utx2 = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']},{'txid' : utx2['txid'], 'vout' : utx2['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : 6.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if out['scriptPubKey']['addresses'][0] in outputs:
                matchingOuts+=1

        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)

        matchingIns = 0
        for vinOut in dec_tx['vin']:
            for vinIn in inputs:
                if vinIn['txid'] == vinOut['txid']:
                    matchingIns+=1

        assert_equal(matchingIns, 2) #we now must see two vins identical to vins given as params

        #########################################################
        # test a fundrawtransaction with two VINs and two vOUTs #
        #########################################################
        utx = get_unspent(self.nodes[2].listunspent(), 1)
        utx2 = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']},{'txid' : utx2['txid'], 'vout' : utx2['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : 6.0, self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if out['scriptPubKey']['addresses'][0] in outputs:
                matchingOuts+=1

        assert_equal(matchingOuts, 2)
        assert_equal(len(dec_tx['vout']), 3)

        ##############################################
        # test a fundrawtransaction with invalid vin #
        ##############################################
        inputs  = [ {'txid' : "1c7f966dab21119bac53213a2bc7532bff1fa844c124fd750a7d0b1332440bd1", 'vout' : 0} ] #invalid vin!
        outputs = { self.nodes[0].getnewaddress() : 1.0}
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[2].fundrawtransaction, rawtx)

        ############################################################
        #compare fee of a standard pubkeyhash transaction
        inputs = []
        outputs = {self.nodes[1].getnewaddress():1.1}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        #create same transaction over sendtoaddress
        txId = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1.1)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        #compare fee
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert(feeDelta >= 0 and feeDelta <= feeTolerance)
        ############################################################

        ############################################################
        #compare fee of a standard pubkeyhash transaction with multiple outputs
        inputs = []
        outputs = {self.nodes[1].getnewaddress():1.1,self.nodes[1].getnewaddress():1.2,self.nodes[1].getnewaddress():0.1,self.nodes[1].getnewaddress():1.3,self.nodes[1].getnewaddress():0.2,self.nodes[1].getnewaddress():0.3}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)
        #create same transaction over sendtoaddress
        txId = self.nodes[0].sendmany("", outputs)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        #compare fee
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert(feeDelta >= 0 and feeDelta <= feeTolerance)
        ############################################################


        ############################################################
        #compare fee of a 2of2 multisig p2sh transaction

        # create 2of2 addr
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[1].getnewaddress()

        addr1Obj = self.nodes[1].getaddressinfo(addr1)
        addr2Obj = self.nodes[1].getaddressinfo(addr2)

        mSigObj = self.nodes[1].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']

        inputs = []
        outputs = {mSigObj:1.1}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        #create same transaction over sendtoaddress
        txId = self.nodes[0].sendtoaddress(mSigObj, 1.1)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        #compare fee
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert(feeDelta >= 0 and feeDelta <= feeTolerance)
        ############################################################


        ############################################################
        #compare fee of a standard pubkeyhash transaction

        # create 4of5 addr
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[1].getnewaddress()
        addr3 = self.nodes[1].getnewaddress()
        addr4 = self.nodes[1].getnewaddress()
        addr5 = self.nodes[1].getnewaddress()

        addr1Obj = self.nodes[1].getaddressinfo(addr1)
        addr2Obj = self.nodes[1].getaddressinfo(addr2)
        addr3Obj = self.nodes[1].getaddressinfo(addr3)
        addr4Obj = self.nodes[1].getaddressinfo(addr4)
        addr5Obj = self.nodes[1].getaddressinfo(addr5)

        mSigObj = self.nodes[1].addmultisigaddress(4, [addr1Obj['pubkey'], addr2Obj['pubkey'], addr3Obj['pubkey'], addr4Obj['pubkey'], addr5Obj['pubkey']])['address']

        inputs = []
        outputs = {mSigObj:1.1}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        #create same transaction over sendtoaddress
        txId = self.nodes[0].sendtoaddress(mSigObj, 1.1)
        signedFee = self.nodes[0].getrawmempool(True)[txId]['fee']

        #compare fee
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert(feeDelta >= 0 and feeDelta <= feeTolerance)
        ############################################################


        ############################################################
        # spend a 2of2 multisig transaction over fundraw

        # create 2of2 addr
        addr1 = self.nodes[2].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[2].getaddressinfo(addr1)
        addr2Obj = self.nodes[2].getaddressinfo(addr2)

        mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']


        # send 1.2 BTC to msig addr
        txId = self.nodes[0].sendtoaddress(mSigObj, 1.2)
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()

        oldBalance = self.nodes[1].getbalance()
        inputs = []
        outputs = {self.nodes[1].getnewaddress():1.1}
        rawtx = self.nodes[2].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[2].fundrawtransaction(rawtx)

        signedTx = self.nodes[2].signrawtransactionwithwallet(fundedTx['hex'])
        txId = self.nodes[2].sendrawtransaction(signedTx['hex'])
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()

        # make sure funds are received at node1
        assert_equal(oldBalance+Decimal('1.10000000'), self.nodes[1].getbalance())

        ############################################################
        # locked wallet test
        self.nodes[1].encryptwallet("test")
        self.stop_nodes()

        self.start_nodes()
        # This test is not meant to test fee estimation and we'd like
        # to be sure all txs are sent at a consistent desired feerate
        for node in self.nodes:
            node.settxfee(min_relay_tx_fee)

        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        connect_nodes_bi(self.nodes,0,3)
        # Again lock the watchonly UTXO or nodes[0] may spend it, because
        # lockunspent is memory-only and thus lost on restart
        self.nodes[0].lockunspent(False, [{"txid": watchonly_txid, "vout": watchonly_vout}])
        self.sync_all()

        # drain the keypool
        self.nodes[1].getnewaddress()
        self.nodes[1].getrawchangeaddress()
        inputs = []
        outputs = {self.nodes[0].getnewaddress():1.1}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        # fund a transaction that requires a new key for the change output
        # creating the key must be impossible because the wallet is locked
        assert_raises_rpc_error(-4, "Keypool ran out, please call keypoolrefill first", self.nodes[1].fundrawtransaction, rawtx)

        #refill the keypool
        self.nodes[1].walletpassphrase("test", 100)
        self.nodes[1].keypoolrefill(8) #need to refill the keypool to get an internal change address
        self.nodes[1].walletlock()

        assert_raises_rpc_error(-13, "walletpassphrase", self.nodes[1].sendtoaddress, self.nodes[0].getnewaddress(), 1.2)

        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress():1.1}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)

        #now we need to unlock
        self.nodes[1].walletpassphrase("test", 600)
        signedTx = self.nodes[1].signrawtransactionwithwallet(fundedTx['hex'])
        txId = self.nodes[1].sendrawtransaction(signedTx['hex'])
        self.nodes[1].generate(1)
        self.sync_all()

        # make sure funds are received at node1
        assert_equal(oldBalance+Decimal('51.10000000'), self.nodes[0].getbalance())


        ###############################################
        # multiple (~19) inputs tx test | Compare fee #
        ###############################################

        #empty node1, send some small coins from node0 to node1
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), self.nodes[1].getbalance(), "", "", True)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        for i in range(0,20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.nodes[0].generate(1)
        self.sync_all()

        #fund a tx with ~20 small inputs
        inputs = []
        outputs = {self.nodes[0].getnewaddress():0.15,self.nodes[0].getnewaddress():0.04}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)

        #create same transaction over sendtoaddress
        txId = self.nodes[1].sendmany("", outputs)
        signedFee = self.nodes[1].getrawmempool(True)[txId]['fee']

        #compare fee
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert(feeDelta >= 0 and feeDelta <= feeTolerance*19) #~19 inputs


        #############################################
        # multiple (~19) inputs tx test | sign/send #
        #############################################

        #again, empty node1, send some small coins from node0 to node1
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), self.nodes[1].getbalance(), "", "", True)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()

        for i in range(0,20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.nodes[0].generate(1)
        self.sync_all()

        #fund a tx with ~20 small inputs
        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress():0.15,self.nodes[0].getnewaddress():0.04}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)
        fundedAndSignedTx = self.nodes[1].signrawtransactionwithwallet(fundedTx['hex'])
        txId = self.nodes[1].sendrawtransaction(fundedAndSignedTx['hex'])
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(oldBalance+Decimal('50.19000000'), self.nodes[0].getbalance()) #0.19+block reward

        #####################################################
        # test fundrawtransaction with OP_RETURN and no vin #
        #####################################################

        rawtx   = "0100000000010000000000000000066a047465737400000000"
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        assert_equal(len(dec_tx['vin']), 0)
        assert_equal(len(dec_tx['vout']), 1)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])

        assert_greater_than(len(dec_tx['vin']), 0) # at least one vin
        assert_equal(len(dec_tx['vout']), 2) # one change output added


        ##################################################
        # test a fundrawtransaction using only watchonly #
        ##################################################

        inputs = []
        outputs = {self.nodes[2].getnewaddress() : watchonly_amount / 2}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        result = self.nodes[3].fundrawtransaction(rawtx, {'includeWatching': True })
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 1)
        assert_equal(res_dec["vin"][0]["txid"], watchonly_txid)

        assert("fee" in result.keys())
        assert_greater_than(result["changepos"], -1)

        ###############################################################
        # test fundrawtransaction using the entirety of watched funds #
        ###############################################################

        inputs = []
        outputs = {self.nodes[2].getnewaddress() : watchonly_amount}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        # Backward compatibility test (2nd param is includeWatching)
        result = self.nodes[3].fundrawtransaction(rawtx, True)
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 2)
        assert(res_dec["vin"][0]["txid"] == watchonly_txid or res_dec["vin"][1]["txid"] == watchonly_txid)

        assert_greater_than(result["fee"], 0)
        assert_greater_than(result["changepos"], -1)
        assert_equal(result["fee"] + res_dec["vout"][result["changepos"]]["value"], watchonly_amount / 10)

        signedtx = self.nodes[3].signrawtransactionwithwallet(result["hex"])
        assert(not signedtx["complete"])
        signedtx = self.nodes[0].signrawtransactionwithwallet(signedtx["hex"])
        assert(signedtx["complete"])
        self.nodes[0].sendrawtransaction(signedtx["hex"])
        self.nodes[0].generate(1)
        self.sync_all()

        #######################
        # Test feeRate option #
        #######################

        # Make sure there is exactly one input so coin selection can't skew the result
        assert_equal(len(self.nodes[3].listunspent(1)), 1)

        inputs = []
        outputs = {self.nodes[3].getnewaddress() : 1}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)
        result = self.nodes[3].fundrawtransaction(rawtx) # uses min_relay_tx_fee (set by settxfee)
        result2 = self.nodes[3].fundrawtransaction(rawtx, {"feeRate": 2*min_relay_tx_fee})
        result3 = self.nodes[3].fundrawtransaction(rawtx, {"feeRate": 10*min_relay_tx_fee})
        assert_raises_rpc_error(-4, "Fee exceeds maximum configured by -maxtxfee", self.nodes[3].fundrawtransaction, rawtx, {"feeRate": 1})
        result_fee_rate = result['fee'] * 1000 / count_bytes(result['hex'])
        assert_fee_amount(result2['fee'], count_bytes(result2['hex']), 2 * result_fee_rate)
        assert_fee_amount(result3['fee'], count_bytes(result3['hex']), 10 * result_fee_rate)

        ################################
        # Test no address reuse occurs #
        ################################

        result3 = self.nodes[3].fundrawtransaction(rawtx)
        res_dec = self.nodes[0].decoderawtransaction(result3["hex"])
        changeaddress = ""
        for out in res_dec['vout']:
            if out['value'] > 1.0:
                changeaddress += out['scriptPubKey']['addresses'][0]
        assert(changeaddress != "")
        nextaddr = self.nodes[3].getnewaddress()
        # Now the change address key should be removed from the keypool
        assert(changeaddress != nextaddr)

        ######################################
        # Test subtractFeeFromOutputs option #
        ######################################

        # Make sure there is exactly one input so coin selection can't skew the result
        assert_equal(len(self.nodes[3].listunspent(1)), 1)

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): 1}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        result = [self.nodes[3].fundrawtransaction(rawtx), # uses min_relay_tx_fee (set by settxfee)
                  self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": []}), # empty subtraction list
                  self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": [0]}), # uses min_relay_tx_fee (set by settxfee)
                  self.nodes[3].fundrawtransaction(rawtx, {"feeRate": 2*min_relay_tx_fee}),
                  self.nodes[3].fundrawtransaction(rawtx, {"feeRate": 2*min_relay_tx_fee, "subtractFeeFromOutputs": [0]})]

        dec_tx = [self.nodes[3].decoderawtransaction(tx_['hex']) for tx_ in result]
        output = [d['vout'][1 - r['changepos']]['value'] for d, r in zip(dec_tx, result)]
        change = [d['vout'][r['changepos']]['value'] for d, r in zip(dec_tx, result)]

        assert_equal(result[0]['fee'], result[1]['fee'], result[2]['fee'])
        assert_equal(result[3]['fee'], result[4]['fee'])
        assert_equal(change[0], change[1])
        assert_equal(output[0], output[1])
        assert_equal(output[0], output[2] + result[2]['fee'])
        assert_equal(change[0] + result[0]['fee'], change[2])
        assert_equal(output[3], output[4] + result[4]['fee'])
        assert_equal(change[3] + result[3]['fee'], change[4])

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): value for value in (1.0, 1.1, 1.2, 1.3)}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        result = [self.nodes[3].fundrawtransaction(rawtx),
                  # split the fee between outputs 0, 2, and 3, but not output 1
                  self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": [0, 2, 3]})]

        dec_tx = [self.nodes[3].decoderawtransaction(result[0]['hex']),
                  self.nodes[3].decoderawtransaction(result[1]['hex'])]

        # Nested list of non-change output amounts for each transaction
        output = [[out['value'] for i, out in enumerate(d['vout']) if i != r['changepos']]
                  for d, r in zip(dec_tx, result)]

        # List of differences in output amounts between normal and subtractFee transactions
        share = [o0 - o1 for o0, o1 in zip(output[0], output[1])]

        # output 1 is the same in both transactions
        assert_equal(share[1], 0)

        # the other 3 outputs are smaller as a result of subtractFeeFromOutputs
        assert_greater_than(share[0], 0)
        assert_greater_than(share[2], 0)
        assert_greater_than(share[3], 0)

        # outputs 2 and 3 take the same share of the fee
        assert_equal(share[2], share[3])

        # output 0 takes at least as much share of the fee, and no more than 2 satoshis more, than outputs 2 and 3
        assert_greater_than_or_equal(share[0], share[2])
        assert_greater_than_or_equal(share[2] + Decimal(2e-8), share[0])

        # the fee is the same in both transactions
        assert_equal(result[0]['fee'], result[1]['fee'])

        # the total subtracted from the outputs is equal to the fee
        assert_equal(share[0] + share[2] + share[3], result[0]['fee'])

if __name__ == '__main__':
    RawTransactionsTest().main()
