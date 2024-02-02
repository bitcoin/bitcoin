#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the fundrawtransaction RPC."""


from decimal import Decimal
from itertools import product
from math import ceil
from test_framework.address import address_to_scriptpubkey

from test_framework.descriptors import descsum_create
from test_framework.messages import (
    COIN,
    CTransaction,
    CTxOut,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    count_bytes,
    get_fee,
)
from test_framework.wallet_util import generate_keypair, WalletUnlock

ERR_NOT_ENOUGH_PRESET_INPUTS = "The preselected coins total amount does not cover the transaction target. " \
                               "Please allow other inputs to be automatically selected or include more coins manually"

def get_unspent(listunspent, amount):
    for utx in listunspent:
        if utx['amount'] == amount:
            return utx
    raise AssertionError('Could not find unspent with amount={}'.format(amount))

class RawTransactionsTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.rpc_timeout = 90  # to prevent timeouts in `test_transaction_too_large`

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

        self.connect_nodes(0, 1)
        self.connect_nodes(1, 2)
        self.connect_nodes(0, 2)
        self.connect_nodes(0, 3)

    def lock_outputs_type(self, wallet, outputtype):
        """
        Only allow UTXOs of the given type
        """
        if outputtype in ["legacy", "p2pkh", "pkh"]:
            prefixes = ["pkh(", "sh(multi("]
        elif outputtype in ["p2sh-segwit", "sh_wpkh"]:
            prefixes = ["sh(wpkh(", "sh(wsh("]
        elif outputtype in ["bech32", "wpkh"]:
            prefixes = ["wpkh(", "wsh("]
        else:
            assert False, f"Unknown output type {outputtype}"

        to_lock = []
        for utxo in wallet.listunspent():
            if "desc" in utxo:
                for prefix in prefixes:
                    if utxo["desc"].startswith(prefix):
                        to_lock.append({"txid": utxo["txid"], "vout": utxo["vout"]})
        wallet.lockunspent(False, to_lock)

    def unlock_utxos(self, wallet):
        """
        Unlock all UTXOs except the watchonly one
        """
        to_keep = []
        if self.watchonly_utxo is not None:
            to_keep.append(self.watchonly_utxo)
        wallet.lockunspent(True)
        wallet.lockunspent(False, to_keep)

    def run_test(self):
        self.watchonly_utxo = None
        self.log.info("Connect nodes, set fees, generate blocks, and sync")
        self.min_relay_tx_fee = self.nodes[0].getnetworkinfo()['relayfee']
        # This test is not meant to test fee estimation and we'd like
        # to be sure all txs are sent at a consistent desired feerate
        for node in self.nodes:
            node.settxfee(self.min_relay_tx_fee)

        # if the fee's positive delta is higher than this value tests will fail,
        # neg. delta always fail the tests.
        # The size of the signature of every input may be at most 2 bytes larger
        # than a minimum sized signature.

        #            = 2 bytes * minRelayTxFeePerByte
        self.fee_tolerance = 2 * self.min_relay_tx_fee / 1000

        self.generate(self.nodes[2], 1)
        self.generate(self.nodes[0], 121)

        self.test_add_inputs_default_value()
        self.test_preset_inputs_selection()
        self.test_weight_calculation()
        self.test_change_position()
        self.test_simple()
        self.test_simple_two_coins()
        self.test_simple_two_outputs()
        self.test_change()
        self.test_no_change()
        self.test_invalid_option()
        self.test_invalid_change_address()
        self.test_valid_change_address()
        self.test_change_type()
        self.test_coin_selection()
        self.test_two_vin()
        self.test_two_vin_two_vout()
        self.test_invalid_input()
        self.test_fee_p2pkh()
        self.test_fee_p2pkh_multi_out()
        self.test_fee_p2sh()
        self.test_fee_4of5()
        self.test_spend_2of2()
        self.test_locked_wallet()
        self.test_many_inputs_fee()
        self.test_many_inputs_send()
        self.test_op_return()
        self.test_watchonly()
        self.test_all_watched_funds()
        self.test_option_feerate()
        self.test_address_reuse()
        self.test_option_subtract_fee_from_outputs()
        self.test_subtract_fee_with_presets()
        self.test_transaction_too_large()
        self.test_include_unsafe()
        self.test_external_inputs()
        self.test_22670()
        self.test_feerate_rounding()
        self.test_input_confs_control()
        self.test_duplicate_outputs()

    def test_duplicate_outputs(self):
        self.log.info("Test deserializing and funding a transaction with duplicate outputs")
        self.nodes[1].createwallet("fundtx_duplicate_outputs")
        w = self.nodes[1].get_wallet_rpc("fundtx_duplicate_outputs")

        addr = w.getnewaddress(address_type="bech32")
        self.nodes[0].sendtoaddress(addr, 5)
        self.generate(self.nodes[0], 1)

        address = self.nodes[0].getnewaddress("bech32")
        tx = CTransaction()
        tx.vin = []
        tx.vout = [CTxOut(1 * COIN, bytearray(address_to_scriptpubkey(address)))] * 2
        tx.nLockTime = 0
        tx_hex = tx.serialize().hex()
        res = w.fundrawtransaction(tx_hex, add_inputs=True)
        signed_res = w.signrawtransactionwithwallet(res["hex"])
        txid = w.sendrawtransaction(signed_res["hex"])
        assert self.nodes[1].getrawtransaction(txid)

        self.log.info("Test SFFO with duplicate outputs")

        res_sffo = w.fundrawtransaction(tx_hex, add_inputs=True, subtractFeeFromOutputs=[0,1])
        signed_res_sffo = w.signrawtransactionwithwallet(res_sffo["hex"])
        txid_sffo = w.sendrawtransaction(signed_res_sffo["hex"])
        assert self.nodes[1].getrawtransaction(txid_sffo)

    def test_change_position(self):
        """Ensure setting changePosition in fundraw with an exact match is handled properly."""
        self.log.info("Test fundrawtxn changePosition option")
        rawmatch = self.nodes[2].createrawtransaction([], {self.nodes[2].getnewaddress():50})
        rawmatch = self.nodes[2].fundrawtransaction(rawmatch, changePosition=1, subtractFeeFromOutputs=[0])
        assert_equal(rawmatch["changepos"], -1)

        self.nodes[3].createwallet(wallet_name="wwatch", disable_private_keys=True)
        wwatch = self.nodes[3].get_wallet_rpc('wwatch')
        watchonly_address = self.nodes[0].getnewaddress()
        watchonly_pubkey = self.nodes[0].getaddressinfo(watchonly_address)["pubkey"]
        self.watchonly_amount = Decimal(200)
        wwatch.importpubkey(watchonly_pubkey, "", True)
        self.watchonly_utxo = self.create_outpoints(self.nodes[0], outputs=[{watchonly_address: self.watchonly_amount}])[0]

        # Lock UTXO so nodes[0] doesn't accidentally spend it
        self.nodes[0].lockunspent(False, [self.watchonly_utxo])

        self.nodes[0].sendtoaddress(self.nodes[3].get_wallet_rpc(self.default_wallet_name).getnewaddress(), self.watchonly_amount / 10)

        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.5)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 5.0)

        self.generate(self.nodes[0], 1)

        wwatch.unloadwallet()

    def test_simple(self):
        self.log.info("Test fundrawtxn")
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert len(dec_tx['vin']) > 0  #test that we have enough inputs

    def test_simple_two_coins(self):
        self.log.info("Test fundrawtxn with 2 coins")
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.2 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert len(dec_tx['vin']) > 0  #test if we have enough inputs
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')

    def test_simple_two_outputs(self):
        self.log.info("Test fundrawtxn with 2 outputs")

        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.6, self.nodes[1].getnewaddress() : 2.5 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])

        assert len(dec_tx['vin']) > 0
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')

    def test_change(self):
        self.log.info("Test fundrawtxn with a vin > required amount")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        self.test_no_change_fee = fee  # Use the same fee for the next tx
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

        assert_equal(fee + totalOut, utx['amount']) #compare vin total and totalout+fee

    def test_no_change(self):
        self.log.info("Test fundrawtxn not having a change output")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = {self.nodes[0].getnewaddress(): Decimal(5.0) - self.test_no_change_fee - self.fee_tolerance}
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

    def test_invalid_option(self):
        self.log.info("Test fundrawtxn with an invalid option")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(-8, "Unknown named parameter foo", self.nodes[2].fundrawtransaction, rawtx, foo='bar')

        # reserveChangeKey was deprecated and is now removed
        assert_raises_rpc_error(-8, "Unknown named parameter reserveChangeKey", lambda: self.nodes[2].fundrawtransaction(hexstring=rawtx, reserveChangeKey=True))

    def test_invalid_change_address(self):
        self.log.info("Test fundrawtxn with an invalid change address")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(-5, "Change address must be a valid bitcoin address", self.nodes[2].fundrawtransaction, rawtx, changeAddress='foobar')

    def test_valid_change_address(self):
        self.log.info("Test fundrawtxn with a provided change address")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        change = self.nodes[2].getnewaddress()
        assert_raises_rpc_error(-8, "changePosition out of bounds", self.nodes[2].fundrawtransaction, rawtx, changeAddress=change, changePosition=2)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx, changeAddress=change, changePosition=0)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        out = dec_tx['vout'][0]
        assert_equal(change, out['scriptPubKey']['address'])

    def test_change_type(self):
        self.log.info("Test fundrawtxn with a provided change type")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-3, "JSON value of type null is not of expected type string", self.nodes[2].fundrawtransaction, rawtx, change_type=None)
        assert_raises_rpc_error(-5, "Unknown change type ''", self.nodes[2].fundrawtransaction, rawtx, change_type='')
        rawtx = self.nodes[2].fundrawtransaction(rawtx, change_type='bech32')
        dec_tx = self.nodes[2].decoderawtransaction(rawtx['hex'])
        assert_equal('witness_v0_keyhash', dec_tx['vout'][rawtx['changepos']]['scriptPubKey']['type'])

    def test_coin_selection(self):
        self.log.info("Test fundrawtxn with a vin < required amount")
        utx = get_unspent(self.nodes[2].listunspent(), 1)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)

        # 4-byte version + 1-byte vin count + 36-byte prevout then script_len
        rawtx = rawtx[:82] + "0100" + rawtx[84:]

        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])
        assert_equal("00", dec_tx['vin'][0]['scriptSig']['hex'])

        # Should fail without add_inputs:
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, self.nodes[2].fundrawtransaction, rawtx, add_inputs=False)
        # add_inputs is enabled by default
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)

        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        matchingOuts = 0
        for i, out in enumerate(dec_tx['vout']):
            if out['scriptPubKey']['address'] in outputs:
                matchingOuts+=1
            else:
                assert_equal(i, rawtxfund['changepos'])

        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])
        assert_equal("00", dec_tx['vin'][0]['scriptSig']['hex'])

        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)

    def test_two_vin(self):
        self.log.info("Test fundrawtxn with 2 vins")
        utx = get_unspent(self.nodes[2].listunspent(), 1)
        utx2 = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']},{'txid' : utx2['txid'], 'vout' : utx2['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : 6.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        # Should fail without add_inputs:
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, self.nodes[2].fundrawtransaction, rawtx, add_inputs=False)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx, add_inputs=True)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        matchingOuts = 0
        for out in dec_tx['vout']:
            if out['scriptPubKey']['address'] in outputs:
                matchingOuts+=1

        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)

        matchingIns = 0
        for vinOut in dec_tx['vin']:
            for vinIn in inputs:
                if vinIn['txid'] == vinOut['txid']:
                    matchingIns+=1

        assert_equal(matchingIns, 2) #we now must see two vins identical to vins given as params

    def test_two_vin_two_vout(self):
        self.log.info("Test fundrawtxn with 2 vins and 2 vouts")
        utx = get_unspent(self.nodes[2].listunspent(), 1)
        utx2 = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']},{'txid' : utx2['txid'], 'vout' : utx2['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : 6.0, self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        # Should fail without add_inputs:
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, self.nodes[2].fundrawtransaction, rawtx, add_inputs=False)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx, add_inputs=True)

        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        matchingOuts = 0
        for out in dec_tx['vout']:
            if out['scriptPubKey']['address'] in outputs:
                matchingOuts+=1

        assert_equal(matchingOuts, 2)
        assert_equal(len(dec_tx['vout']), 3)

    def test_invalid_input(self):
        self.log.info("Test fundrawtxn with an invalid vin")
        txid = "1c7f966dab21119bac53213a2bc7532bff1fa844c124fd750a7d0b1332440bd1"
        vout = 0
        inputs  = [ {'txid' : txid, 'vout' : vout} ] #invalid vin!
        outputs = { self.nodes[0].getnewaddress() : 1.0}
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-4, "Unable to find UTXO for external input", self.nodes[2].fundrawtransaction, rawtx)

    def test_fee_p2pkh(self):
        """Compare fee of a standard pubkeyhash transaction."""
        self.log.info("Test fundrawtxn p2pkh fee")
        self.lock_outputs_type(self.nodes[0], "p2pkh")
        inputs = []
        outputs = {self.nodes[1].getnewaddress():1.1}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1.1)
        signedFee = self.nodes[0].getmempoolentry(txId)['fees']['base']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

        self.unlock_utxos(self.nodes[0])

    def test_fee_p2pkh_multi_out(self):
        """Compare fee of a standard pubkeyhash transaction with multiple outputs."""
        self.log.info("Test fundrawtxn p2pkh fee with multiple outputs")
        self.lock_outputs_type(self.nodes[0], "p2pkh")
        inputs = []
        outputs = {
            self.nodes[1].getnewaddress():1.1,
            self.nodes[1].getnewaddress():1.2,
            self.nodes[1].getnewaddress():0.1,
            self.nodes[1].getnewaddress():1.3,
            self.nodes[1].getnewaddress():0.2,
            self.nodes[1].getnewaddress():0.3,
        }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendmany("", outputs)
        signedFee = self.nodes[0].getmempoolentry(txId)['fees']['base']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

        self.unlock_utxos(self.nodes[0])

    def test_fee_p2sh(self):
        """Compare fee of a 2-of-2 multisig p2sh transaction."""
        self.lock_outputs_type(self.nodes[0], "p2pkh")
        # Create 2-of-2 addr.
        addr1 = self.nodes[1].getnewaddress()
        addr2 = self.nodes[1].getnewaddress()

        addr1Obj = self.nodes[1].getaddressinfo(addr1)
        addr2Obj = self.nodes[1].getaddressinfo(addr2)

        mSigObj = self.nodes[3].createmultisig(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']

        inputs = []
        outputs = {mSigObj:1.1}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendtoaddress(mSigObj, 1.1)
        signedFee = self.nodes[0].getmempoolentry(txId)['fees']['base']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

        self.unlock_utxos(self.nodes[0])

    def test_fee_4of5(self):
        """Compare fee of a standard pubkeyhash transaction."""
        self.log.info("Test fundrawtxn fee with 4-of-5 addresses")
        self.lock_outputs_type(self.nodes[0], "p2pkh")

        # Create 4-of-5 addr.
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

        mSigObj = self.nodes[1].createmultisig(
            4,
            [
                addr1Obj['pubkey'],
                addr2Obj['pubkey'],
                addr3Obj['pubkey'],
                addr4Obj['pubkey'],
                addr5Obj['pubkey'],
            ]
        )['address']

        inputs = []
        outputs = {mSigObj:1.1}
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[0].fundrawtransaction(rawtx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[0].sendtoaddress(mSigObj, 1.1)
        signedFee = self.nodes[0].getmempoolentry(txId)['fees']['base']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance

        self.unlock_utxos(self.nodes[0])

    def test_spend_2of2(self):
        """Spend a 2-of-2 multisig transaction over fundraw."""
        self.log.info("Test fundpsbt spending 2-of-2 multisig")

        # Create 2-of-2 addr.
        addr1 = self.nodes[2].getnewaddress()
        addr2 = self.nodes[2].getnewaddress()

        addr1Obj = self.nodes[2].getaddressinfo(addr1)
        addr2Obj = self.nodes[2].getaddressinfo(addr2)

        self.nodes[2].createwallet(wallet_name='wmulti', disable_private_keys=True)
        wmulti = self.nodes[2].get_wallet_rpc('wmulti')
        w2 = self.nodes[2].get_wallet_rpc(self.default_wallet_name)
        mSigObj = wmulti.addmultisigaddress(
            2,
            [
                addr1Obj['pubkey'],
                addr2Obj['pubkey'],
            ]
        )['address']
        if not self.options.descriptors:
            wmulti.importaddress(mSigObj)

        # Send 1.2 BTC to msig addr.
        self.nodes[0].sendtoaddress(mSigObj, 1.2)
        self.generate(self.nodes[0], 1)

        oldBalance = self.nodes[1].getbalance()
        inputs = []
        outputs = {self.nodes[1].getnewaddress():1.1}
        funded_psbt = wmulti.walletcreatefundedpsbt(inputs=inputs, outputs=outputs, changeAddress=w2.getrawchangeaddress())['psbt']

        signed_psbt = w2.walletprocesspsbt(funded_psbt)
        self.nodes[2].sendrawtransaction(signed_psbt['hex'])
        self.generate(self.nodes[2], 1)

        # Make sure funds are received at node1.
        assert_equal(oldBalance+Decimal('1.10000000'), self.nodes[1].getbalance())

        wmulti.unloadwallet()

    def test_locked_wallet(self):
        self.log.info("Test fundrawtxn with locked wallet and hardened derivation")

        df_wallet = self.nodes[1].get_wallet_rpc(self.default_wallet_name)
        self.nodes[1].createwallet(wallet_name="locked_wallet", descriptors=self.options.descriptors)
        wallet = self.nodes[1].get_wallet_rpc("locked_wallet")
        # This test is not meant to exercise fee estimation. Making sure all txs are sent at a consistent fee rate.
        wallet.settxfee(self.min_relay_tx_fee)

        # Add some balance to the wallet (this will be reverted at the end of the test)
        df_wallet.sendall(recipients=[wallet.getnewaddress()])
        self.generate(self.nodes[1], 1)

        # Encrypt wallet and import descriptors
        wallet.encryptwallet("test")

        if self.options.descriptors:
            with WalletUnlock(wallet, "test"):
                wallet.importdescriptors([{
                    'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPdYeeZbPSKd2KYLmeVKtcFA7kqCxDvDR13MQ6us8HopUR2wLcS2ZKPhLyKsqpDL2FtL73LMHcgoCL7DXsciA8eX8nbjCR2eG/0h/*h)'),
                    'timestamp': 'now',
                    'active': True
                },
                {
                    'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPdYeeZbPSKd2KYLmeVKtcFA7kqCxDvDR13MQ6us8HopUR2wLcS2ZKPhLyKsqpDL2FtL73LMHcgoCL7DXsciA8eX8nbjCR2eG/1h/*h)'),
                    'timestamp': 'now',
                    'active': True,
                    'internal': True
                }])

        # Drain the keypool.
        wallet.getnewaddress()
        wallet.getrawchangeaddress()

        # Choose input
        inputs = wallet.listunspent()

        # Deduce exact fee to produce a changeless transaction
        tx_size = 110  # Total tx size: 110 vbytes, p2wpkh -> p2wpkh. Input 68 vbytes + rest of tx is 42 vbytes.
        value = inputs[0]["amount"] - get_fee(tx_size, self.min_relay_tx_fee)

        outputs = {self.nodes[0].getnewaddress():value}
        rawtx = wallet.createrawtransaction(inputs, outputs)
        # fund a transaction that does not require a new key for the change output
        funded_tx = wallet.fundrawtransaction(rawtx)
        assert_equal(funded_tx["changepos"], -1)

        # fund a transaction that requires a new key for the change output
        # creating the key must be impossible because the wallet is locked
        outputs = {self.nodes[0].getnewaddress():value - Decimal("0.1")}
        rawtx = wallet.createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-4, "Transaction needs a change address, but we can't generate it.", wallet.fundrawtransaction, rawtx)

        # Refill the keypool.
        with WalletUnlock(wallet, "test"):
            wallet.keypoolrefill(8) #need to refill the keypool to get an internal change address

        assert_raises_rpc_error(-13, "walletpassphrase", wallet.sendtoaddress, self.nodes[0].getnewaddress(), 1.2)

        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress():1.1}
        rawtx = wallet.createrawtransaction(inputs, outputs)
        fundedTx = wallet.fundrawtransaction(rawtx)
        assert fundedTx["changepos"] != -1

        # Now we need to unlock.
        with WalletUnlock(wallet, "test"):
            signedTx = wallet.signrawtransactionwithwallet(fundedTx['hex'])
            wallet.sendrawtransaction(signedTx['hex'])
            self.generate(self.nodes[1], 1)

            # Make sure funds are received at node1.
            assert_equal(oldBalance+Decimal('51.10000000'), self.nodes[0].getbalance())

            # Restore pre-test wallet state
            wallet.sendall(recipients=[df_wallet.getnewaddress(), df_wallet.getnewaddress(), df_wallet.getnewaddress()])
        wallet.unloadwallet()
        self.generate(self.nodes[1], 1)

    def test_many_inputs_fee(self):
        """Multiple (~19) inputs tx test | Compare fee."""
        self.log.info("Test fundrawtxn fee with many inputs")

        # Empty node1, send some small coins from node0 to node1.
        self.nodes[1].sendall(recipients=[self.nodes[0].getnewaddress()])
        self.generate(self.nodes[1], 1)

        for _ in range(20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.generate(self.nodes[0], 1)

        # Fund a tx with ~20 small inputs.
        inputs = []
        outputs = {self.nodes[0].getnewaddress():0.15,self.nodes[0].getnewaddress():0.04}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[1].sendmany("", outputs)
        signedFee = self.nodes[1].getmempoolentry(txId)['fees']['base']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance * 19  #~19 inputs

    def test_many_inputs_send(self):
        """Multiple (~19) inputs tx test | sign/send."""
        self.log.info("Test fundrawtxn sign+send with many inputs")

        # Again, empty node1, send some small coins from node0 to node1.
        self.nodes[1].sendall(recipients=[self.nodes[0].getnewaddress()])
        self.generate(self.nodes[1], 1)

        for _ in range(20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.generate(self.nodes[0], 1)

        # Fund a tx with ~20 small inputs.
        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress():0.15,self.nodes[0].getnewaddress():0.04}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)
        fundedAndSignedTx = self.nodes[1].signrawtransactionwithwallet(fundedTx['hex'])
        self.nodes[1].sendrawtransaction(fundedAndSignedTx['hex'])
        self.generate(self.nodes[1], 1)
        assert_equal(oldBalance+Decimal('50.19000000'), self.nodes[0].getbalance()) #0.19+block reward

    def test_op_return(self):
        self.log.info("Test fundrawtxn with OP_RETURN and no vin")

        rawtx   = "0100000000010000000000000000066a047465737400000000"
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        assert_equal(len(dec_tx['vin']), 0)
        assert_equal(len(dec_tx['vout']), 1)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])

        assert_greater_than(len(dec_tx['vin']), 0) # at least one vin
        assert_equal(len(dec_tx['vout']), 2) # one change output added

    def test_watchonly(self):
        self.log.info("Test fundrawtxn using only watchonly")

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): self.watchonly_amount / 2}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        self.nodes[3].loadwallet('wwatch')
        wwatch = self.nodes[3].get_wallet_rpc('wwatch')
        # Setup change addresses for the watchonly wallet
        desc_import = [{
            "desc": descsum_create("wpkh(tpubD6NzVbkrYhZ4YNXVQbNhMK1WqguFsUXceaVJKbmno2aZ3B6QfbMeraaYvnBSGpV3vxLyTTK9DYT1yoEck4XUScMzXoQ2U2oSmE2JyMedq3H/1/*)"),
            "timestamp": "now",
            "internal": True,
            "active": True,
            "keypool": True,
            "range": [0, 100],
            "watchonly": True,
        }]
        if self.options.descriptors:
            wwatch.importdescriptors(desc_import)
        else:
            wwatch.importmulti(desc_import)

        # Backward compatibility test (2nd params is includeWatching)
        result = wwatch.fundrawtransaction(rawtx, True)
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 1)
        assert_equal(res_dec["vin"][0]["txid"], self.watchonly_utxo['txid'])

        assert "fee" in result.keys()
        assert_greater_than(result["changepos"], -1)

        wwatch.unloadwallet()

    def test_all_watched_funds(self):
        self.log.info("Test fundrawtxn using entirety of watched funds")

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): self.watchonly_amount}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        self.nodes[3].loadwallet('wwatch')
        wwatch = self.nodes[3].get_wallet_rpc('wwatch')
        w3 = self.nodes[3].get_wallet_rpc(self.default_wallet_name)
        result = wwatch.fundrawtransaction(rawtx, includeWatching=True, changeAddress=w3.getrawchangeaddress(), subtractFeeFromOutputs=[0])
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 1)
        assert res_dec["vin"][0]["txid"] == self.watchonly_utxo['txid']

        assert_greater_than(result["fee"], 0)
        assert_equal(result["changepos"], -1)
        assert_equal(result["fee"] + res_dec["vout"][0]["value"], self.watchonly_amount)

        signedtx = wwatch.signrawtransactionwithwallet(result["hex"])
        assert not signedtx["complete"]
        signedtx = self.nodes[0].signrawtransactionwithwallet(signedtx["hex"])
        assert signedtx["complete"]
        self.nodes[0].sendrawtransaction(signedtx["hex"])
        self.generate(self.nodes[0], 1)

        wwatch.unloadwallet()

    def test_option_feerate(self):
        self.log.info("Test fundrawtxn with explicit fee rates (fee_rate sat/vB and feeRate BTC/kvB)")
        node = self.nodes[3]
        # Make sure there is exactly one input so coin selection can't skew the result.
        assert_equal(len(self.nodes[3].listunspent(1)), 1)
        inputs = []
        outputs = {node.getnewaddress() : 1}
        rawtx = node.createrawtransaction(inputs, outputs)

        result = node.fundrawtransaction(rawtx)  # uses self.min_relay_tx_fee (set by settxfee)
        btc_kvb_to_sat_vb = 100000  # (1e5)
        result1 = node.fundrawtransaction(rawtx, fee_rate=str(2 * btc_kvb_to_sat_vb * self.min_relay_tx_fee))
        result2 = node.fundrawtransaction(rawtx, feeRate=2 * self.min_relay_tx_fee)
        result3 = node.fundrawtransaction(rawtx, fee_rate=10 * btc_kvb_to_sat_vb * self.min_relay_tx_fee)
        result4 = node.fundrawtransaction(rawtx, feeRate=str(10 * self.min_relay_tx_fee))

        result_fee_rate = result['fee'] * 1000 / count_bytes(result['hex'])
        assert_fee_amount(result1['fee'], count_bytes(result1['hex']), 2 * result_fee_rate)
        assert_fee_amount(result2['fee'], count_bytes(result2['hex']), 2 * result_fee_rate)
        assert_fee_amount(result3['fee'], count_bytes(result3['hex']), 10 * result_fee_rate)
        assert_fee_amount(result4['fee'], count_bytes(result4['hex']), 10 * result_fee_rate)

        # Test that funding non-standard "zero-fee" transactions is valid.
        for param, zero_value in product(["fee_rate", "feeRate"], [0, 0.000, 0.00000000, "0", "0.000", "0.00000000"]):
            assert_equal(self.nodes[3].fundrawtransaction(rawtx, {param: zero_value})["fee"], 0)

        # With no arguments passed, expect fee of 141 satoshis.
        assert_approx(node.fundrawtransaction(rawtx)["fee"], vexp=0.00000141, vspan=0.00000001)
        # Expect fee to be 10,000x higher when an explicit fee rate 10,000x greater is specified.
        result = node.fundrawtransaction(rawtx, fee_rate=10000)
        assert_approx(result["fee"], vexp=0.0141, vspan=0.0001)

        self.log.info("Test fundrawtxn with invalid estimate_mode settings")
        for k, v in {"number": 42, "object": {"foo": "bar"}}.items():
            assert_raises_rpc_error(-3, f"JSON value of type {k} for field estimate_mode is not of expected type string",
                node.fundrawtransaction, rawtx, estimate_mode=v, conf_target=0.1, add_inputs=True)
        for mode in ["", "foo", Decimal("3.141592")]:
            assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"',
                node.fundrawtransaction, rawtx, estimate_mode=mode, conf_target=0.1, add_inputs=True)

        self.log.info("Test fundrawtxn with invalid conf_target settings")
        for mode in ["unset", "economical", "conservative"]:
            self.log.debug("{}".format(mode))
            for k, v in {"string": "", "object": {"foo": "bar"}}.items():
                assert_raises_rpc_error(-3, f"JSON value of type {k} for field conf_target is not of expected type number",
                    node.fundrawtransaction, rawtx, estimate_mode=mode, conf_target=v, add_inputs=True)
            for n in [-1, 0, 1009]:
                assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008",  # max value of 1008 per src/policy/fees.h
                    node.fundrawtransaction, rawtx, estimate_mode=mode, conf_target=n, add_inputs=True)

        self.log.info("Test invalid fee rate settings")
        for param, value in {("fee_rate", 100000), ("feeRate", 1.000)}:
            assert_raises_rpc_error(-4, "Fee exceeds maximum configured by user (maxtxfee)",
                node.fundrawtransaction, rawtx, add_inputs=True, **{param: value})
            assert_raises_rpc_error(-3, "Amount out of range",
                node.fundrawtransaction, rawtx, add_inputs=True, **{param: -1})
            assert_raises_rpc_error(-3, "Amount is not a number or string",
                node.fundrawtransaction, rawtx, add_inputs=True, **{param: {"foo": "bar"}})
            # Test fee rate values that don't pass fixed-point parsing checks.
            for invalid_value in ["", 0.000000001, 1e-09, 1.111111111, 1111111111111111, "31.999999999999999999999"]:
                assert_raises_rpc_error(-3, "Invalid amount", node.fundrawtransaction, rawtx, add_inputs=True, **{param: invalid_value})
        # Test fee_rate values that cannot be represented in sat/vB.
        for invalid_value in [0.0001, 0.00000001, 0.00099999, 31.99999999]:
            assert_raises_rpc_error(-3, "Invalid amount",
                node.fundrawtransaction, rawtx, fee_rate=invalid_value, add_inputs=True)

        self.log.info("Test min fee rate checks are bypassed with fundrawtxn, e.g. a fee_rate under 1 sat/vB is allowed")
        node.fundrawtransaction(rawtx, fee_rate=0.999, add_inputs=True)
        node.fundrawtransaction(rawtx, feeRate=0.00000999, add_inputs=True)

        self.log.info("- raises RPC error if both feeRate and fee_rate are passed")
        assert_raises_rpc_error(-8, "Cannot specify both fee_rate (sat/vB) and feeRate (BTC/kvB)",
            node.fundrawtransaction, rawtx, fee_rate=0.1, feeRate=0.1, add_inputs=True)

        self.log.info("- raises RPC error if both feeRate and estimate_mode passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and feeRate",
            node.fundrawtransaction, rawtx, estimate_mode="economical", feeRate=0.1, add_inputs=True)

        for param in ["feeRate", "fee_rate"]:
            self.log.info("- raises RPC error if both {} and conf_target are passed".format(param))
            assert_raises_rpc_error(-8, "Cannot specify both conf_target and {}. Please provide either a confirmation "
                "target in blocks for automatic fee estimation, or an explicit fee rate.".format(param),
                node.fundrawtransaction, rawtx, {param: 1, "conf_target": 1, "add_inputs": True})

        self.log.info("- raises RPC error if both fee_rate and estimate_mode are passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and fee_rate",
            node.fundrawtransaction, rawtx, fee_rate=1, estimate_mode="economical", add_inputs=True)

    def test_address_reuse(self):
        """Test no address reuse occurs."""
        self.log.info("Test fundrawtxn does not reuse addresses")

        rawtx = self.nodes[3].createrawtransaction(inputs=[], outputs={self.nodes[3].getnewaddress(): 1})
        result3 = self.nodes[3].fundrawtransaction(rawtx)
        res_dec = self.nodes[0].decoderawtransaction(result3["hex"])
        changeaddress = ""
        for out in res_dec['vout']:
            if out['value'] > 1.0:
                changeaddress += out['scriptPubKey']['address']
        assert changeaddress != ""
        nextaddr = self.nodes[3].getnewaddress()
        # Now the change address key should be removed from the keypool.
        assert changeaddress != nextaddr

    def test_option_subtract_fee_from_outputs(self):
        self.log.info("Test fundrawtxn subtractFeeFromOutputs option")

        # Make sure there is exactly one input so coin selection can't skew the result.
        assert_equal(len(self.nodes[3].listunspent(1)), 1)

        inputs = []
        outputs = {self.nodes[2].getnewaddress(): 1}
        rawtx = self.nodes[3].createrawtransaction(inputs, outputs)

        # Test subtract fee from outputs with feeRate (BTC/kvB)
        result = [self.nodes[3].fundrawtransaction(rawtx),  # uses self.min_relay_tx_fee (set by settxfee)
            self.nodes[3].fundrawtransaction(rawtx, subtractFeeFromOutputs=[]),  # empty subtraction list
            self.nodes[3].fundrawtransaction(rawtx, subtractFeeFromOutputs=[0]),  # uses self.min_relay_tx_fee (set by settxfee)
            self.nodes[3].fundrawtransaction(rawtx, feeRate=2 * self.min_relay_tx_fee),
            self.nodes[3].fundrawtransaction(rawtx, feeRate=2 * self.min_relay_tx_fee, subtractFeeFromOutputs=[0]),]
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

        # Test subtract fee from outputs with fee_rate (sat/vB)
        btc_kvb_to_sat_vb = 100000  # (1e5)
        result = [self.nodes[3].fundrawtransaction(rawtx),  # uses self.min_relay_tx_fee (set by settxfee)
            self.nodes[3].fundrawtransaction(rawtx, subtractFeeFromOutputs=[]),  # empty subtraction list
            self.nodes[3].fundrawtransaction(rawtx, subtractFeeFromOutputs=[0]),  # uses self.min_relay_tx_fee (set by settxfee)
            self.nodes[3].fundrawtransaction(rawtx, fee_rate=2 * btc_kvb_to_sat_vb * self.min_relay_tx_fee),
            self.nodes[3].fundrawtransaction(rawtx, fee_rate=2 * btc_kvb_to_sat_vb * self.min_relay_tx_fee, subtractFeeFromOutputs=[0]),]
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
                  # Split the fee between outputs 0, 2, and 3, but not output 1.
                  self.nodes[3].fundrawtransaction(rawtx, subtractFeeFromOutputs=[0, 2, 3])]

        dec_tx = [self.nodes[3].decoderawtransaction(result[0]['hex']),
                  self.nodes[3].decoderawtransaction(result[1]['hex'])]

        # Nested list of non-change output amounts for each transaction.
        output = [[out['value'] for i, out in enumerate(d['vout']) if i != r['changepos']]
                  for d, r in zip(dec_tx, result)]

        # List of differences in output amounts between normal and subtractFee transactions.
        share = [o0 - o1 for o0, o1 in zip(output[0], output[1])]

        # Output 1 is the same in both transactions.
        assert_equal(share[1], 0)

        # The other 3 outputs are smaller as a result of subtractFeeFromOutputs.
        assert_greater_than(share[0], 0)
        assert_greater_than(share[2], 0)
        assert_greater_than(share[3], 0)

        # Outputs 2 and 3 take the same share of the fee.
        assert_equal(share[2], share[3])

        # Output 0 takes at least as much share of the fee, and no more than 2
        # satoshis more, than outputs 2 and 3.
        assert_greater_than_or_equal(share[0], share[2])
        assert_greater_than_or_equal(share[2] + Decimal(2e-8), share[0])

        # The fee is the same in both transactions.
        assert_equal(result[0]['fee'], result[1]['fee'])

        # The total subtracted from the outputs is equal to the fee.
        assert_equal(share[0] + share[2] + share[3], result[0]['fee'])

    def test_subtract_fee_with_presets(self):
        self.log.info("Test fundrawtxn subtract fee from outputs with preset inputs that are sufficient")

        addr = self.nodes[0].getnewaddress()
        utxo = self.create_outpoints(self.nodes[0], outputs=[{addr: 10}])[0]

        rawtx = self.nodes[0].createrawtransaction([utxo], [{self.nodes[0].getnewaddress(): 5}])
        fundedtx = self.nodes[0].fundrawtransaction(rawtx, subtractFeeFromOutputs=[0])
        signedtx = self.nodes[0].signrawtransactionwithwallet(fundedtx['hex'])
        self.nodes[0].sendrawtransaction(signedtx['hex'])

    def test_transaction_too_large(self):
        self.log.info("Test fundrawtx where BnB solution would result in a too large transaction, but Knapsack would not")
        self.nodes[0].createwallet("large")
        wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        recipient = self.nodes[0].get_wallet_rpc("large")
        outputs = {}
        rawtx = recipient.createrawtransaction([], {wallet.getnewaddress(): 147.99899260})

        # Make 1500 0.1 BTC outputs. The amount that we target for funding is in
        # the BnB range when these outputs are used.  However if these outputs
        # are selected, the transaction will end up being too large, so it
        # shouldn't use BnB and instead fall back to Knapsack but that behavior
        # is not implemented yet. For now we just check that we get an error.
        # First, force the wallet to bulk-generate the addresses we'll need.
        recipient.keypoolrefill(1500)
        for _ in range(1500):
            outputs[recipient.getnewaddress()] = 0.1
        wallet.sendmany("", outputs)
        self.generate(self.nodes[0], 10)
        assert_raises_rpc_error(-4, "The inputs size exceeds the maximum weight. "
                                    "Please try sending a smaller amount or manually consolidating your wallet's UTXOs",
                                recipient.fundrawtransaction, rawtx)
        self.nodes[0].unloadwallet("large")

    def test_external_inputs(self):
        self.log.info("Test funding with external inputs")
        privkey, _ = generate_keypair(wif=True)
        self.nodes[2].createwallet("extfund")
        wallet = self.nodes[2].get_wallet_rpc("extfund")

        # Make a weird but signable script. sh(pkh()) descriptor accomplishes this
        desc = descsum_create("sh(pkh({}))".format(privkey))
        if self.options.descriptors:
            res = self.nodes[0].importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = self.nodes[0].importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        addr = self.nodes[0].deriveaddresses(desc)[0]
        addr_info = self.nodes[0].getaddressinfo(addr)

        self.nodes[0].sendtoaddress(addr, 10)
        self.nodes[0].sendtoaddress(wallet.getnewaddress(), 10)
        self.generate(self.nodes[0], 6)
        ext_utxo = self.nodes[0].listunspent(addresses=[addr])[0]

        # An external input without solving data should result in an error
        raw_tx = wallet.createrawtransaction([ext_utxo], {self.nodes[0].getnewaddress(): ext_utxo["amount"] / 2})
        assert_raises_rpc_error(-4, "Not solvable pre-selected input COutPoint(%s, %s)" % (ext_utxo["txid"][0:10], ext_utxo["vout"]), wallet.fundrawtransaction, raw_tx)

        # Error conditions
        assert_raises_rpc_error(-5, 'Pubkey "not a pubkey" must be a hex string', wallet.fundrawtransaction, raw_tx, solving_data={"pubkeys":["not a pubkey"]})
        assert_raises_rpc_error(-5, 'Pubkey "01234567890a0b0c0d0e0f" must have a length of either 33 or 65 bytes', wallet.fundrawtransaction, raw_tx, solving_data={"pubkeys":["01234567890a0b0c0d0e0f"]})
        assert_raises_rpc_error(-5, "'not a script' is not hex", wallet.fundrawtransaction, raw_tx, solving_data={"scripts":["not a script"]})
        assert_raises_rpc_error(-8, "Unable to parse descriptor 'not a descriptor'", wallet.fundrawtransaction, raw_tx, solving_data={"descriptors":["not a descriptor"]})
        assert_raises_rpc_error(-8, "Invalid parameter, missing vout key", wallet.fundrawtransaction, raw_tx, input_weights=[{"txid": ext_utxo["txid"]}])
        assert_raises_rpc_error(-8, "Invalid parameter, vout cannot be negative", wallet.fundrawtransaction, raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": -1}])
        assert_raises_rpc_error(-8, "Invalid parameter, missing weight key", wallet.fundrawtransaction, raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"]}])
        assert_raises_rpc_error(-8, "Invalid parameter, weight cannot be less than 165", wallet.fundrawtransaction, raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 164}])
        assert_raises_rpc_error(-8, "Invalid parameter, weight cannot be less than 165", wallet.fundrawtransaction, raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": -1}])
        assert_raises_rpc_error(-8, "Invalid parameter, weight cannot be greater than", wallet.fundrawtransaction, raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 400001}])

        # But funding should work when the solving data is provided
        funded_tx = wallet.fundrawtransaction(raw_tx, solving_data={"pubkeys": [addr_info['pubkey']], "scripts": [addr_info["embedded"]["scriptPubKey"]]})
        signed_tx = wallet.signrawtransactionwithwallet(funded_tx['hex'])
        assert not signed_tx['complete']
        signed_tx = self.nodes[0].signrawtransactionwithwallet(signed_tx['hex'])
        assert signed_tx['complete']

        funded_tx = wallet.fundrawtransaction(raw_tx, solving_data={"descriptors": [desc]})
        signed_tx1 = wallet.signrawtransactionwithwallet(funded_tx['hex'])
        assert not signed_tx1['complete']
        signed_tx2 = self.nodes[0].signrawtransactionwithwallet(signed_tx1['hex'])
        assert signed_tx2['complete']

        unsigned_weight = self.nodes[0].decoderawtransaction(signed_tx1["hex"])["weight"]
        signed_weight = self.nodes[0].decoderawtransaction(signed_tx2["hex"])["weight"]
        # Input's weight is difference between weight of signed and unsigned,
        # and the weight of stuff that didn't change (prevout, sequence, 1 byte of scriptSig)
        input_weight = signed_weight - unsigned_weight + (41 * 4)
        low_input_weight = input_weight // 2
        high_input_weight = input_weight * 2

        # Funding should also work if the input weight is provided
        funded_tx = wallet.fundrawtransaction(raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": input_weight}], fee_rate=2)
        signed_tx = wallet.signrawtransactionwithwallet(funded_tx["hex"])
        signed_tx = self.nodes[0].signrawtransactionwithwallet(signed_tx["hex"])
        assert_equal(self.nodes[0].testmempoolaccept([signed_tx["hex"]])[0]["allowed"], True)
        assert_equal(signed_tx["complete"], True)
        # Reducing the weight should have a lower fee
        funded_tx2 = wallet.fundrawtransaction(raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": low_input_weight}], fee_rate=2)
        assert_greater_than(funded_tx["fee"], funded_tx2["fee"])
        # Increasing the weight should have a higher fee
        funded_tx2 = wallet.fundrawtransaction(raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}], fee_rate=2)
        assert_greater_than(funded_tx2["fee"], funded_tx["fee"])
        # The provided weight should override the calculated weight when solving data is provided
        funded_tx3 = wallet.fundrawtransaction(raw_tx, solving_data={"descriptors": [desc]}, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}], fee_rate=2)
        assert_equal(funded_tx2["fee"], funded_tx3["fee"])
        # The feerate should be met
        funded_tx4 = wallet.fundrawtransaction(raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}], fee_rate=10)
        input_add_weight = high_input_weight - (41 * 4)
        tx4_weight = wallet.decoderawtransaction(funded_tx4["hex"])["weight"] + input_add_weight
        tx4_vsize = int(ceil(tx4_weight / 4))
        assert_fee_amount(funded_tx4["fee"], tx4_vsize, Decimal(0.0001))

        # Funding with weight at csuint boundaries should not cause problems
        funded_tx = wallet.fundrawtransaction(raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 255}], fee_rate=2)
        funded_tx = wallet.fundrawtransaction(raw_tx, input_weights=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 65539}], fee_rate=2)

        self.nodes[2].unloadwallet("extfund")

    def test_add_inputs_default_value(self):
        self.log.info("Test 'add_inputs' default value")

        # Create and fund the wallet with 5 BTC
        self.nodes[2].createwallet("test_preset_inputs")
        wallet = self.nodes[2].get_wallet_rpc("test_preset_inputs")
        addr1 = wallet.getnewaddress(address_type="bech32")
        self.nodes[0].sendtoaddress(addr1, 5)
        self.generate(self.nodes[0], 1)

        # Covered cases:
        # 1. Default add_inputs value with no preset inputs (add_inputs=true):
        #       Expect: automatically add coins from the wallet to the tx.
        # 2. Default add_inputs value with preset inputs (add_inputs=false):
        #       Expect: disallow automatic coin selection.
        # 3. Explicit add_inputs=true and preset inputs (with preset inputs not-covering the target amount).
        #       Expect: include inputs from the wallet.
        # 4. Explicit add_inputs=true and preset inputs (with preset inputs covering the target amount).
        #       Expect: only preset inputs are used.
        # 5. Explicit add_inputs=true, no preset inputs (same as (1) but with an explicit set):
        #       Expect: include inputs from the wallet.
        # 6. Explicit add_inputs=false, no preset inputs:
        #       Expect: failure as we did not provide inputs and the process cannot automatically select coins.

        # Case (1), 'send' command
        # 'add_inputs' value is true unless "inputs" are specified, in such case, add_inputs=false.
        # So, the wallet will automatically select coins and create the transaction if only the outputs are provided.
        tx = wallet.send(outputs=[{addr1: 3}])
        assert tx["complete"]

        # Case (2), 'send' command
        # Select an input manually, which doesn't cover the entire output amount and
        # verify that the dynamically set 'add_inputs=false' value works.

        # Fund wallet with 2 outputs, 5 BTC each.
        addr2 = wallet.getnewaddress(address_type="bech32")
        source_tx = self.nodes[0].send(outputs=[{addr1: 5}, {addr2: 5}], change_position=0)
        self.generate(self.nodes[0], 1)

        # Select only one input.
        options = {
            "inputs": [
                {
                    "txid": source_tx["txid"],
                    "vout": 1  # change position was hardcoded to index 0
                }
            ]
        }
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, wallet.send, outputs=[{addr1: 8}], **options)

        # Case (3), Explicit add_inputs=true and preset inputs (with preset inputs not-covering the target amount)
        options["add_inputs"] = True
        options["add_to_wallet"] = False
        tx = wallet.send(outputs=[{addr1: 8}], **options)
        assert tx["complete"]

        # Case (4), Explicit add_inputs=true and preset inputs (with preset inputs covering the target amount)
        options["inputs"].append({
            "txid": source_tx["txid"],
            "vout": 2  # change position was hardcoded to index 0
        })
        tx = wallet.send(outputs=[{addr1: 8}], **options)
        assert tx["complete"]
        # Check that only the preset inputs were added to the tx
        decoded_psbt_inputs = self.nodes[0].decodepsbt(tx["psbt"])['tx']['vin']
        assert_equal(len(decoded_psbt_inputs), 2)
        for input in decoded_psbt_inputs:
            assert_equal(input["txid"], source_tx["txid"])

        # Case (5), assert that inputs are added to the tx by explicitly setting add_inputs=true
        options = {"add_inputs": True, "add_to_wallet": True}
        tx = wallet.send(outputs=[{addr1: 8}], **options)
        assert tx["complete"]

        # 6. Explicit add_inputs=false, no preset inputs:
        options = {"add_inputs": False}
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, wallet.send, outputs=[{addr1: 3}], **options)

        ################################################

        # Case (1), 'walletcreatefundedpsbt' command
        # Default add_inputs value with no preset inputs (add_inputs=true)
        inputs = []
        outputs = {self.nodes[1].getnewaddress(): 8}
        assert "psbt" in wallet.walletcreatefundedpsbt(inputs=inputs, outputs=outputs)

        # Case (2), 'walletcreatefundedpsbt' command
        # Default add_inputs value with preset inputs (add_inputs=false).
        inputs = [{
            "txid": source_tx["txid"],
            "vout": 1  # change position was hardcoded to index 0
        }]
        outputs = {self.nodes[1].getnewaddress(): 8}
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, wallet.walletcreatefundedpsbt, inputs=inputs, outputs=outputs)

        # Case (3), Explicit add_inputs=true and preset inputs (with preset inputs not-covering the target amount)
        options["add_inputs"] = True
        assert "psbt" in wallet.walletcreatefundedpsbt(outputs=[{addr1: 8}], inputs=inputs, **options)

        # Case (4), Explicit add_inputs=true and preset inputs (with preset inputs covering the target amount)
        inputs.append({
            "txid": source_tx["txid"],
            "vout": 2  # change position was hardcoded to index 0
        })
        psbt_tx = wallet.walletcreatefundedpsbt(outputs=[{addr1: 8}], inputs=inputs, **options)
        # Check that only the preset inputs were added to the tx
        decoded_psbt_inputs = self.nodes[0].decodepsbt(psbt_tx["psbt"])['tx']['vin']
        assert_equal(len(decoded_psbt_inputs), 2)
        for input in decoded_psbt_inputs:
            assert_equal(input["txid"], source_tx["txid"])

        # Case (5), 'walletcreatefundedpsbt' command
        # Explicit add_inputs=true, no preset inputs
        options = {
            "add_inputs": True
        }
        assert "psbt" in wallet.walletcreatefundedpsbt(inputs=[], outputs=outputs, **options)

        # Case (6). Explicit add_inputs=false, no preset inputs:
        options = {"add_inputs": False}
        assert_raises_rpc_error(-4, ERR_NOT_ENOUGH_PRESET_INPUTS, wallet.walletcreatefundedpsbt, inputs=[], outputs=outputs, **options)

        self.nodes[2].unloadwallet("test_preset_inputs")

    def test_preset_inputs_selection(self):
        self.log.info('Test wallet preset inputs are not double-counted or reused in coin selection')

        # Create and fund the wallet with 4 UTXO of 5 BTC each (20 BTC total)
        self.nodes[2].createwallet("test_preset_inputs_selection")
        wallet = self.nodes[2].get_wallet_rpc("test_preset_inputs_selection")
        outputs = {}
        for _ in range(4):
            outputs[wallet.getnewaddress(address_type="bech32")] = 5
        self.nodes[0].sendmany("", outputs)
        self.generate(self.nodes[0], 1)

        # Select the preset inputs
        coins = wallet.listunspent()
        preset_inputs = [coins[0], coins[1], coins[2]]

        # Now let's create the tx creation options
        options = {
            "inputs": preset_inputs,
            "add_inputs": True,  # automatically add coins from the wallet to fulfill the target
            "subtract_fee_from_outputs": [0],  # deduct fee from first output
            "add_to_wallet": False
        }

        # Attempt to send 29 BTC from a wallet that only has 20 BTC. The wallet should exclude
        # the preset inputs from the pool of available coins, realize that there is not enough
        # money to fund the 29 BTC payment, and fail with "Insufficient funds".
        #
        # Even with SFFO, the wallet can only afford to send 20 BTC.
        # If the wallet does not properly exclude preset inputs from the pool of available coins
        # prior to coin selection, it may create a transaction that does not fund the full payment
        # amount or, through SFFO, incorrectly reduce the recipient's amount by the difference
        # between the original target and the wrongly counted inputs (in this case 9 BTC)
        # so that the recipient's amount is no longer equal to the user's selected target of 29 BTC.

        # First case, use 'subtract_fee_from_outputs = true'
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.send, outputs=[{wallet.getnewaddress(address_type="bech32"): 29}], options=options)

        # Second case, don't use 'subtract_fee_from_outputs'
        del options["subtract_fee_from_outputs"]
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.send, outputs=[{wallet.getnewaddress(address_type="bech32"): 29}], options=options)

        self.nodes[2].unloadwallet("test_preset_inputs_selection")

    def test_weight_calculation(self):
        self.log.info("Test weight calculation with external inputs")

        self.nodes[2].createwallet("test_weight_calculation")
        wallet = self.nodes[2].get_wallet_rpc("test_weight_calculation")

        addr = wallet.getnewaddress(address_type="bech32")
        ext_addr = self.nodes[0].getnewaddress(address_type="bech32")
        utxo, ext_utxo = self.create_outpoints(self.nodes[0], outputs=[{addr: 5}, {ext_addr: 5}])

        self.nodes[0].sendtoaddress(wallet.getnewaddress(address_type="bech32"), 5)
        self.generate(self.nodes[0], 1)

        rawtx = wallet.createrawtransaction([utxo], [{self.nodes[0].getnewaddress(address_type="bech32"): 8}])
        fundedtx = wallet.fundrawtransaction(rawtx, fee_rate=10, change_type="bech32")
        # with 71-byte signatures we should expect following tx size
        # tx overhead (10) + 2 inputs (41 each) + 2 p2wpkh (31 each) + (segwit marker and flag (2) + 2 p2wpkh 71 byte sig witnesses (107 each)) / witness scaling factor (4)
        tx_size = ceil(10 + 41*2 + 31*2 + (2 + 107*2)/4)
        assert_equal(fundedtx['fee'] * COIN, tx_size * 10)

        # Using the other output should have 72 byte sigs
        rawtx = wallet.createrawtransaction([ext_utxo], [{self.nodes[0].getnewaddress(): 13}])
        ext_desc = self.nodes[0].getaddressinfo(ext_addr)["desc"]
        fundedtx = wallet.fundrawtransaction(rawtx, fee_rate=10, change_type="bech32", solving_data={"descriptors": [ext_desc]})
        # tx overhead (10) + 3 inputs (41 each) + 2 p2wpkh(31 each) + (segwit marker and flag (2) + 2 p2wpkh 71 bytes sig witnesses (107 each) + p2wpkh 72 byte sig witness (108)) / witness scaling factor (4)
        tx_size = ceil(10 + 41*3 + 31*2 + (2 + 107*2 + 108)/4)
        assert_equal(fundedtx['fee'] * COIN, tx_size * 10)

        self.nodes[2].unloadwallet("test_weight_calculation")

    def test_include_unsafe(self):
        self.log.info("Test fundrawtxn with unsafe inputs")

        self.nodes[0].createwallet("unsafe")
        wallet = self.nodes[0].get_wallet_rpc("unsafe")

        # We receive unconfirmed funds from external keys (unsafe outputs).
        addr = wallet.getnewaddress()
        inputs = []
        for i in range(0, 2):
            utxo = self.create_outpoints(self.nodes[2], outputs=[{addr: 5}])[0]
            inputs.append((utxo['txid'], utxo['vout']))
        self.sync_mempools()

        # Unsafe inputs are ignored by default.
        rawtx = wallet.createrawtransaction([], [{self.nodes[2].getnewaddress(): 7.5}])
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.fundrawtransaction, rawtx)

        # But we can opt-in to use them for funding.
        fundedtx = wallet.fundrawtransaction(rawtx, include_unsafe=True)
        tx_dec = wallet.decoderawtransaction(fundedtx['hex'])
        assert all((txin["txid"], txin["vout"]) in inputs for txin in tx_dec["vin"])
        signedtx = wallet.signrawtransactionwithwallet(fundedtx['hex'])
        assert wallet.testmempoolaccept([signedtx['hex']])[0]["allowed"]

        # And we can also use them once they're confirmed.
        self.generate(self.nodes[0], 1)
        fundedtx = wallet.fundrawtransaction(rawtx, include_unsafe=False)
        tx_dec = wallet.decoderawtransaction(fundedtx['hex'])
        assert all((txin["txid"], txin["vout"]) in inputs for txin in tx_dec["vin"])
        signedtx = wallet.signrawtransactionwithwallet(fundedtx['hex'])
        assert wallet.testmempoolaccept([signedtx['hex']])[0]["allowed"]
        self.nodes[0].unloadwallet("unsafe")

    def test_22670(self):
        # In issue #22670, it was observed that ApproximateBestSubset may
        # choose enough value to cover the target amount but not enough to cover the transaction fees.
        # This leads to a transaction whose actual transaction feerate is lower than expected.
        # However at normal feerates, the difference between the effective value and the real value
        # that this bug is not detected because the transaction fee must be at least 0.01 BTC (the minimum change value).
        # Otherwise the targeted minimum change value will be enough to cover the transaction fees that were not
        # being accounted for. So the minimum relay fee is set to 0.1 BTC/kvB in this test.
        self.log.info("Test issue 22670 ApproximateBestSubset bug")
        # Make sure the default wallet will not be loaded when restarted with a high minrelaytxfee
        self.nodes[0].unloadwallet(self.default_wallet_name, False)
        feerate = Decimal("0.1")
        self.restart_node(0, [f"-minrelaytxfee={feerate}", "-discardfee=0"]) # Set high minrelayfee, set discardfee to 0 for easier calculation

        self.nodes[0].loadwallet(self.default_wallet_name, True)
        funds = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet(wallet_name="tester")
        tester = self.nodes[0].get_wallet_rpc("tester")

        # Because this test is specifically for ApproximateBestSubset, the target value must be greater
        # than any single input available, and require more than 1 input. So we make 3 outputs
        for i in range(0, 3):
            funds.sendtoaddress(tester.getnewaddress(address_type="bech32"), 1)
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)

        # Create transactions in order to calculate fees for the target bounds that can trigger this bug
        change_tx = tester.fundrawtransaction(tester.createrawtransaction([], [{funds.getnewaddress(): 1.5}]))
        tx = tester.createrawtransaction([], [{funds.getnewaddress(): 2}])
        no_change_tx = tester.fundrawtransaction(tx, subtractFeeFromOutputs=[0])

        overhead_fees = feerate * len(tx) / 2 / 1000
        cost_of_change = change_tx["fee"] - no_change_tx["fee"]
        fees = no_change_tx["fee"]
        assert_greater_than(fees, 0.01)

        def do_fund_send(target):
            create_tx = tester.createrawtransaction([], [{funds.getnewaddress(): target}])
            funded_tx = tester.fundrawtransaction(create_tx)
            signed_tx = tester.signrawtransactionwithwallet(funded_tx["hex"])
            assert signed_tx["complete"]
            decoded_tx = tester.decoderawtransaction(signed_tx["hex"])
            assert_equal(len(decoded_tx["vin"]), 3)
            assert tester.testmempoolaccept([signed_tx["hex"]])[0]["allowed"]

        # We want to choose more value than is available in 2 inputs when considering the fee,
        # but not enough to need 3 inputs when not considering the fee.
        # So the target value must be at least 2.00000001 - fee.
        lower_bound = Decimal("2.00000001") - fees
        # The target value must be at most 2 - cost_of_change - not_input_fees - min_change (these are all
        # included in the target before ApproximateBestSubset).
        upper_bound = Decimal("2.0") - cost_of_change - overhead_fees - Decimal("0.01")
        assert_greater_than_or_equal(upper_bound, lower_bound)
        do_fund_send(lower_bound)
        do_fund_send(upper_bound)

        self.restart_node(0)
        self.connect_nodes(0, 1)
        self.connect_nodes(0, 2)
        self.connect_nodes(0, 3)

    def test_feerate_rounding(self):
        self.log.info("Test that rounding of GetFee does not result in an assertion")

        self.nodes[1].createwallet("roundtest")
        w = self.nodes[1].get_wallet_rpc("roundtest")

        addr = w.getnewaddress(address_type="bech32")
        self.nodes[0].sendtoaddress(addr, 1)
        self.generate(self.nodes[0], 1)

        # A P2WPKH input costs 68 vbytes; With a single P2WPKH output, the rest of the tx is 42 vbytes for a total of 110 vbytes.
        # At a feerate of 1.85 sat/vb, the input will need a fee of 125.8 sats and the rest 77.7 sats
        # The entire tx fee should be 203.5 sats.
        # Coin selection rounds the fee individually instead of at the end (due to how CFeeRate::GetFee works).
        # If rounding down (which is the incorrect behavior), then the calculated fee will be 125 + 77 = 202.
        # If rounding up, then the calculated fee will be 126 + 78 = 204.
        # In the former case, the calculated needed fee is higher than the actual fee being paid, so an assertion is reached
        # To test this does not happen, we subtract 202 sats from the input value. If working correctly, this should
        # fail with insufficient funds rather than bitcoind asserting.
        rawtx = w.createrawtransaction(inputs=[], outputs=[{self.nodes[0].getnewaddress(address_type="bech32"): 1 - 0.00000202}])
        assert_raises_rpc_error(-4, "Insufficient funds", w.fundrawtransaction, rawtx, fee_rate=1.85)

    def test_input_confs_control(self):
        self.nodes[0].createwallet("minconf")
        wallet = self.nodes[0].get_wallet_rpc("minconf")

        # Fund the wallet with different chain heights
        for _ in range(2):
            self.nodes[2].sendmany("", {wallet.getnewaddress():1, wallet.getnewaddress():1})
            self.generate(self.nodes[2], 1)

        unconfirmed_txid = wallet.sendtoaddress(wallet.getnewaddress(), 0.5)

        self.log.info("Crafting TX using an unconfirmed input")
        target_address = self.nodes[2].getnewaddress()
        raw_tx1 = wallet.createrawtransaction([], {target_address: 0.1}, 0, True)
        funded_tx1 = wallet.fundrawtransaction(raw_tx1, {'fee_rate': 1, 'maxconf': 0})['hex']

        # Make sure we only had the one input
        tx1_inputs = self.nodes[0].decoderawtransaction(funded_tx1)['vin']
        assert_equal(len(tx1_inputs), 1)

        utxo1 = tx1_inputs[0]
        assert unconfirmed_txid == utxo1['txid']

        final_tx1 = wallet.signrawtransactionwithwallet(funded_tx1)['hex']
        txid1 = self.nodes[0].sendrawtransaction(final_tx1)

        mempool = self.nodes[0].getrawmempool()
        assert txid1 in mempool

        self.log.info("Fail to craft a new TX with minconf above highest one")
        # Create a replacement tx to 'final_tx1' that has 1 BTC target instead of 0.1.
        raw_tx2 = wallet.createrawtransaction([{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1})
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.fundrawtransaction, raw_tx2, {'add_inputs': True, 'minconf': 3, 'fee_rate': 10})

        self.log.info("Fail to broadcast a new TX with maxconf 0 due to BIP125 rules to verify it actually chose unconfirmed outputs")
        # Now fund 'raw_tx2' to fulfill the total target (1 BTC) by using all the wallet unconfirmed outputs.
        # As it was created with the first unconfirmed output, 'raw_tx2' only has 0.1 BTC covered (need to fund 0.9 BTC more).
        # So, the selection process, to cover the amount, will pick up the 'final_tx1' output as well, which is an output of the tx that this
        # new tx is replacing!. So, once we send it to the mempool, it will return a "bad-txns-spends-conflicting-tx"
        # because the input will no longer exist once the first tx gets replaced by this new one).
        funded_invalid = wallet.fundrawtransaction(raw_tx2, {'add_inputs': True, 'maxconf': 0, 'fee_rate': 10})['hex']
        final_invalid = wallet.signrawtransactionwithwallet(funded_invalid)['hex']
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, final_invalid)

        self.log.info("Craft a replacement adding inputs with highest depth possible")
        funded_tx2 = wallet.fundrawtransaction(raw_tx2, {'add_inputs': True, 'minconf': 2, 'fee_rate': 10})['hex']
        tx2_inputs = self.nodes[0].decoderawtransaction(funded_tx2)['vin']
        assert_greater_than_or_equal(len(tx2_inputs), 2)
        for vin in tx2_inputs:
            if vin['txid'] != unconfirmed_txid:
                assert_greater_than_or_equal(self.nodes[0].gettxout(vin['txid'], vin['vout'])['confirmations'], 2)

        final_tx2 = wallet.signrawtransactionwithwallet(funded_tx2)['hex']
        txid2 = self.nodes[0].sendrawtransaction(final_tx2)

        mempool = self.nodes[0].getrawmempool()
        assert txid1 not in mempool
        assert txid2 in mempool

        wallet.unloadwallet()

if __name__ == '__main__':
    RawTransactionsTest().main()
