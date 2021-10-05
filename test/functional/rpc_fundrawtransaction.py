#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the fundrawtransaction RPC."""

from decimal import Decimal
from itertools import product

from test_framework.descriptors import descsum_create
from test_framework.key import ECKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    count_bytes,
    find_vout_for_address,
)
from test_framework.wallet_util import bytes_to_wif


def get_unspent(listunspent, amount):
    for utx in listunspent:
        if utx['amount'] == amount:
            return utx
    raise AssertionError('Could not find unspent with amount={}'.format(amount))

class RawTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.setup_clean_chain = True
        # This test isn't testing tx relay. Set whitelist on the peers for
        # instant tx relay.
        self.extra_args = [['-whitelist=noban@127.0.0.1']] * self.num_nodes
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
        if self.watchonly_txid is not None and self.watchonly_vout is not None:
            to_keep.append({"txid": self.watchonly_txid, "vout": self.watchonly_vout})
        wallet.lockunspent(True)
        wallet.lockunspent(False, to_keep)

    def run_test(self):
        self.watchonly_txid = None
        self.watchonly_vout = None
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
        self.sync_all()
        self.generate(self.nodes[0], 121)
        self.sync_all()

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

    def test_change_position(self):
        """Ensure setting changePosition in fundraw with an exact match is handled properly."""
        self.log.info("Test fundrawtxn changePosition option")
        rawmatch = self.nodes[2].createrawtransaction([], {self.nodes[2].getnewaddress():50})
        rawmatch = self.nodes[2].fundrawtransaction(rawmatch, {"changePosition":1, "subtractFeeFromOutputs":[0]})
        assert_equal(rawmatch["changepos"], -1)

        self.nodes[3].createwallet(wallet_name="wwatch", disable_private_keys=True)
        wwatch = self.nodes[3].get_wallet_rpc('wwatch')
        watchonly_address = self.nodes[0].getnewaddress()
        watchonly_pubkey = self.nodes[0].getaddressinfo(watchonly_address)["pubkey"]
        self.watchonly_amount = Decimal(200)
        wwatch.importpubkey(watchonly_pubkey, "", True)
        self.watchonly_txid = self.nodes[0].sendtoaddress(watchonly_address, self.watchonly_amount)

        # Lock UTXO so nodes[0] doesn't accidentally spend it
        self.watchonly_vout = find_vout_for_address(self.nodes[0], self.watchonly_txid, watchonly_address)
        self.nodes[0].lockunspent(False, [{"txid": self.watchonly_txid, "vout": self.watchonly_vout}])

        self.nodes[0].sendtoaddress(self.nodes[3].get_wallet_rpc(self.default_wallet_name).getnewaddress(), self.watchonly_amount / 10)

        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.5)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 5.0)

        self.generate(self.nodes[0], 1)
        self.sync_all()

        wwatch.unloadwallet()

    def test_simple(self):
        self.log.info("Test fundrawtxn")
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert len(dec_tx['vin']) > 0  #test that we have enough inputs

    def test_simple_two_coins(self):
        self.log.info("Test fundrawtxn with 2 coins")
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.2 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        assert len(dec_tx['vin']) > 0  #test if we have enough inputs
        assert_equal(dec_tx['vin'][0]['scriptSig']['hex'], '')

    def test_simple_two_outputs(self):
        self.log.info("Test fundrawtxn with 2 outputs")

        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.6, self.nodes[1].getnewaddress() : 2.5 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']

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

        assert_raises_rpc_error(-3, "Unexpected key foo", self.nodes[2].fundrawtransaction, rawtx, {'foo':'bar'})

        # reserveChangeKey was deprecated and is now removed
        assert_raises_rpc_error(-3, "Unexpected key reserveChangeKey", lambda: self.nodes[2].fundrawtransaction(hexstring=rawtx, options={'reserveChangeKey': True}))

    def test_invalid_change_address(self):
        self.log.info("Test fundrawtxn with an invalid change address")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        assert_raises_rpc_error(-5, "Change address must be a valid bitcoin address", self.nodes[2].fundrawtransaction, rawtx, {'changeAddress':'foobar'})

    def test_valid_change_address(self):
        self.log.info("Test fundrawtxn with a provided change address")
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
        assert_equal(change, out['scriptPubKey']['address'])

    def test_change_type(self):
        self.log.info("Test fundrawtxn with a provided change type")
        utx = get_unspent(self.nodes[2].listunspent(), 5)

        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : Decimal(4.0) }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[2].fundrawtransaction, rawtx, {'change_type': None})
        assert_raises_rpc_error(-5, "Unknown change type ''", self.nodes[2].fundrawtransaction, rawtx, {'change_type': ''})
        rawtx = self.nodes[2].fundrawtransaction(rawtx, {'change_type': 'bech32'})
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
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[2].fundrawtransaction, rawtx, {"add_inputs": False})
        # add_inputs is enabled by default
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)

        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for i, out in enumerate(dec_tx['vout']):
            totalOut += out['value']
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
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[2].fundrawtransaction, rawtx, {"add_inputs": False})
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx, {"add_inputs": True})

        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
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
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[2].fundrawtransaction, rawtx, {"add_inputs": False})
        rawtxfund = self.nodes[2].fundrawtransaction(rawtx, {"add_inputs": True})

        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if out['scriptPubKey']['address'] in outputs:
                matchingOuts+=1

        assert_equal(matchingOuts, 2)
        assert_equal(len(dec_tx['vout']), 3)

    def test_invalid_input(self):
        self.log.info("Test fundrawtxn with an invalid vin")
        inputs  = [ {'txid' : "1c7f966dab21119bac53213a2bc7532bff1fa844c124fd750a7d0b1332440bd1", 'vout' : 0} ] #invalid vin!
        outputs = { self.nodes[0].getnewaddress() : 1.0}
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-4, "Insufficient funds", self.nodes[2].fundrawtransaction, rawtx)

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
        signedFee = self.nodes[0].getmempoolentry(txId)['fee']

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
        signedFee = self.nodes[0].getmempoolentry(txId)['fee']

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
        signedFee = self.nodes[0].getmempoolentry(txId)['fee']

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
        signedFee = self.nodes[0].getmempoolentry(txId)['fee']

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
        self.sync_all()

        oldBalance = self.nodes[1].getbalance()
        inputs = []
        outputs = {self.nodes[1].getnewaddress():1.1}
        funded_psbt = wmulti.walletcreatefundedpsbt(inputs=inputs, outputs=outputs, options={'changeAddress': w2.getrawchangeaddress()})['psbt']

        signed_psbt = w2.walletprocesspsbt(funded_psbt)
        final_psbt = w2.finalizepsbt(signed_psbt['psbt'])
        self.nodes[2].sendrawtransaction(final_psbt['hex'])
        self.generate(self.nodes[2], 1)
        self.sync_all()

        # Make sure funds are received at node1.
        assert_equal(oldBalance+Decimal('1.10000000'), self.nodes[1].getbalance())

        wmulti.unloadwallet()

    def test_locked_wallet(self):
        self.log.info("Test fundrawtxn with locked wallet and hardened derivation")

        self.nodes[1].encryptwallet("test")

        if self.options.descriptors:
            self.nodes[1].walletpassphrase('test', 10)
            self.nodes[1].importdescriptors([{
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
            self.nodes[1].walletlock()

        # Drain the keypool.
        self.nodes[1].getnewaddress()
        self.nodes[1].getrawchangeaddress()

        # Choose 2 inputs
        inputs = self.nodes[1].listunspent()[0:2]
        value = sum(inp["amount"] for inp in inputs) - Decimal("0.00000500") # Pay a 500 sat fee
        outputs = {self.nodes[0].getnewaddress():value}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        # fund a transaction that does not require a new key for the change output
        self.nodes[1].fundrawtransaction(rawtx)

        # fund a transaction that requires a new key for the change output
        # creating the key must be impossible because the wallet is locked
        outputs = {self.nodes[0].getnewaddress():value - Decimal("0.1")}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        assert_raises_rpc_error(-4, "Transaction needs a change address, but we can't generate it.", self.nodes[1].fundrawtransaction, rawtx)

        # Refill the keypool.
        self.nodes[1].walletpassphrase("test", 100)
        self.nodes[1].keypoolrefill(8) #need to refill the keypool to get an internal change address
        self.nodes[1].walletlock()

        assert_raises_rpc_error(-13, "walletpassphrase", self.nodes[1].sendtoaddress, self.nodes[0].getnewaddress(), 1.2)

        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress():1.1}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)

        # Now we need to unlock.
        self.nodes[1].walletpassphrase("test", 600)
        signedTx = self.nodes[1].signrawtransactionwithwallet(fundedTx['hex'])
        self.nodes[1].sendrawtransaction(signedTx['hex'])
        self.generate(self.nodes[1], 1)
        self.sync_all()

        # Make sure funds are received at node1.
        assert_equal(oldBalance+Decimal('51.10000000'), self.nodes[0].getbalance())

    def test_many_inputs_fee(self):
        """Multiple (~19) inputs tx test | Compare fee."""
        self.log.info("Test fundrawtxn fee with many inputs")

        # Empty node1, send some small coins from node0 to node1.
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), self.nodes[1].getbalance(), "", "", True)
        self.generate(self.nodes[1], 1)
        self.sync_all()

        for _ in range(20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.generate(self.nodes[0], 1)
        self.sync_all()

        # Fund a tx with ~20 small inputs.
        inputs = []
        outputs = {self.nodes[0].getnewaddress():0.15,self.nodes[0].getnewaddress():0.04}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)

        # Create same transaction over sendtoaddress.
        txId = self.nodes[1].sendmany("", outputs)
        signedFee = self.nodes[1].getmempoolentry(txId)['fee']

        # Compare fee.
        feeDelta = Decimal(fundedTx['fee']) - Decimal(signedFee)
        assert feeDelta >= 0 and feeDelta <= self.fee_tolerance * 19  #~19 inputs

    def test_many_inputs_send(self):
        """Multiple (~19) inputs tx test | sign/send."""
        self.log.info("Test fundrawtxn sign+send with many inputs")

        # Again, empty node1, send some small coins from node0 to node1.
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), self.nodes[1].getbalance(), "", "", True)
        self.generate(self.nodes[1], 1)
        self.sync_all()

        for _ in range(20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.generate(self.nodes[0], 1)
        self.sync_all()

        # Fund a tx with ~20 small inputs.
        oldBalance = self.nodes[0].getbalance()

        inputs = []
        outputs = {self.nodes[0].getnewaddress():0.15,self.nodes[0].getnewaddress():0.04}
        rawtx = self.nodes[1].createrawtransaction(inputs, outputs)
        fundedTx = self.nodes[1].fundrawtransaction(rawtx)
        fundedAndSignedTx = self.nodes[1].signrawtransactionwithwallet(fundedTx['hex'])
        self.nodes[1].sendrawtransaction(fundedAndSignedTx['hex'])
        self.generate(self.nodes[1], 1)
        self.sync_all()
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
        assert_equal(res_dec["vin"][0]["txid"], self.watchonly_txid)

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
        result = wwatch.fundrawtransaction(rawtx, {'includeWatching': True, 'changeAddress': w3.getrawchangeaddress(), 'subtractFeeFromOutputs': [0]})
        res_dec = self.nodes[0].decoderawtransaction(result["hex"])
        assert_equal(len(res_dec["vin"]), 1)
        assert res_dec["vin"][0]["txid"] == self.watchonly_txid

        assert_greater_than(result["fee"], 0)
        assert_equal(result["changepos"], -1)
        assert_equal(result["fee"] + res_dec["vout"][0]["value"], self.watchonly_amount)

        signedtx = wwatch.signrawtransactionwithwallet(result["hex"])
        assert not signedtx["complete"]
        signedtx = self.nodes[0].signrawtransactionwithwallet(signedtx["hex"])
        assert signedtx["complete"]
        self.nodes[0].sendrawtransaction(signedtx["hex"])
        self.generate(self.nodes[0], 1)
        self.sync_all()

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
        result1 = node.fundrawtransaction(rawtx, {"fee_rate": str(2 * btc_kvb_to_sat_vb * self.min_relay_tx_fee)})
        result2 = node.fundrawtransaction(rawtx, {"feeRate": 2 * self.min_relay_tx_fee})
        result3 = node.fundrawtransaction(rawtx, {"fee_rate": 10 * btc_kvb_to_sat_vb * self.min_relay_tx_fee})
        result4 = node.fundrawtransaction(rawtx, {"feeRate": str(10 * self.min_relay_tx_fee)})

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
        result = node.fundrawtransaction(rawtx, {"fee_rate": 10000})
        assert_approx(result["fee"], vexp=0.0141, vspan=0.0001)

        self.log.info("Test fundrawtxn with invalid estimate_mode settings")
        for k, v in {"number": 42, "object": {"foo": "bar"}}.items():
            assert_raises_rpc_error(-3, "Expected type string for estimate_mode, got {}".format(k),
                node.fundrawtransaction, rawtx, {"estimate_mode": v, "conf_target": 0.1, "add_inputs": True})
        for mode in ["", "foo", Decimal("3.141592")]:
            assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"',
                node.fundrawtransaction, rawtx, {"estimate_mode": mode, "conf_target": 0.1, "add_inputs": True})

        self.log.info("Test fundrawtxn with invalid conf_target settings")
        for mode in ["unset", "economical", "conservative"]:
            self.log.debug("{}".format(mode))
            for k, v in {"string": "", "object": {"foo": "bar"}}.items():
                assert_raises_rpc_error(-3, "Expected type number for conf_target, got {}".format(k),
                    node.fundrawtransaction, rawtx, {"estimate_mode": mode, "conf_target": v, "add_inputs": True})
            for n in [-1, 0, 1009]:
                assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008",  # max value of 1008 per src/policy/fees.h
                    node.fundrawtransaction, rawtx, {"estimate_mode": mode, "conf_target": n, "add_inputs": True})

        self.log.info("Test invalid fee rate settings")
        for param, value in {("fee_rate", 100000), ("feeRate", 1.000)}:
            assert_raises_rpc_error(-4, "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)",
                node.fundrawtransaction, rawtx, {param: value, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount out of range",
                node.fundrawtransaction, rawtx, {param: -1, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount is not a number or string",
                node.fundrawtransaction, rawtx, {param: {"foo": "bar"}, "add_inputs": True})
            # Test fee rate values that don't pass fixed-point parsing checks.
            for invalid_value in ["", 0.000000001, 1e-09, 1.111111111, 1111111111111111, "31.999999999999999999999"]:
                assert_raises_rpc_error(-3, "Invalid amount", node.fundrawtransaction, rawtx, {param: invalid_value, "add_inputs": True})
        # Test fee_rate values that cannot be represented in sat/vB.
        for invalid_value in [0.0001, 0.00000001, 0.00099999, 31.99999999, "0.0001", "0.00000001", "0.00099999", "31.99999999"]:
            assert_raises_rpc_error(-3, "Invalid amount",
                node.fundrawtransaction, rawtx, {"fee_rate": invalid_value, "add_inputs": True})

        self.log.info("Test min fee rate checks are bypassed with fundrawtxn, e.g. a fee_rate under 1 sat/vB is allowed")
        node.fundrawtransaction(rawtx, {"fee_rate": 0.999, "add_inputs": True})
        node.fundrawtransaction(rawtx, {"feeRate": 0.00000999, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and fee_rate are passed")
        assert_raises_rpc_error(-8, "Cannot specify both fee_rate (sat/vB) and feeRate (BTC/kvB)",
            node.fundrawtransaction, rawtx, {"fee_rate": 0.1, "feeRate": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and estimate_mode passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and feeRate",
            node.fundrawtransaction, rawtx, {"estimate_mode": "economical", "feeRate": 0.1, "add_inputs": True})

        for param in ["feeRate", "fee_rate"]:
            self.log.info("- raises RPC error if both {} and conf_target are passed".format(param))
            assert_raises_rpc_error(-8, "Cannot specify both conf_target and {}. Please provide either a confirmation "
                "target in blocks for automatic fee estimation, or an explicit fee rate.".format(param),
                node.fundrawtransaction, rawtx, {param: 1, "conf_target": 1, "add_inputs": True})

        self.log.info("- raises RPC error if both fee_rate and estimate_mode are passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and fee_rate",
            node.fundrawtransaction, rawtx, {"fee_rate": 1, "estimate_mode": "economical", "add_inputs": True})

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
            self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": []}),  # empty subtraction list
            self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": [0]}),  # uses self.min_relay_tx_fee (set by settxfee)
            self.nodes[3].fundrawtransaction(rawtx, {"feeRate": 2 * self.min_relay_tx_fee}),
            self.nodes[3].fundrawtransaction(rawtx, {"feeRate": 2 * self.min_relay_tx_fee, "subtractFeeFromOutputs": [0]}),]
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
            self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": []}),  # empty subtraction list
            self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": [0]}),  # uses self.min_relay_tx_fee (set by settxfee)
            self.nodes[3].fundrawtransaction(rawtx, {"fee_rate": 2 * btc_kvb_to_sat_vb * self.min_relay_tx_fee}),
            self.nodes[3].fundrawtransaction(rawtx, {"fee_rate": 2 * btc_kvb_to_sat_vb * self.min_relay_tx_fee, "subtractFeeFromOutputs": [0]}),]
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
                  self.nodes[3].fundrawtransaction(rawtx, {"subtractFeeFromOutputs": [0, 2, 3]})]

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
        txid = self.nodes[0].sendtoaddress(addr, 10)
        vout = find_vout_for_address(self.nodes[0], txid, addr)

        rawtx = self.nodes[0].createrawtransaction([{'txid': txid, 'vout': vout}], [{self.nodes[0].getnewaddress(): 5}])
        fundedtx = self.nodes[0].fundrawtransaction(rawtx, {'subtractFeeFromOutputs': [0]})
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
        for _ in range(1500):
            outputs[recipient.getnewaddress()] = 0.1
        wallet.sendmany("", outputs)
        self.generate(self.nodes[0], 10)
        assert_raises_rpc_error(-4, "Transaction too large", recipient.fundrawtransaction, rawtx)
        self.nodes[0].unloadwallet("large")

    def test_external_inputs(self):
        self.log.info("Test funding with external inputs")

        eckey = ECKey()
        eckey.generate()
        privkey = bytes_to_wif(eckey.get_bytes())

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
        self.nodes[0].generate(6)
        ext_utxo = self.nodes[0].listunspent(addresses=[addr])[0]

        # An external input without solving data should result in an error
        raw_tx = wallet.createrawtransaction([ext_utxo], {self.nodes[0].getnewaddress(): 15})
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.fundrawtransaction, raw_tx)

        # Error conditions
        assert_raises_rpc_error(-5, "'not a pubkey' is not hex", wallet.fundrawtransaction, raw_tx, {"solving_data": {"pubkeys":["not a pubkey"]}})
        assert_raises_rpc_error(-5, "'01234567890a0b0c0d0e0f' is not a valid public key", wallet.fundrawtransaction, raw_tx, {"solving_data": {"pubkeys":["01234567890a0b0c0d0e0f"]}})
        assert_raises_rpc_error(-5, "'not a script' is not hex", wallet.fundrawtransaction, raw_tx, {"solving_data": {"scripts":["not a script"]}})
        assert_raises_rpc_error(-8, "Unable to parse descriptor 'not a descriptor'", wallet.fundrawtransaction, raw_tx, {"solving_data": {"descriptors":["not a descriptor"]}})

        # But funding should work when the solving data is provided
        funded_tx = wallet.fundrawtransaction(raw_tx, {"solving_data": {"pubkeys": [addr_info['pubkey']], "scripts": [addr_info["embedded"]["scriptPubKey"]]}})
        signed_tx = wallet.signrawtransactionwithwallet(funded_tx['hex'])
        assert not signed_tx['complete']
        signed_tx = self.nodes[0].signrawtransactionwithwallet(signed_tx['hex'])
        assert signed_tx['complete']

        funded_tx = wallet.fundrawtransaction(raw_tx, {"solving_data": {"descriptors": [desc]}})
        signed_tx = wallet.signrawtransactionwithwallet(funded_tx['hex'])
        assert not signed_tx['complete']
        signed_tx = self.nodes[0].signrawtransactionwithwallet(signed_tx['hex'])
        assert signed_tx['complete']
        self.nodes[2].unloadwallet("extfund")

    def test_include_unsafe(self):
        self.log.info("Test fundrawtxn with unsafe inputs")

        self.nodes[0].createwallet("unsafe")
        wallet = self.nodes[0].get_wallet_rpc("unsafe")

        # We receive unconfirmed funds from external keys (unsafe outputs).
        addr = wallet.getnewaddress()
        inputs = []
        for i in range(0, 2):
            txid = self.nodes[2].sendtoaddress(addr, 5)
            self.sync_mempools()
            vout = find_vout_for_address(wallet, txid, addr)
            inputs.append((txid, vout))

        # Unsafe inputs are ignored by default.
        rawtx = wallet.createrawtransaction([], [{self.nodes[2].getnewaddress(): 7.5}])
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.fundrawtransaction, rawtx)

        # But we can opt-in to use them for funding.
        fundedtx = wallet.fundrawtransaction(rawtx, {"include_unsafe": True})
        tx_dec = wallet.decoderawtransaction(fundedtx['hex'])
        assert all((txin["txid"], txin["vout"]) in inputs for txin in tx_dec["vin"])
        signedtx = wallet.signrawtransactionwithwallet(fundedtx['hex'])
        assert wallet.testmempoolaccept([signedtx['hex']])[0]["allowed"]

        # And we can also use them once they're confirmed.
        self.generate(self.nodes[0], 1)
        fundedtx = wallet.fundrawtransaction(rawtx, {"include_unsafe": False})
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
        self.generate(self.nodes[0], 1)

        # Create transactions in order to calculate fees for the target bounds that can trigger this bug
        change_tx = tester.fundrawtransaction(tester.createrawtransaction([], [{funds.getnewaddress(): 1.5}]))
        tx = tester.createrawtransaction([], [{funds.getnewaddress(): 2}])
        no_change_tx = tester.fundrawtransaction(tx, {"subtractFeeFromOutputs": [0]})

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

if __name__ == '__main__':
    RawTransactionsTest().main()
