#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the Partially Signed Transaction RPCs.
"""
from decimal import Decimal
from itertools import product
from random import randbytes

from test_framework.descriptors import descsum_create
from test_framework.key import H_POINT
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    MAX_BIP125_RBF_SEQUENCE,
)
from test_framework.psbt import (
    PSBT,
    PSBTMap,
    PSBT_GLOBAL_UNSIGNED_TX,
    PSBT_IN_RIPEMD160,
    PSBT_IN_SHA256,
    PSBT_IN_HASH160,
    PSBT_IN_HASH256,
    PSBT_IN_NON_WITNESS_UTXO,
    PSBT_IN_WITNESS_UTXO,
    PSBT_OUT_TAP_TREE,
)
from test_framework.script import CScript, OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)
from test_framework.wallet_util import (
    calculate_input_weight,
    generate_keypair,
    get_generate_key,
)

import json
import os


class PSBTTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-walletrbf=1", "-addresstype=bech32", "-changetype=bech32"], #TODO: Remove address type restrictions once taproot has psbt extensions
            ["-walletrbf=0", "-changetype=legacy"],
            []
        ]
        # whitelist peers to speed up tx relay / mempool sync
        for args in self.extra_args:
            args.append("-whitelist=noban@127.0.0.1")
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_utxo_conversion(self):
        self.log.info("Check that non-witness UTXOs are removed for segwit v1+ inputs")
        mining_node = self.nodes[2]
        offline_node = self.nodes[0]
        online_node = self.nodes[1]

        # Disconnect offline node from others
        # Topology of test network is linear, so this one call is enough
        self.disconnect_nodes(0, 1)

        # Create watchonly on online_node
        online_node.createwallet(wallet_name='wonline', disable_private_keys=True)
        wonline = online_node.get_wallet_rpc('wonline')
        w2 = online_node.get_wallet_rpc(self.default_wallet_name)

        # Mine a transaction that credits the offline address
        offline_addr = offline_node.getnewaddress(address_type="bech32m")
        online_addr = w2.getnewaddress(address_type="bech32m")
        wonline.importaddress(offline_addr, "", False)
        mining_wallet = mining_node.get_wallet_rpc(self.default_wallet_name)
        mining_wallet.sendtoaddress(address=offline_addr, amount=1.0)
        self.generate(mining_node, nblocks=1, sync_fun=lambda: self.sync_all([online_node, mining_node]))

        # Construct an unsigned PSBT on the online node
        utxos = wonline.listunspent(addresses=[offline_addr])
        raw = wonline.createrawtransaction([{"txid":utxos[0]["txid"], "vout":utxos[0]["vout"]}],[{online_addr:0.9999}])
        psbt = wonline.walletprocesspsbt(online_node.converttopsbt(raw))["psbt"]
        assert not "not_witness_utxo" in mining_node.decodepsbt(psbt)["inputs"][0]

        # add non-witness UTXO manually
        psbt_new = PSBT.from_base64(psbt)
        prev_tx = wonline.gettransaction(utxos[0]["txid"])["hex"]
        psbt_new.i[0].map[PSBT_IN_NON_WITNESS_UTXO] = bytes.fromhex(prev_tx)
        assert "non_witness_utxo" in mining_node.decodepsbt(psbt_new.to_base64())["inputs"][0]

        # Have the offline node sign the PSBT (which will remove the non-witness UTXO)
        signed_psbt = offline_node.walletprocesspsbt(psbt_new.to_base64())
        assert not "non_witness_utxo" in mining_node.decodepsbt(signed_psbt["psbt"])["inputs"][0]

        # Make sure we can mine the resulting transaction
        txid = mining_node.sendrawtransaction(signed_psbt["hex"])
        self.generate(mining_node, nblocks=1, sync_fun=lambda: self.sync_all([online_node, mining_node]))
        assert_equal(online_node.gettxout(txid,0)["confirmations"], 1)

        wonline.unloadwallet()

        # Reconnect
        self.connect_nodes(1, 0)
        self.connect_nodes(0, 2)

    def test_input_confs_control(self):
        self.nodes[0].createwallet("minconf")
        wallet = self.nodes[0].get_wallet_rpc("minconf")

        # Fund the wallet with different chain heights
        for _ in range(2):
            self.nodes[1].sendmany("", {wallet.getnewaddress():1, wallet.getnewaddress():1})
            self.generate(self.nodes[1], 1)

        unconfirmed_txid = wallet.sendtoaddress(wallet.getnewaddress(), 0.5)

        self.log.info("Crafting PSBT using an unconfirmed input")
        target_address = self.nodes[1].getnewaddress()
        psbtx1 = wallet.walletcreatefundedpsbt([], {target_address: 0.1}, 0, {'fee_rate': 1, 'maxconf': 0})['psbt']

        # Make sure we only had the one input
        tx1_inputs = self.nodes[0].decodepsbt(psbtx1)['tx']['vin']
        assert_equal(len(tx1_inputs), 1)

        utxo1 = tx1_inputs[0]
        assert_equal(unconfirmed_txid, utxo1['txid'])

        signed_tx1 = wallet.walletprocesspsbt(psbtx1)
        txid1 = self.nodes[0].sendrawtransaction(signed_tx1['hex'])

        mempool = self.nodes[0].getrawmempool()
        assert txid1 in mempool

        self.log.info("Fail to craft a new PSBT that sends more funds with add_inputs = False")
        assert_raises_rpc_error(-4, "The preselected coins total amount does not cover the transaction target. Please allow other inputs to be automatically selected or include more coins manually", wallet.walletcreatefundedpsbt, [{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': False})

        self.log.info("Fail to craft a new PSBT with minconf above highest one")
        assert_raises_rpc_error(-4, "Insufficient funds", wallet.walletcreatefundedpsbt, [{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': True, 'minconf': 3, 'fee_rate': 10})

        self.log.info("Fail to broadcast a new PSBT with maxconf 0 due to BIP125 rules to verify it actually chose unconfirmed outputs")
        psbt_invalid = wallet.walletcreatefundedpsbt([{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': True, 'maxconf': 0, 'fee_rate': 10})['psbt']
        signed_invalid = wallet.walletprocesspsbt(psbt_invalid)
        assert_raises_rpc_error(-26, "bad-txns-spends-conflicting-tx", self.nodes[0].sendrawtransaction, signed_invalid['hex'])

        self.log.info("Craft a replacement adding inputs with highest confs possible")
        psbtx2 = wallet.walletcreatefundedpsbt([{'txid': utxo1['txid'], 'vout': utxo1['vout']}], {target_address: 1}, 0, {'add_inputs': True, 'minconf': 2, 'fee_rate': 10})['psbt']
        tx2_inputs = self.nodes[0].decodepsbt(psbtx2)['tx']['vin']
        assert_greater_than_or_equal(len(tx2_inputs), 2)
        for vin in tx2_inputs:
            if vin['txid'] != unconfirmed_txid:
                assert_greater_than_or_equal(self.nodes[0].gettxout(vin['txid'], vin['vout'])['confirmations'], 2)

        signed_tx2 = wallet.walletprocesspsbt(psbtx2)
        txid2 = self.nodes[0].sendrawtransaction(signed_tx2['hex'])

        mempool = self.nodes[0].getrawmempool()
        assert txid1 not in mempool
        assert txid2 in mempool

        wallet.unloadwallet()

    def assert_change_type(self, psbtx, expected_type):
        """Assert that the given PSBT has a change output with the given type."""

        # The decodepsbt RPC is stateless and independent of any settings, we can always just call it on the first node
        decoded_psbt = self.nodes[0].decodepsbt(psbtx["psbt"])
        changepos = psbtx["changepos"]
        assert_equal(decoded_psbt["tx"]["vout"][changepos]["scriptPubKey"]["type"], expected_type)

    def run_test(self):
        # Create and fund a raw tx for sending 10 BTC
        psbtx1 = self.nodes[0].walletcreatefundedpsbt([], {self.nodes[2].getnewaddress():10})['psbt']

        # If inputs are specified, do not automatically add more:
        utxo1 = self.nodes[0].listunspent()[0]
        assert_raises_rpc_error(-4, "The preselected coins total amount does not cover the transaction target. "
                                    "Please allow other inputs to be automatically selected or include more coins manually",
                                self.nodes[0].walletcreatefundedpsbt, [{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90})

        psbtx1 = self.nodes[0].walletcreatefundedpsbt([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():90}, 0, {"add_inputs": True})['psbt']
        assert_equal(len(self.nodes[0].decodepsbt(psbtx1)['tx']['vin']), 2)

        # Inputs argument can be null
        self.nodes[0].walletcreatefundedpsbt(None, {self.nodes[2].getnewaddress():10})

        # Node 1 should not be able to add anything to it but still return the psbtx same as before
        psbtx = self.nodes[1].walletprocesspsbt(psbtx1)['psbt']
        assert_equal(psbtx1, psbtx)

        # Node 0 should not be able to sign the transaction with the wallet is locked
        self.nodes[0].encryptwallet("password")
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].walletprocesspsbt, psbtx)

        # Node 0 should be able to process without signing though
        unsigned_tx = self.nodes[0].walletprocesspsbt(psbtx, False)
        assert_equal(unsigned_tx['complete'], False)

        self.nodes[0].walletpassphrase(passphrase="password", timeout=1000000)

        # Sign the transaction but don't finalize
        processed_psbt = self.nodes[0].walletprocesspsbt(psbt=psbtx, finalize=False)
        assert "hex" not in processed_psbt
        signed_psbt = processed_psbt['psbt']

        # Finalize and send
        finalized_hex = self.nodes[0].finalizepsbt(signed_psbt)['hex']
        self.nodes[0].sendrawtransaction(finalized_hex)

        # Alternative method: sign AND finalize in one command
        processed_finalized_psbt = self.nodes[0].walletprocesspsbt(psbt=psbtx, finalize=True)
        finalized_psbt = processed_finalized_psbt['psbt']
        finalized_psbt_hex = processed_finalized_psbt['hex']
        assert signed_psbt != finalized_psbt
        assert finalized_psbt_hex == finalized_hex

        # Manually selected inputs can be locked:
        assert_equal(len(self.nodes[0].listlockunspent()), 0)
        utxo1 = self.nodes[0].listunspent()[0]
        psbtx1 = self.nodes[0].walletcreatefundedpsbt([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():1}, 0,{"lockUnspents": True})["psbt"]
        assert_equal(len(self.nodes[0].listlockunspent()), 1)

        # Locks are ignored for manually selected inputs
        self.nodes[0].walletcreatefundedpsbt([{"txid": utxo1['txid'], "vout": utxo1['vout']}], {self.nodes[2].getnewaddress():1}, 0)

        # Create p2sh, p2wpkh, and p2wsh addresses
        pubkey0 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())['pubkey']
        pubkey1 = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['pubkey']
        pubkey2 = self.nodes[2].getaddressinfo(self.nodes[2].getnewaddress())['pubkey']

        # Setup watchonly wallets
        self.nodes[2].createwallet(wallet_name='wmulti', disable_private_keys=True)
        wmulti = self.nodes[2].get_wallet_rpc('wmulti')

        # Create all the addresses
        p2sh = wmulti.addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "legacy")['address']
        p2wsh = wmulti.addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "bech32")['address']
        p2sh_p2wsh = wmulti.addmultisigaddress(2, [pubkey0, pubkey1, pubkey2], "", "p2sh-segwit")['address']
        if not self.options.descriptors:
            wmulti.importaddress(p2sh)
            wmulti.importaddress(p2wsh)
            wmulti.importaddress(p2sh_p2wsh)
        p2wpkh = self.nodes[1].getnewaddress("", "bech32")
        p2pkh = self.nodes[1].getnewaddress("", "legacy")
        p2sh_p2wpkh = self.nodes[1].getnewaddress("", "p2sh-segwit")

        # fund those addresses
        rawtx = self.nodes[0].createrawtransaction([], {p2sh:10, p2wsh:10, p2wpkh:10, p2sh_p2wsh:10, p2sh_p2wpkh:10, p2pkh:10})
        rawtx = self.nodes[0].fundrawtransaction(rawtx, {"changePosition":3})
        signed_tx = self.nodes[0].signrawtransactionwithwallet(rawtx['hex'])['hex']
        txid = self.nodes[0].sendrawtransaction(signed_tx)
        self.generate(self.nodes[0], 6)

        # Find the output pos
        p2sh_pos = -1
        p2wsh_pos = -1
        p2wpkh_pos = -1
        p2pkh_pos = -1
        p2sh_p2wsh_pos = -1
        p2sh_p2wpkh_pos = -1
        decoded = self.nodes[0].decoderawtransaction(signed_tx)
        for out in decoded['vout']:
            if out['scriptPubKey']['address'] == p2sh:
                p2sh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2wsh:
                p2wsh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2wpkh:
                p2wpkh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2sh_p2wsh:
                p2sh_p2wsh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2sh_p2wpkh:
                p2sh_p2wpkh_pos = out['n']
            elif out['scriptPubKey']['address'] == p2pkh:
                p2pkh_pos = out['n']

        inputs = [{"txid": txid, "vout": p2wpkh_pos}, {"txid": txid, "vout": p2sh_p2wpkh_pos}, {"txid": txid, "vout": p2pkh_pos}]
        outputs = [{self.nodes[1].getnewaddress(): 29.99}]

        # spend single key from node 1
        created_psbt = self.nodes[1].walletcreatefundedpsbt(inputs, outputs)
        walletprocesspsbt_out = self.nodes[1].walletprocesspsbt(created_psbt['psbt'])
        # Make sure it has both types of UTXOs
        decoded = self.nodes[1].decodepsbt(walletprocesspsbt_out['psbt'])
        assert 'non_witness_utxo' in decoded['inputs'][0]
        assert 'witness_utxo' in decoded['inputs'][0]
        # Check decodepsbt fee calculation (input values shall only be counted once per UTXO)
        assert_equal(decoded['fee'], created_psbt['fee'])
        assert_equal(walletprocesspsbt_out['complete'], True)
        self.nodes[1].sendrawtransaction(walletprocesspsbt_out['hex'])

        self.log.info("Test walletcreatefundedpsbt fee rate of 10000 sat/vB and 0.1 BTC/kvB produces a total fee at or slightly below -maxtxfee (~0.05290000)")
        res1 = self.nodes[1].walletcreatefundedpsbt(inputs, outputs, 0, {"fee_rate": 10000, "add_inputs": True})
        assert_approx(res1["fee"], 0.055, 0.005)
        res2 = self.nodes[1].walletcreatefundedpsbt(inputs, outputs, 0, {"feeRate": "0.1", "add_inputs": True})
        assert_approx(res2["fee"], 0.055, 0.005)

        self.log.info("Test min fee rate checks with walletcreatefundedpsbt are bypassed, e.g. a fee_rate under 1 sat/vB is allowed")
        res3 = self.nodes[1].walletcreatefundedpsbt(inputs, outputs, 0, {"fee_rate": "0.999", "add_inputs": True})
        assert_approx(res3["fee"], 0.00000381, 0.0000001)
        res4 = self.nodes[1].walletcreatefundedpsbt(inputs, outputs, 0, {"feeRate": 0.00000999, "add_inputs": True})
        assert_approx(res4["fee"], 0.00000381, 0.0000001)

        self.log.info("Test min fee rate checks with walletcreatefundedpsbt are bypassed and that funding non-standard 'zero-fee' transactions is valid")
        for param, zero_value in product(["fee_rate", "feeRate"], [0, 0.000, 0.00000000, "0", "0.000", "0.00000000"]):
            assert_equal(0, self.nodes[1].walletcreatefundedpsbt(inputs, outputs, 0, {param: zero_value, "add_inputs": True})["fee"])

        self.log.info("Test invalid fee rate settings")
        for param, value in {("fee_rate", 100000), ("feeRate", 1)}:
            assert_raises_rpc_error(-4, "Fee exceeds maximum configured by user (maxtxfee)",
                self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {param: value, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount out of range",
                self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {param: -1, "add_inputs": True})
            assert_raises_rpc_error(-3, "Amount is not a number or string",
                self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {param: {"foo": "bar"}, "add_inputs": True})
            # Test fee rate values that don't pass fixed-point parsing checks.
            for invalid_value in ["", 0.000000001, 1e-09, 1.111111111, 1111111111111111, "31.999999999999999999999"]:
                assert_raises_rpc_error(-3, "Invalid amount",
                    self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {param: invalid_value, "add_inputs": True})
        # Test fee_rate values that cannot be represented in sat/vB.
        for invalid_value in [0.0001, 0.00000001, 0.00099999, 31.99999999]:
            assert_raises_rpc_error(-3, "Invalid amount",
                self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"fee_rate": invalid_value, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and fee_rate are passed")
        assert_raises_rpc_error(-8, "Cannot specify both fee_rate (sat/vB) and feeRate (BTC/kvB)",
            self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"fee_rate": 0.1, "feeRate": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error if both feeRate and estimate_mode passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and feeRate",
            self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"estimate_mode": "economical", "feeRate": 0.1, "add_inputs": True})

        for param in ["feeRate", "fee_rate"]:
            self.log.info("- raises RPC error if both {} and conf_target are passed".format(param))
            assert_raises_rpc_error(-8, "Cannot specify both conf_target and {}. Please provide either a confirmation "
                "target in blocks for automatic fee estimation, or an explicit fee rate.".format(param),
                self.nodes[1].walletcreatefundedpsbt ,inputs, outputs, 0, {param: 1, "conf_target": 1, "add_inputs": True})

        self.log.info("- raises RPC error if both fee_rate and estimate_mode are passed")
        assert_raises_rpc_error(-8, "Cannot specify both estimate_mode and fee_rate",
            self.nodes[1].walletcreatefundedpsbt ,inputs, outputs, 0, {"fee_rate": 1, "estimate_mode": "economical", "add_inputs": True})

        self.log.info("- raises RPC error with invalid estimate_mode settings")
        for k, v in {"number": 42, "object": {"foo": "bar"}}.items():
            assert_raises_rpc_error(-3, f"JSON value of type {k} for field estimate_mode is not of expected type string",
                self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"estimate_mode": v, "conf_target": 0.1, "add_inputs": True})
        for mode in ["", "foo", Decimal("3.141592")]:
            assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"',
                self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": 0.1, "add_inputs": True})

        self.log.info("- raises RPC error with invalid conf_target settings")
        for mode in ["unset", "economical", "conservative"]:
            self.log.debug("{}".format(mode))
            for k, v in {"string": "", "object": {"foo": "bar"}}.items():
                assert_raises_rpc_error(-3, f"JSON value of type {k} for field conf_target is not of expected type number",
                    self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": v, "add_inputs": True})
            for n in [-1, 0, 1009]:
                assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008",  # max value of 1008 per src/policy/fees.h
                    self.nodes[1].walletcreatefundedpsbt, inputs, outputs, 0, {"estimate_mode": mode, "conf_target": n, "add_inputs": True})

        self.log.info("Test walletcreatefundedpsbt with too-high fee rate produces total fee well above -maxtxfee and raises RPC error")
        # previously this was silently capped at -maxtxfee
        for bool_add, outputs_array in {True: outputs, False: [{self.nodes[1].getnewaddress(): 1}]}.items():
            msg = "Fee exceeds maximum configured by user (maxtxfee)"
            assert_raises_rpc_error(-4, msg, self.nodes[1].walletcreatefundedpsbt, inputs, outputs_array, 0, {"fee_rate": 1000000, "add_inputs": bool_add})
            assert_raises_rpc_error(-4, msg, self.nodes[1].walletcreatefundedpsbt, inputs, outputs_array, 0, {"feeRate": 1, "add_inputs": bool_add})

        self.log.info("Test various PSBT operations")
        # partially sign multisig things with node 1
        psbtx = wmulti.walletcreatefundedpsbt(inputs=[{"txid":txid,"vout":p2wsh_pos},{"txid":txid,"vout":p2sh_pos},{"txid":txid,"vout":p2sh_p2wsh_pos}], outputs={self.nodes[1].getnewaddress():29.99}, changeAddress=self.nodes[1].getrawchangeaddress())['psbt']
        walletprocesspsbt_out = self.nodes[1].walletprocesspsbt(psbtx)
        psbtx = walletprocesspsbt_out['psbt']
        assert_equal(walletprocesspsbt_out['complete'], False)

        # Unload wmulti, we don't need it anymore
        wmulti.unloadwallet()

        # partially sign with node 2. This should be complete and sendable
        walletprocesspsbt_out = self.nodes[2].walletprocesspsbt(psbtx)
        assert_equal(walletprocesspsbt_out['complete'], True)
        self.nodes[2].sendrawtransaction(walletprocesspsbt_out['hex'])

        # check that walletprocesspsbt fails to decode a non-psbt
        rawtx = self.nodes[1].createrawtransaction([{"txid":txid,"vout":p2wpkh_pos}], {self.nodes[1].getnewaddress():9.99})
        assert_raises_rpc_error(-22, "TX decode failed", self.nodes[1].walletprocesspsbt, rawtx)

        # Convert a non-psbt to psbt and make sure we can decode it
        rawtx = self.nodes[0].createrawtransaction([], {self.nodes[1].getnewaddress():10})
        rawtx = self.nodes[0].fundrawtransaction(rawtx)
        new_psbt = self.nodes[0].converttopsbt(rawtx['hex'])
        self.nodes[0].decodepsbt(new_psbt)

        # Make sure that a non-psbt with signatures cannot be converted
        signedtx = self.nodes[0].signrawtransactionwithwallet(rawtx['hex'])
        assert_raises_rpc_error(-22, "Inputs must not have scriptSigs and scriptWitnesses",
                                self.nodes[0].converttopsbt, hexstring=signedtx['hex'])  # permitsigdata=False by default
        assert_raises_rpc_error(-22, "Inputs must not have scriptSigs and scriptWitnesses",
                                self.nodes[0].converttopsbt, hexstring=signedtx['hex'], permitsigdata=False)
        assert_raises_rpc_error(-22, "Inputs must not have scriptSigs and scriptWitnesses",
                                self.nodes[0].converttopsbt, hexstring=signedtx['hex'], permitsigdata=False, iswitness=True)
        # Unless we allow it to convert and strip signatures
        self.nodes[0].converttopsbt(hexstring=signedtx['hex'], permitsigdata=True)

        # Create outputs to nodes 1 and 2
        # (note that we intentionally create two different txs here, as we want
        #  to check that each node is missing prevout data for one of the two
        #  utxos, see "should only have data for one input" test below)
        node1_addr = self.nodes[1].getnewaddress()
        node2_addr = self.nodes[2].getnewaddress()
        utxo1 = self.create_outpoints(self.nodes[0], outputs=[{node1_addr: 13}])[0]
        utxo2 = self.create_outpoints(self.nodes[0], outputs=[{node2_addr: 13}])[0]
        self.generate(self.nodes[0], 6)[0]

        # Create a psbt spending outputs from nodes 1 and 2
        psbt_orig = self.nodes[0].createpsbt([utxo1, utxo2], {self.nodes[0].getnewaddress():25.999})

        # Update psbts, should only have data for one input and not the other
        psbt1 = self.nodes[1].walletprocesspsbt(psbt_orig, False, "ALL")['psbt']
        psbt1_decoded = self.nodes[0].decodepsbt(psbt1)
        assert psbt1_decoded['inputs'][0] and not psbt1_decoded['inputs'][1]
        # Check that BIP32 path was added
        assert "bip32_derivs" in psbt1_decoded['inputs'][0]
        psbt2 = self.nodes[2].walletprocesspsbt(psbt_orig, False, "ALL", False)['psbt']
        psbt2_decoded = self.nodes[0].decodepsbt(psbt2)
        assert not psbt2_decoded['inputs'][0] and psbt2_decoded['inputs'][1]
        # Check that BIP32 paths were not added
        assert "bip32_derivs" not in psbt2_decoded['inputs'][1]

        # Sign PSBTs (workaround issue #18039)
        psbt1 = self.nodes[1].walletprocesspsbt(psbt_orig)['psbt']
        psbt2 = self.nodes[2].walletprocesspsbt(psbt_orig)['psbt']

        # Combine, finalize, and send the psbts
        combined = self.nodes[0].combinepsbt([psbt1, psbt2])
        finalized = self.nodes[0].finalizepsbt(combined)['hex']
        self.nodes[0].sendrawtransaction(finalized)
        self.generate(self.nodes[0], 6)

        # Test additional args in walletcreatepsbt
        # Make sure both pre-included and funded inputs
        # have the correct sequence numbers based on
        # replaceable arg
        block_height = self.nodes[0].getblockcount()
        unspent = self.nodes[0].listunspent()[0]
        psbtx_info = self.nodes[0].walletcreatefundedpsbt([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, {"replaceable": False, "add_inputs": True}, False)
        decoded_psbt = self.nodes[0].decodepsbt(psbtx_info["psbt"])
        for tx_in, psbt_in in zip(decoded_psbt["tx"]["vin"], decoded_psbt["inputs"]):
            assert_greater_than(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" not in psbt_in
        assert_equal(decoded_psbt["tx"]["locktime"], block_height+2)

        # Same construction with only locktime set and RBF explicitly enabled
        psbtx_info = self.nodes[0].walletcreatefundedpsbt([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height, {"replaceable": True, "add_inputs": True}, True)
        decoded_psbt = self.nodes[0].decodepsbt(psbtx_info["psbt"])
        for tx_in, psbt_in in zip(decoded_psbt["tx"]["vin"], decoded_psbt["inputs"]):
            assert_equal(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" in psbt_in
        assert_equal(decoded_psbt["tx"]["locktime"], block_height)

        # Same construction without optional arguments
        psbtx_info = self.nodes[0].walletcreatefundedpsbt([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}])
        decoded_psbt = self.nodes[0].decodepsbt(psbtx_info["psbt"])
        for tx_in, psbt_in in zip(decoded_psbt["tx"]["vin"], decoded_psbt["inputs"]):
            assert_equal(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" in psbt_in
        assert_equal(decoded_psbt["tx"]["locktime"], 0)

        # Same construction without optional arguments, for a node with -walletrbf=0
        unspent1 = self.nodes[1].listunspent()[0]
        psbtx_info = self.nodes[1].walletcreatefundedpsbt([{"txid":unspent1["txid"], "vout":unspent1["vout"]}], [{self.nodes[2].getnewaddress():unspent1["amount"]+1}], block_height, {"add_inputs": True})
        decoded_psbt = self.nodes[1].decodepsbt(psbtx_info["psbt"])
        for tx_in, psbt_in in zip(decoded_psbt["tx"]["vin"], decoded_psbt["inputs"]):
            assert_greater_than(tx_in["sequence"], MAX_BIP125_RBF_SEQUENCE)
            assert "bip32_derivs" in psbt_in

        # Make sure change address wallet does not have P2SH innerscript access to results in success
        # when attempting BnB coin selection
        self.nodes[0].walletcreatefundedpsbt([], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], block_height+2, {"changeAddress":self.nodes[1].getnewaddress()}, False)

        # Make sure the wallet's change type is respected by default
        small_output = {self.nodes[0].getnewaddress():0.1}
        psbtx_native = self.nodes[0].walletcreatefundedpsbt([], [small_output])
        self.assert_change_type(psbtx_native, "witness_v0_keyhash")
        psbtx_legacy = self.nodes[1].walletcreatefundedpsbt([], [small_output])
        self.assert_change_type(psbtx_legacy, "pubkeyhash")

        # Make sure the change type of the wallet can also be overwritten
        psbtx_np2wkh = self.nodes[1].walletcreatefundedpsbt([], [small_output], 0, {"change_type":"p2sh-segwit"})
        self.assert_change_type(psbtx_np2wkh, "scripthash")

        # Make sure the change type cannot be specified if a change address is given
        invalid_options = {"change_type":"legacy","changeAddress":self.nodes[0].getnewaddress()}
        assert_raises_rpc_error(-8, "both change address and address type options", self.nodes[0].walletcreatefundedpsbt, [], [small_output], 0, invalid_options)

        # Regression test for 14473 (mishandling of already-signed witness transaction):
        psbtx_info = self.nodes[0].walletcreatefundedpsbt([{"txid":unspent["txid"], "vout":unspent["vout"]}], [{self.nodes[2].getnewaddress():unspent["amount"]+1}], 0, {"add_inputs": True})
        complete_psbt = self.nodes[0].walletprocesspsbt(psbtx_info["psbt"])
        double_processed_psbt = self.nodes[0].walletprocesspsbt(complete_psbt["psbt"])
        assert_equal(complete_psbt, double_processed_psbt)
        # We don't care about the decode result, but decoding must succeed.
        self.nodes[0].decodepsbt(double_processed_psbt["psbt"])

        # Make sure unsafe inputs are included if specified
        self.nodes[2].createwallet(wallet_name="unsafe")
        wunsafe = self.nodes[2].get_wallet_rpc("unsafe")
        self.nodes[0].sendtoaddress(wunsafe.getnewaddress(), 2)
        self.sync_mempools()
        assert_raises_rpc_error(-4, "Insufficient funds", wunsafe.walletcreatefundedpsbt, [], [{self.nodes[0].getnewaddress(): 1}])
        wunsafe.walletcreatefundedpsbt([], [{self.nodes[0].getnewaddress(): 1}], 0, {"include_unsafe": True})

        # BIP 174 Test Vectors

        # Check that unknown values are just passed through
        unknown_psbt = "cHNidP8BAD8CAAAAAf//////////////////////////////////////////AAAAAAD/////AQAAAAAAAAAAA2oBAAAAAAAACg8BAgMEBQYHCAkPAQIDBAUGBwgJCgsMDQ4PAAA="
        unknown_out = self.nodes[0].walletprocesspsbt(unknown_psbt)['psbt']
        assert_equal(unknown_psbt, unknown_out)

        # Open the data file
        with open(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/rpc_psbt.json'), encoding='utf-8') as f:
            d = json.load(f)
            invalids = d['invalid']
            invalid_with_msgs = d["invalid_with_msg"]
            valids = d['valid']
            creators = d['creator']
            signers = d['signer']
            combiners = d['combiner']
            finalizers = d['finalizer']
            extractors = d['extractor']

        # Invalid PSBTs
        for invalid in invalids:
            assert_raises_rpc_error(-22, "TX decode failed", self.nodes[0].decodepsbt, invalid)
        for invalid in invalid_with_msgs:
            psbt, msg = invalid
            assert_raises_rpc_error(-22, f"TX decode failed {msg}", self.nodes[0].decodepsbt, psbt)

        # Valid PSBTs
        for valid in valids:
            self.nodes[0].decodepsbt(valid)

        # Creator Tests
        for creator in creators:
            created_tx = self.nodes[0].createpsbt(inputs=creator['inputs'], outputs=creator['outputs'], replaceable=False)
            assert_equal(created_tx, creator['result'])

        # Signer tests
        for i, signer in enumerate(signers):
            self.nodes[2].createwallet(wallet_name="wallet{}".format(i))
            wrpc = self.nodes[2].get_wallet_rpc("wallet{}".format(i))
            for key in signer['privkeys']:
                wrpc.importprivkey(key)
            signed_tx = wrpc.walletprocesspsbt(signer['psbt'], True, "ALL")['psbt']
            assert_equal(signed_tx, signer['result'])

        # Combiner test
        for combiner in combiners:
            combined = self.nodes[2].combinepsbt(combiner['combine'])
            assert_equal(combined, combiner['result'])

        # Empty combiner test
        assert_raises_rpc_error(-8, "Parameter 'txs' cannot be empty", self.nodes[0].combinepsbt, [])

        # Finalizer test
        for finalizer in finalizers:
            finalized = self.nodes[2].finalizepsbt(finalizer['finalize'], False)['psbt']
            assert_equal(finalized, finalizer['result'])

        # Extractor test
        for extractor in extractors:
            extracted = self.nodes[2].finalizepsbt(extractor['extract'], True)['hex']
            assert_equal(extracted, extractor['result'])

        # Unload extra wallets
        for i, signer in enumerate(signers):
            self.nodes[2].unloadwallet("wallet{}".format(i))

        if self.options.descriptors:
            self.test_utxo_conversion()

        self.test_input_confs_control()

        # Test that psbts with p2pkh outputs are created properly
        p2pkh = self.nodes[0].getnewaddress(address_type='legacy')
        psbt = self.nodes[1].walletcreatefundedpsbt([], [{p2pkh : 1}], 0, {"includeWatching" : True}, True)
        self.nodes[0].decodepsbt(psbt['psbt'])

        # Test decoding error: invalid base64
        assert_raises_rpc_error(-22, "TX decode failed invalid base64", self.nodes[0].decodepsbt, ";definitely not base64;")

        # Send to all types of addresses
        addr1 = self.nodes[1].getnewaddress("", "bech32")
        addr2 = self.nodes[1].getnewaddress("", "legacy")
        addr3 = self.nodes[1].getnewaddress("", "p2sh-segwit")
        utxo1, utxo2, utxo3 = self.create_outpoints(self.nodes[1], outputs=[{addr1: 11}, {addr2: 11}, {addr3: 11}])
        self.sync_all()

        def test_psbt_input_keys(psbt_input, keys):
            """Check that the psbt input has only the expected keys."""
            assert_equal(set(keys), set(psbt_input.keys()))

        # Create a PSBT. None of the inputs are filled initially
        psbt = self.nodes[1].createpsbt([utxo1, utxo2, utxo3], {self.nodes[0].getnewaddress():32.999})
        decoded = self.nodes[1].decodepsbt(psbt)
        test_psbt_input_keys(decoded['inputs'][0], [])
        test_psbt_input_keys(decoded['inputs'][1], [])
        test_psbt_input_keys(decoded['inputs'][2], [])

        # Update a PSBT with UTXOs from the node
        # Bech32 inputs should be filled with witness UTXO. Other inputs should not be filled because they are non-witness
        updated = self.nodes[1].utxoupdatepsbt(psbt)
        decoded = self.nodes[1].decodepsbt(updated)
        test_psbt_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo'])
        test_psbt_input_keys(decoded['inputs'][1], ['non_witness_utxo'])
        test_psbt_input_keys(decoded['inputs'][2], ['non_witness_utxo'])

        # Try again, now while providing descriptors, making P2SH-segwit work, and causing bip32_derivs and redeem_script to be filled in
        descs = [self.nodes[1].getaddressinfo(addr)['desc'] for addr in [addr1,addr2,addr3]]
        updated = self.nodes[1].utxoupdatepsbt(psbt=psbt, descriptors=descs)
        decoded = self.nodes[1].decodepsbt(updated)
        test_psbt_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo', 'bip32_derivs'])
        test_psbt_input_keys(decoded['inputs'][1], ['non_witness_utxo', 'bip32_derivs'])
        test_psbt_input_keys(decoded['inputs'][2], ['non_witness_utxo','witness_utxo', 'bip32_derivs', 'redeem_script'])

        # Two PSBTs with a common input should not be joinable
        psbt1 = self.nodes[1].createpsbt([utxo1], {self.nodes[0].getnewaddress():Decimal('10.999')})
        assert_raises_rpc_error(-8, "exists in multiple PSBTs", self.nodes[1].joinpsbts, [psbt1, updated])

        # Join two distinct PSBTs
        addr4 = self.nodes[1].getnewaddress("", "p2sh-segwit")
        utxo4 = self.create_outpoints(self.nodes[0], outputs=[{addr4: 5}])[0]
        self.generate(self.nodes[0], 6)
        psbt2 = self.nodes[1].createpsbt([utxo4], {self.nodes[0].getnewaddress():Decimal('4.999')})
        psbt2 = self.nodes[1].walletprocesspsbt(psbt2)['psbt']
        psbt2_decoded = self.nodes[0].decodepsbt(psbt2)
        assert "final_scriptwitness" in psbt2_decoded['inputs'][0] and "final_scriptSig" in psbt2_decoded['inputs'][0]
        joined = self.nodes[0].joinpsbts([psbt, psbt2])
        joined_decoded = self.nodes[0].decodepsbt(joined)
        assert len(joined_decoded['inputs']) == 4 and len(joined_decoded['outputs']) == 2 and "final_scriptwitness" not in joined_decoded['inputs'][3] and "final_scriptSig" not in joined_decoded['inputs'][3]

        # Check that joining shuffles the inputs and outputs
        # 10 attempts should be enough to get a shuffled join
        shuffled = False
        for _ in range(10):
            shuffled_joined = self.nodes[0].joinpsbts([psbt, psbt2])
            shuffled |= joined != shuffled_joined
            if shuffled:
                break
        assert shuffled

        # Newly created PSBT needs UTXOs and updating
        addr = self.nodes[1].getnewaddress("", "p2sh-segwit")
        utxo = self.create_outpoints(self.nodes[0], outputs=[{addr: 7}])[0]
        addrinfo = self.nodes[1].getaddressinfo(addr)
        self.generate(self.nodes[0], 6)[0]
        psbt = self.nodes[1].createpsbt([utxo], {self.nodes[0].getnewaddress("", "p2sh-segwit"):Decimal('6.999')})
        analyzed = self.nodes[0].analyzepsbt(psbt)
        assert not analyzed['inputs'][0]['has_utxo'] and not analyzed['inputs'][0]['is_final'] and analyzed['inputs'][0]['next'] == 'updater' and analyzed['next'] == 'updater'

        # After update with wallet, only needs signing
        updated = self.nodes[1].walletprocesspsbt(psbt, False, 'ALL', True)['psbt']
        analyzed = self.nodes[0].analyzepsbt(updated)
        assert analyzed['inputs'][0]['has_utxo'] and not analyzed['inputs'][0]['is_final'] and analyzed['inputs'][0]['next'] == 'signer' and analyzed['next'] == 'signer' and analyzed['inputs'][0]['missing']['signatures'][0] == addrinfo['embedded']['witness_program']

        # Check fee and size things
        assert analyzed['fee'] == Decimal('0.001') and analyzed['estimated_vsize'] == 134 and analyzed['estimated_feerate'] == Decimal('0.00746268')

        # After signing and finalizing, needs extracting
        signed = self.nodes[1].walletprocesspsbt(updated)['psbt']
        analyzed = self.nodes[0].analyzepsbt(signed)
        assert analyzed['inputs'][0]['has_utxo'] and analyzed['inputs'][0]['is_final'] and analyzed['next'] == 'extractor'

        self.log.info("PSBT spending unspendable outputs should have error message and Creator as next")
        analysis = self.nodes[0].analyzepsbt('cHNidP8BAJoCAAAAAljoeiG1ba8MI76OcHBFbDNvfLqlyHV5JPVFiHuyq911AAAAAAD/////g40EJ9DsZQpoqka7CwmK6kQiwHGyyng1Kgd5WdB86h0BAAAAAP////8CcKrwCAAAAAAWAEHYXCtx0AYLCcmIauuBXlCZHdoSTQDh9QUAAAAAFv8/wADXYP/7//////8JxOh0LR2HAI8AAAAAAAEBIADC6wsAAAAAF2oUt/X69ELjeX2nTof+fZ10l+OyAokDAQcJAwEHEAABAACAAAEBIADC6wsAAAAAF2oUt/X69ELjeX2nTof+fZ10l+OyAokDAQcJAwEHENkMak8AAAAA')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PSBT is not valid. Input 0 spends unspendable output')

        self.log.info("PSBT with invalid values should have error message and Creator as next")
        analysis = self.nodes[0].analyzepsbt('cHNidP8BAHECAAAAAfA00BFgAm6tp86RowwH6BMImQNL5zXUcTT97XoLGz0BAAAAAAD/////AgD5ApUAAAAAFgAUKNw0x8HRctAgmvoevm4u1SbN7XL87QKVAAAAABYAFPck4gF7iL4NL4wtfRAKgQbghiTUAAAAAAABAR8AgIFq49AHABYAFJUDtxf2PHo641HEOBOAIvFMNTr2AAAA')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PSBT is not valid. Input 0 has invalid value')

        self.log.info("PSBT with signed, but not finalized, inputs should have Finalizer as next")
        analysis = self.nodes[0].analyzepsbt('cHNidP8BAHECAAAAAZYezcxdnbXoQCmrD79t/LzDgtUo9ERqixk8wgioAobrAAAAAAD9////AlDDAAAAAAAAFgAUy/UxxZuzZswcmFnN/E9DGSiHLUsuGPUFAAAAABYAFLsH5o0R38wXx+X2cCosTMCZnQ4baAAAAAABAR8A4fUFAAAAABYAFOBI2h5thf3+Lflb2LGCsVSZwsltIgIC/i4dtVARCRWtROG0HHoGcaVklzJUcwo5homgGkSNAnJHMEQCIGx7zKcMIGr7cEES9BR4Kdt/pzPTK3fKWcGyCJXb7MVnAiALOBgqlMH4GbC1HDh/HmylmO54fyEy4lKde7/BT/PWxwEBAwQBAAAAIgYC/i4dtVARCRWtROG0HHoGcaVklzJUcwo5homgGkSNAnIYDwVpQ1QAAIABAACAAAAAgAAAAAAAAAAAAAAiAgL+CIiB59NSCssOJRGiMYQK1chahgAaaJpIXE41Cyir+xgPBWlDVAAAgAEAAIAAAACAAQAAAAAAAAAA')
        assert_equal(analysis['next'], 'finalizer')

        analysis = self.nodes[0].analyzepsbt('cHNidP8BAHECAAAAAfA00BFgAm6tp86RowwH6BMImQNL5zXUcTT97XoLGz0BAAAAAAD/////AgCAgWrj0AcAFgAUKNw0x8HRctAgmvoevm4u1SbN7XL87QKVAAAAABYAFPck4gF7iL4NL4wtfRAKgQbghiTUAAAAAAABAR8A8gUqAQAAABYAFJUDtxf2PHo641HEOBOAIvFMNTr2AAAA')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PSBT is not valid. Output amount invalid')

        analysis = self.nodes[0].analyzepsbt('cHNidP8BAJoCAAAAAkvEW8NnDtdNtDpsmze+Ht2LH35IJcKv00jKAlUs21RrAwAAAAD/////S8Rbw2cO1020OmybN74e3Ysffkglwq/TSMoCVSzbVGsBAAAAAP7///8CwLYClQAAAAAWABSNJKzjaUb3uOxixsvh1GGE3fW7zQD5ApUAAAAAFgAUKNw0x8HRctAgmvoevm4u1SbN7XIAAAAAAAEAnQIAAAACczMa321tVHuN4GKWKRncycI22aX3uXgwSFUKM2orjRsBAAAAAP7///9zMxrfbW1Ue43gYpYpGdzJwjbZpfe5eDBIVQozaiuNGwAAAAAA/v///wIA+QKVAAAAABl2qRT9zXUVA8Ls5iVqynLHe5/vSe1XyYisQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAAAAAQEfQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAA==')
        assert_equal(analysis['next'], 'creator')
        assert_equal(analysis['error'], 'PSBT is not valid. Input 0 specifies invalid prevout')

        assert_raises_rpc_error(-25, 'Inputs missing or spent', self.nodes[0].walletprocesspsbt, 'cHNidP8BAJoCAAAAAkvEW8NnDtdNtDpsmze+Ht2LH35IJcKv00jKAlUs21RrAwAAAAD/////S8Rbw2cO1020OmybN74e3Ysffkglwq/TSMoCVSzbVGsBAAAAAP7///8CwLYClQAAAAAWABSNJKzjaUb3uOxixsvh1GGE3fW7zQD5ApUAAAAAFgAUKNw0x8HRctAgmvoevm4u1SbN7XIAAAAAAAEAnQIAAAACczMa321tVHuN4GKWKRncycI22aX3uXgwSFUKM2orjRsBAAAAAP7///9zMxrfbW1Ue43gYpYpGdzJwjbZpfe5eDBIVQozaiuNGwAAAAAA/v///wIA+QKVAAAAABl2qRT9zXUVA8Ls5iVqynLHe5/vSe1XyYisQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAAAAAQEfQM0ClQAAAAAWABRmWQUcjSjghQ8/uH4Bn/zkakwLtAAAAA==')

        self.log.info("Test that we can fund psbts with external inputs specified")

        privkey, _ = generate_keypair(wif=True)

        self.nodes[1].createwallet("extfund")
        wallet = self.nodes[1].get_wallet_rpc("extfund")

        # Make a weird but signable script. sh(wsh(pkh())) descriptor accomplishes this
        desc = descsum_create("sh(wsh(pkh({})))".format(privkey))
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
        assert_raises_rpc_error(-4, "Not solvable pre-selected input COutPoint(%s, %s)" % (ext_utxo["txid"][0:10], ext_utxo["vout"]), wallet.walletcreatefundedpsbt, [ext_utxo], {self.nodes[0].getnewaddress(): 15})

        # But funding should work when the solving data is provided
        psbt = wallet.walletcreatefundedpsbt([ext_utxo], {self.nodes[0].getnewaddress(): 15}, 0, {"add_inputs": True, "solving_data": {"pubkeys": [addr_info['pubkey']], "scripts": [addr_info["embedded"]["scriptPubKey"], addr_info["embedded"]["embedded"]["scriptPubKey"]]}})
        signed = wallet.walletprocesspsbt(psbt['psbt'])
        assert not signed['complete']
        signed = self.nodes[0].walletprocesspsbt(signed['psbt'])
        assert signed['complete']

        psbt = wallet.walletcreatefundedpsbt([ext_utxo], {self.nodes[0].getnewaddress(): 15}, 0, {"add_inputs": True, "solving_data":{"descriptors": [desc]}})
        signed = wallet.walletprocesspsbt(psbt['psbt'])
        assert not signed['complete']
        signed = self.nodes[0].walletprocesspsbt(signed['psbt'])
        assert signed['complete']
        final = signed['hex']

        dec = self.nodes[0].decodepsbt(signed["psbt"])
        for i, txin in enumerate(dec["tx"]["vin"]):
            if txin["txid"] == ext_utxo["txid"] and txin["vout"] == ext_utxo["vout"]:
                input_idx = i
                break
        psbt_in = dec["inputs"][input_idx]
        scriptsig_hex = psbt_in["final_scriptSig"]["hex"] if "final_scriptSig" in psbt_in else ""
        witness_stack_hex = psbt_in["final_scriptwitness"] if "final_scriptwitness" in psbt_in else None
        input_weight = calculate_input_weight(scriptsig_hex, witness_stack_hex)
        low_input_weight = input_weight // 2
        high_input_weight = input_weight * 2

        # Input weight error conditions
        assert_raises_rpc_error(
            -8,
            "Input weights should be specified in inputs rather than in options.",
            wallet.walletcreatefundedpsbt,
            inputs=[ext_utxo],
            outputs={self.nodes[0].getnewaddress(): 15},
            options={"input_weights": [{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 1000}]}
        )

        # Funding should also work if the input weight is provided
        psbt = wallet.walletcreatefundedpsbt(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        signed = wallet.walletprocesspsbt(psbt["psbt"])
        signed = self.nodes[0].walletprocesspsbt(signed["psbt"])
        final = signed["hex"]
        assert self.nodes[0].testmempoolaccept([final])[0]["allowed"]
        # Reducing the weight should have a lower fee
        psbt2 = wallet.walletcreatefundedpsbt(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": low_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        assert_greater_than(psbt["fee"], psbt2["fee"])
        # Increasing the weight should have a higher fee
        psbt2 = wallet.walletcreatefundedpsbt(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        assert_greater_than(psbt2["fee"], psbt["fee"])
        # The provided weight should override the calculated weight when solving data is provided
        psbt3 = wallet.walletcreatefundedpsbt(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True, solving_data={"descriptors": [desc]},
        )
        assert_equal(psbt2["fee"], psbt3["fee"])

        # Import the external utxo descriptor so that we can sign for it from the test wallet
        if self.options.descriptors:
            res = wallet.importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = wallet.importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        # The provided weight should override the calculated weight for a wallet input
        psbt3 = wallet.walletcreatefundedpsbt(
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": high_input_weight}],
            outputs={self.nodes[0].getnewaddress(): 15},
            add_inputs=True,
        )
        assert_equal(psbt2["fee"], psbt3["fee"])

        self.log.info("Test signing inputs that the wallet has keys for but is not watching the scripts")
        self.nodes[1].createwallet(wallet_name="scriptwatchonly", disable_private_keys=True)
        watchonly = self.nodes[1].get_wallet_rpc("scriptwatchonly")

        privkey, pubkey = generate_keypair(wif=True)

        desc = descsum_create("wsh(pkh({}))".format(pubkey.hex()))
        if self.options.descriptors:
            res = watchonly.importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = watchonly.importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        addr = self.nodes[0].deriveaddresses(desc)[0]
        self.nodes[0].sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)
        self.nodes[0].importprivkey(privkey)

        psbt = watchonly.sendall([wallet.getnewaddress()])["psbt"]
        signed_tx = self.nodes[0].walletprocesspsbt(psbt)
        self.nodes[0].sendrawtransaction(signed_tx["hex"])

        # Same test but for taproot
        if self.options.descriptors:
            privkey, pubkey = generate_keypair(wif=True)

            desc = descsum_create("tr({},pk({}))".format(H_POINT, pubkey.hex()))
            res = watchonly.importdescriptors([{"desc": desc, "timestamp": "now"}])
            assert res[0]["success"]
            addr = self.nodes[0].deriveaddresses(desc)[0]
            self.nodes[0].sendtoaddress(addr, 10)
            self.generate(self.nodes[0], 1)
            self.nodes[0].importdescriptors([{"desc": descsum_create("tr({})".format(privkey)), "timestamp":"now"}])

            psbt = watchonly.sendall([wallet.getnewaddress(), addr])["psbt"]
            processed_psbt = self.nodes[0].walletprocesspsbt(psbt)
            txid = self.nodes[0].sendrawtransaction(processed_psbt["hex"])
            vout = find_vout_for_address(self.nodes[0], txid, addr)

            # Make sure tap tree is in psbt
            parsed_psbt = PSBT.from_base64(psbt)
            assert_greater_than(len(parsed_psbt.o[vout].map[PSBT_OUT_TAP_TREE]), 0)
            assert "taproot_tree" in self.nodes[0].decodepsbt(psbt)["outputs"][vout]
            parsed_psbt.make_blank()
            comb_psbt = self.nodes[0].combinepsbt([psbt, parsed_psbt.to_base64()])
            assert_equal(comb_psbt, psbt)

            self.log.info("Test that walletprocesspsbt both updates and signs a non-updated psbt containing Taproot inputs")
            addr = self.nodes[0].getnewaddress("", "bech32m")
            utxo = self.create_outpoints(self.nodes[0], outputs=[{addr: 1}])[0]
            psbt = self.nodes[0].createpsbt([utxo], [{self.nodes[0].getnewaddress(): 0.9999}])
            signed = self.nodes[0].walletprocesspsbt(psbt)
            rawtx = signed["hex"]
            self.nodes[0].sendrawtransaction(rawtx)
            self.generate(self.nodes[0], 1)

            # Make sure tap tree is not in psbt
            parsed_psbt = PSBT.from_base64(psbt)
            assert PSBT_OUT_TAP_TREE not in parsed_psbt.o[0].map
            assert "taproot_tree" not in self.nodes[0].decodepsbt(psbt)["outputs"][0]
            parsed_psbt.make_blank()
            comb_psbt = self.nodes[0].combinepsbt([psbt, parsed_psbt.to_base64()])
            assert_equal(comb_psbt, psbt)

        self.log.info("Test walletprocesspsbt raises if an invalid sighashtype is passed")
        assert_raises_rpc_error(-8, "'all' is not a valid sighash parameter.", self.nodes[0].walletprocesspsbt, psbt, sighashtype="all")

        self.log.info("Test decoding PSBT with per-input preimage types")
        # note that the decodepsbt RPC doesn't check whether preimages and hashes match
        hash_ripemd160, preimage_ripemd160 = randbytes(20), randbytes(50)
        hash_sha256, preimage_sha256 = randbytes(32), randbytes(50)
        hash_hash160, preimage_hash160 = randbytes(20), randbytes(50)
        hash_hash256, preimage_hash256 = randbytes(32), randbytes(50)

        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(hash=int('aa' * 32, 16), n=0), scriptSig=b""),
                  CTxIn(outpoint=COutPoint(hash=int('bb' * 32, 16), n=0), scriptSig=b""),
                  CTxIn(outpoint=COutPoint(hash=int('cc' * 32, 16), n=0), scriptSig=b""),
                  CTxIn(outpoint=COutPoint(hash=int('dd' * 32, 16), n=0), scriptSig=b"")]
        tx.vout = [CTxOut(nValue=0, scriptPubKey=b"")]
        psbt = PSBT()
        psbt.g = PSBTMap({PSBT_GLOBAL_UNSIGNED_TX: tx.serialize()})
        psbt.i = [PSBTMap({bytes([PSBT_IN_RIPEMD160]) + hash_ripemd160: preimage_ripemd160}),
                  PSBTMap({bytes([PSBT_IN_SHA256]) + hash_sha256: preimage_sha256}),
                  PSBTMap({bytes([PSBT_IN_HASH160]) + hash_hash160: preimage_hash160}),
                  PSBTMap({bytes([PSBT_IN_HASH256]) + hash_hash256: preimage_hash256})]
        psbt.o = [PSBTMap()]
        res_inputs = self.nodes[0].decodepsbt(psbt.to_base64())["inputs"]
        assert_equal(len(res_inputs), 4)
        preimage_keys = ["ripemd160_preimages", "sha256_preimages", "hash160_preimages", "hash256_preimages"]
        expected_hashes = [hash_ripemd160, hash_sha256, hash_hash160, hash_hash256]
        expected_preimages = [preimage_ripemd160, preimage_sha256, preimage_hash160, preimage_hash256]
        for res_input, preimage_key, hash, preimage in zip(res_inputs, preimage_keys, expected_hashes, expected_preimages):
            assert preimage_key in res_input
            assert_equal(len(res_input[preimage_key]), 1)
            assert hash.hex() in res_input[preimage_key]
            assert_equal(res_input[preimage_key][hash.hex()], preimage.hex())

        self.log.info("Test that combining PSBTs with different transactions fails")
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(hash=int('aa' * 32, 16), n=0), scriptSig=b"")]
        tx.vout = [CTxOut(nValue=0, scriptPubKey=b"")]
        psbt1 = PSBT(g=PSBTMap({PSBT_GLOBAL_UNSIGNED_TX: tx.serialize()}), i=[PSBTMap()], o=[PSBTMap()]).to_base64()
        tx.vout[0].nValue += 1  # slightly modify tx
        psbt2 = PSBT(g=PSBTMap({PSBT_GLOBAL_UNSIGNED_TX: tx.serialize()}), i=[PSBTMap()], o=[PSBTMap()]).to_base64()
        assert_raises_rpc_error(-8, "PSBTs not compatible (different transactions)", self.nodes[0].combinepsbt, [psbt1, psbt2])
        assert_equal(self.nodes[0].combinepsbt([psbt1, psbt1]), psbt1)

        self.log.info("Test that PSBT inputs are being checked via script execution")
        acs_prevout = CTxOut(nValue=0, scriptPubKey=CScript([OP_TRUE]))
        tx = CTransaction()
        tx.vin = [CTxIn(outpoint=COutPoint(hash=int('dd' * 32, 16), n=0), scriptSig=b"")]
        tx.vout = [CTxOut(nValue=0, scriptPubKey=b"")]
        psbt = PSBT()
        psbt.g = PSBTMap({PSBT_GLOBAL_UNSIGNED_TX: tx.serialize()})
        psbt.i = [PSBTMap({bytes([PSBT_IN_WITNESS_UTXO]) : acs_prevout.serialize()})]
        psbt.o = [PSBTMap()]
        assert_equal(self.nodes[0].finalizepsbt(psbt.to_base64()),
            {'hex': '0200000001dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd0000000000000000000100000000000000000000000000', 'complete': True})

        self.log.info("Test we don't crash when making a 0-value funded transaction at 0 fee without forcing an input selection")
        assert_raises_rpc_error(-4, "Transaction requires one destination of non-0 value, a non-0 feerate, or a pre-selected input", self.nodes[0].walletcreatefundedpsbt, [], [{"data": "deadbeef"}], 0, {"fee_rate": "0"})

        self.log.info("Test descriptorprocesspsbt updates and signs a psbt with descriptors")

        self.generate(self.nodes[2], 1)

        # Disable the wallet for node 2 since `descriptorprocesspsbt` does not use the wallet
        self.restart_node(2, extra_args=["-disablewallet"])
        self.connect_nodes(0, 2)
        self.connect_nodes(1, 2)

        key_info = get_generate_key()
        key = key_info.privkey
        address = key_info.p2wpkh_addr

        descriptor = descsum_create(f"wpkh({key})")

        utxo = self.create_outpoints(self.nodes[0], outputs=[{address: 1}])[0]
        self.sync_all()

        psbt = self.nodes[2].createpsbt([utxo], {self.nodes[0].getnewaddress(): 0.99999})
        decoded = self.nodes[2].decodepsbt(psbt)
        test_psbt_input_keys(decoded['inputs'][0], [])

        # Test that even if the wrong descriptor is given, `witness_utxo` and `non_witness_utxo`
        # are still added to the psbt
        alt_descriptor = descsum_create(f"wpkh({get_generate_key().privkey})")
        alt_psbt = self.nodes[2].descriptorprocesspsbt(psbt=psbt, descriptors=[alt_descriptor], sighashtype="ALL")["psbt"]
        decoded = self.nodes[2].decodepsbt(alt_psbt)
        test_psbt_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo'])

        # Test that the psbt is not finalized and does not have bip32_derivs unless specified
        processed_psbt = self.nodes[2].descriptorprocesspsbt(psbt=psbt, descriptors=[descriptor], sighashtype="ALL", bip32derivs=True, finalize=False)
        decoded = self.nodes[2].decodepsbt(processed_psbt['psbt'])
        test_psbt_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo', 'partial_signatures', 'bip32_derivs'])

        # If psbt not finalized, test that result does not have hex
        assert "hex" not in processed_psbt

        processed_psbt = self.nodes[2].descriptorprocesspsbt(psbt=psbt, descriptors=[descriptor], sighashtype="ALL", bip32derivs=False, finalize=True)
        decoded = self.nodes[2].decodepsbt(processed_psbt['psbt'])
        test_psbt_input_keys(decoded['inputs'][0], ['witness_utxo', 'non_witness_utxo', 'final_scriptwitness'])

        # Test psbt is complete
        assert_equal(processed_psbt['complete'], True)

        # Broadcast transaction
        self.nodes[2].sendrawtransaction(processed_psbt['hex'])

        self.log.info("Test descriptorprocesspsbt raises if an invalid sighashtype is passed")
        assert_raises_rpc_error(-8, "'all' is not a valid sighash parameter.", self.nodes[2].descriptorprocesspsbt, psbt, [descriptor], sighashtype="all")


if __name__ == '__main__':
    PSBTTest().main()
