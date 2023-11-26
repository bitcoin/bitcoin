#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the send RPC command."""

from decimal import Decimal, getcontext
from itertools import product

from test_framework.authproxy import JSONRPCException
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_fee_amount,
    assert_greater_than,
    assert_raises_rpc_error,
    count_bytes,
)
from test_framework.wallet_util import (
    calculate_input_weight,
    generate_keypair,
)


class WalletSendTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 2
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [
            ["-walletrbf=1"],
            ["-walletrbf=1"]
        ]
        getcontext().prec = 8 # Satoshi precision for Decimal

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_send(self, from_wallet, to_wallet=None, amount=None, data=None,
                  arg_conf_target=None, arg_estimate_mode=None, arg_fee_rate=None,
                  conf_target=None, estimate_mode=None, fee_rate=None, add_to_wallet=None, psbt=None,
                  inputs=None, add_inputs=None, include_unsafe=None, change_address=None, change_position=None, change_type=None,
                  include_watching=None, locktime=None, lock_unspents=None, replaceable=None, subtract_fee_from_outputs=None,
                  expect_error=None, solving_data=None, minconf=None):
        assert (amount is None) != (data is None)

        from_balance_before = from_wallet.getbalances()["mine"]["trusted"]
        if include_unsafe:
            from_balance_before += from_wallet.getbalances()["mine"]["untrusted_pending"]

        if to_wallet is None:
            assert amount is None
        else:
            to_untrusted_pending_before = to_wallet.getbalances()["mine"]["untrusted_pending"]

        if amount:
            dest = to_wallet.getnewaddress()
            outputs = {dest: amount}
        else:
            outputs = {"data": data}

        # Construct options dictionary
        options = {}
        if add_to_wallet is not None:
            options["add_to_wallet"] = add_to_wallet
        else:
            if psbt:
                add_to_wallet = False
            else:
                add_to_wallet = from_wallet.getwalletinfo()["private_keys_enabled"] # Default value
        if psbt is not None:
            options["psbt"] = psbt
        if conf_target is not None:
            options["conf_target"] = conf_target
        if estimate_mode is not None:
            options["estimate_mode"] = estimate_mode
        if fee_rate is not None:
            options["fee_rate"] = fee_rate
        if inputs is not None:
            options["inputs"] = inputs
        if add_inputs is not None:
            options["add_inputs"] = add_inputs
        if include_unsafe is not None:
            options["include_unsafe"] = include_unsafe
        if change_address is not None:
            options["change_address"] = change_address
        if change_position is not None:
            options["change_position"] = change_position
        if change_type is not None:
            options["change_type"] = change_type
        if include_watching is not None:
            options["include_watching"] = include_watching
        if locktime is not None:
            options["locktime"] = locktime
        if lock_unspents is not None:
            options["lock_unspents"] = lock_unspents
        if replaceable is None:
            replaceable = True # default
        else:
            options["replaceable"] = replaceable
        if subtract_fee_from_outputs is not None:
            options["subtract_fee_from_outputs"] = subtract_fee_from_outputs
        if solving_data is not None:
            options["solving_data"] = solving_data
        if minconf is not None:
            options["minconf"] = minconf

        if len(options.keys()) == 0:
            options = None

        if expect_error is None:
            res = from_wallet.send(outputs=outputs, conf_target=arg_conf_target, estimate_mode=arg_estimate_mode, fee_rate=arg_fee_rate, options=options)
        else:
            try:
                assert_raises_rpc_error(expect_error[0], expect_error[1], from_wallet.send,
                    outputs=outputs, conf_target=arg_conf_target, estimate_mode=arg_estimate_mode, fee_rate=arg_fee_rate, options=options)
            except AssertionError:
                # Provide debug info if the test fails
                self.log.error("Unexpected successful result:")
                self.log.error(arg_conf_target)
                self.log.error(arg_estimate_mode)
                self.log.error(arg_fee_rate)
                self.log.error(options)
                res = from_wallet.send(outputs=outputs, conf_target=arg_conf_target, estimate_mode=arg_estimate_mode, fee_rate=arg_fee_rate, options=options)
                self.log.error(res)
                if "txid" in res and add_to_wallet:
                    self.log.error("Transaction details:")
                    try:
                        tx = from_wallet.gettransaction(res["txid"])
                        self.log.error(tx)
                        self.log.error("testmempoolaccept (transaction may already be in mempool):")
                        self.log.error(from_wallet.testmempoolaccept([tx["hex"]]))
                    except JSONRPCException as exc:
                        self.log.error(exc)

                raise

            return

        if locktime:
            assert_equal(from_wallet.gettransaction(txid=res["txid"], verbose=True)["decoded"]["locktime"], locktime)
            return res
        else:
            if add_to_wallet:
                decoded_tx = from_wallet.gettransaction(txid=res["txid"], verbose=True)["decoded"]
                assert_greater_than(decoded_tx["locktime"], from_wallet.getblockcount() - 100)

        if from_wallet.getwalletinfo()["private_keys_enabled"] and not include_watching:
            assert_equal(res["complete"], True)
            assert "txid" in res
        else:
            assert_equal(res["complete"], False)
            assert not "txid" in res
            assert "psbt" in res

        from_balance = from_wallet.getbalances()["mine"]["trusted"]
        if include_unsafe:
            from_balance += from_wallet.getbalances()["mine"]["untrusted_pending"]

        if add_to_wallet and not include_watching:
            # Ensure transaction exists in the wallet:
            tx = from_wallet.gettransaction(res["txid"])
            assert tx
            assert_equal(tx["bip125-replaceable"], "yes" if replaceable else "no")
            # Ensure transaction exists in the mempool:
            tx = from_wallet.getrawtransaction(res["txid"], True)
            assert tx
            if amount:
                if subtract_fee_from_outputs:
                    assert_equal(from_balance_before - from_balance, amount)
                else:
                    assert_greater_than(from_balance_before - from_balance, amount)
            else:
                assert next((out for out in tx["vout"] if out["scriptPubKey"]["asm"] == "OP_RETURN 35"), None)
        else:
            assert_equal(from_balance_before, from_balance)

        if to_wallet:
            self.sync_mempools()
            if add_to_wallet:
                if not subtract_fee_from_outputs:
                    assert_equal(to_wallet.getbalances()["mine"]["untrusted_pending"], to_untrusted_pending_before + Decimal(amount if amount else 0))
            else:
                assert_equal(to_wallet.getbalances()["mine"]["untrusted_pending"], to_untrusted_pending_before)

        return res

    def run_test(self):
        self.log.info("Setup wallets...")
        # w0 is a wallet with coinbase rewards
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        # w1 is a regular wallet
        self.nodes[1].createwallet(wallet_name="w1")
        w1 = self.nodes[1].get_wallet_rpc("w1")
        # w2 contains the private keys for w3
        self.nodes[1].createwallet(wallet_name="w2", blank=True)
        w2 = self.nodes[1].get_wallet_rpc("w2")
        xpriv = "tprv8ZgxMBicQKsPfHCsTwkiM1KT56RXbGGTqvc2hgqzycpwbHqqpcajQeMRZoBD35kW4RtyCemu6j34Ku5DEspmgjKdt2qe4SvRch5Kk8B8A2v"
        xpub = "tpubD6NzVbkrYhZ4YkEfMbRJkQyZe7wTkbTNRECozCtJPtdLRn6cT1QKb8yHjwAPcAr26eHBFYs5iLiFFnCbwPRsncCKUKCfubHDMGKzMVcN1Jg"
        if self.options.descriptors:
            w2.importdescriptors([{
                "desc": descsum_create("wpkh(" + xpriv + "/0/0/*)"),
                "timestamp": "now",
                "range": [0, 100],
                "active": True
            },{
                "desc": descsum_create("wpkh(" + xpriv + "/0/1/*)"),
                "timestamp": "now",
                "range": [0, 100],
                "active": True,
                "internal": True
            }])
        else:
            w2.sethdseed(True)

        # w3 is a watch-only wallet, based on w2
        self.nodes[1].createwallet(wallet_name="w3", disable_private_keys=True)
        w3 = self.nodes[1].get_wallet_rpc("w3")
        if self.options.descriptors:
            # Match the privkeys in w2 for descriptors
            res = w3.importdescriptors([{
                "desc": descsum_create("wpkh(" + xpub + "/0/0/*)"),
                "timestamp": "now",
                "range": [0, 100],
                "keypool": True,
                "active": True,
                "watchonly": True
            },{
                "desc": descsum_create("wpkh(" + xpub + "/0/1/*)"),
                "timestamp": "now",
                "range": [0, 100],
                "keypool": True,
                "active": True,
                "internal": True,
                "watchonly": True
            }])
            assert_equal(res, [{"success": True}, {"success": True}])

        for _ in range(3):
            a2_receive = w2.getnewaddress()
            if not self.options.descriptors:
                # Because legacy wallets use exclusively hardened derivation, we can't do a ranged import like we do for descriptors
                a2_change = w2.getrawchangeaddress() # doesn't actually use change derivation
                res = w3.importmulti([{
                    "desc": w2.getaddressinfo(a2_receive)["desc"],
                    "timestamp": "now",
                    "keypool": True,
                    "watchonly": True
                },{
                    "desc": w2.getaddressinfo(a2_change)["desc"],
                    "timestamp": "now",
                    "keypool": True,
                    "internal": True,
                    "watchonly": True
                }])
                assert_equal(res, [{"success": True}, {"success": True}])

        w0.sendtoaddress(a2_receive, 10) # fund w3
        self.generate(self.nodes[0], 1)

        if not self.options.descriptors:
            # w4 has private keys enabled, but only contains watch-only keys (from w2)
            # This is legacy wallet behavior only as descriptor wallets don't allow watchonly and non-watchonly things in the same wallet.
            self.nodes[1].createwallet(wallet_name="w4", disable_private_keys=False)
            w4 = self.nodes[1].get_wallet_rpc("w4")
            for _ in range(3):
                a2_receive = w2.getnewaddress()
                res = w4.importmulti([{
                    "desc": w2.getaddressinfo(a2_receive)["desc"],
                    "timestamp": "now",
                    "keypool": False,
                    "watchonly": True
                }])
                assert_equal(res, [{"success": True}])

            w0.sendtoaddress(a2_receive, 10) # fund w4
            self.generate(self.nodes[0], 1)

        self.log.info("Send to address...")
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1)
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=True)

        self.log.info("Don't broadcast...")
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=False)
        assert res["hex"]

        self.log.info("Return PSBT...")
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, psbt=True)
        assert res["psbt"]

        self.log.info("Create transaction that spends to address, but don't broadcast...")
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=False)
        # conf_target & estimate_mode can be set as argument or option
        res1 = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_conf_target=1, arg_estimate_mode="economical", add_to_wallet=False)
        res2 = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, conf_target=1, estimate_mode="economical", add_to_wallet=False)
        assert_equal(self.nodes[1].decodepsbt(res1["psbt"])["fee"],
                     self.nodes[1].decodepsbt(res2["psbt"])["fee"])
        # but not at the same time
        for mode in ["unset", "economical", "conservative"]:
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_conf_target=1, arg_estimate_mode="economical",
                conf_target=1, estimate_mode=mode, add_to_wallet=False,
                expect_error=(-8, "Pass conf_target and estimate_mode either as arguments or in the options object, but not both"))

        self.log.info("Create PSBT from watch-only wallet w3, sign with w2...")
        res = self.test_send(from_wallet=w3, to_wallet=w1, amount=1)
        res = w2.walletprocesspsbt(res["psbt"])
        assert res["complete"]

        if not self.options.descriptors:
            # Descriptor wallets do not allow mixed watch-only and non-watch-only things in the same wallet.
            # This is specifically testing that w4 ignores its own private keys and creates a psbt with send
            # which is not something that needs to be tested in descriptor wallets.
            self.log.info("Create PSBT from wallet w4 with watch-only keys, sign with w2...")
            self.test_send(from_wallet=w4, to_wallet=w1, amount=1, expect_error=(-4, "Insufficient funds"))
            res = self.test_send(from_wallet=w4, to_wallet=w1, amount=1, include_watching=True, add_to_wallet=False)
            res = w2.walletprocesspsbt(res["psbt"])
            assert res["complete"]

        self.log.info("Create OP_RETURN...")
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1)
        self.test_send(from_wallet=w0, data="Hello World", expect_error=(-8, "Data must be hexadecimal string (not 'Hello World')"))
        self.test_send(from_wallet=w0, data="23")
        res = self.test_send(from_wallet=w3, data="23")
        res = w2.walletprocesspsbt(res["psbt"])
        assert res["complete"]

        self.log.info("Test setting explicit fee rate")
        res1 = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate="1", add_to_wallet=False)
        res2 = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate="1", add_to_wallet=False)
        assert_equal(self.nodes[1].decodepsbt(res1["psbt"])["fee"], self.nodes[1].decodepsbt(res2["psbt"])["fee"])

        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=7, add_to_wallet=False)
        fee = self.nodes[1].decodepsbt(res["psbt"])["fee"]
        assert_fee_amount(fee, count_bytes(res["hex"]), Decimal("0.00007"))

        # "unset" and None are treated the same for estimate_mode
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=2, estimate_mode="unset", add_to_wallet=False)
        fee = self.nodes[1].decodepsbt(res["psbt"])["fee"]
        assert_fee_amount(fee, count_bytes(res["hex"]), Decimal("0.00002"))

        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=4.531, add_to_wallet=False)
        fee = self.nodes[1].decodepsbt(res["psbt"])["fee"]
        assert_fee_amount(fee, count_bytes(res["hex"]), Decimal("0.00004531"))

        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=3, add_to_wallet=False)
        fee = self.nodes[1].decodepsbt(res["psbt"])["fee"]
        assert_fee_amount(fee, count_bytes(res["hex"]), Decimal("0.00003"))

        # Test that passing fee_rate as both an argument and an option raises.
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=1, fee_rate=1, add_to_wallet=False,
                       expect_error=(-8, "Pass the fee_rate either as an argument, or in the options object, but not both"))

        assert_raises_rpc_error(-8, "Use fee_rate (sat/vB) instead of feeRate", w0.send, {w1.getnewaddress(): 1}, 6, "conservative", 1, {"feeRate": 0.01})

        assert_raises_rpc_error(-3, "Unexpected key totalFee", w0.send, {w1.getnewaddress(): 1}, 6, "conservative", 1, {"totalFee": 0.01})

        for target, mode in product([-1, 0, 1009], ["economical", "conservative"]):
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, conf_target=target, estimate_mode=mode,
                expect_error=(-8, "Invalid conf_target, must be between 1 and 1008"))  # max value of 1008 per src/policy/fees.h
        msg = 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"'
        for target, mode in product([-1, 0], ["btc/kb", "sat/b"]):
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, conf_target=target, estimate_mode=mode, expect_error=(-8, msg))
        for mode in ["", "foo", Decimal("3.141592")]:
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, conf_target=0.1, estimate_mode=mode, expect_error=(-8, msg))
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_conf_target=0.1, arg_estimate_mode=mode, expect_error=(-8, msg))
            assert_raises_rpc_error(-8, msg, w0.send, {w1.getnewaddress(): 1}, 0.1, mode)

        for mode in ["economical", "conservative"]:
            for k, v in {"string": "true", "bool": True, "object": {"foo": "bar"}}.items():
                self.test_send(from_wallet=w0, to_wallet=w1, amount=1, conf_target=v, estimate_mode=mode,
                    expect_error=(-3, f"JSON value of type {k} for field conf_target is not of expected type number"))

        # Test setting explicit fee rate just below the minimum of 1 sat/vB.
        self.log.info("Explicit fee rate raises RPC error 'fee rate too low' if fee_rate of 0.99999999 is passed")
        msg = "Fee rate (0.999 sat/vB) is lower than the minimum fee rate setting (1.000 sat/vB)"
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=0.999, expect_error=(-4, msg))
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=0.999, expect_error=(-4, msg))

        self.log.info("Explicit fee rate raises if invalid fee_rate is passed")
        # Test fee_rate with zero values.
        msg = "Fee rate (0.000 sat/vB) is lower than the minimum fee rate setting (1.000 sat/vB)"
        for zero_value in [0, 0.000, 0.00000000, "0", "0.000", "0.00000000"]:
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=zero_value, expect_error=(-4, msg))
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=zero_value, expect_error=(-4, msg))
        msg = "Invalid amount"
        # Test fee_rate values that don't pass fixed-point parsing checks.
        for invalid_value in ["", 0.000000001, 1e-09, 1.111111111, 1111111111111111, "31.999999999999999999999"]:
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=invalid_value, expect_error=(-3, msg))
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=invalid_value, expect_error=(-3, msg))
        # Test fee_rate values that cannot be represented in sat/vB.
        for invalid_value in [0.0001, 0.00000001, 0.00099999, 31.99999999]:
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=invalid_value, expect_error=(-3, msg))
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=invalid_value, expect_error=(-3, msg))
        # Test fee_rate out of range (negative number).
        msg = "Amount out of range"
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=-1, expect_error=(-3, msg))
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=-1, expect_error=(-3, msg))
        # Test type error.
        msg = "Amount is not a number or string"
        for invalid_value in [True, {"foo": "bar"}]:
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, fee_rate=invalid_value, expect_error=(-3, msg))
            self.test_send(from_wallet=w0, to_wallet=w1, amount=1, arg_fee_rate=invalid_value, expect_error=(-3, msg))

        # TODO: Return hex if fee rate is below -maxmempool
        # res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, conf_target=0.1, estimate_mode="sat/b", add_to_wallet=False)
        # assert res["hex"]
        # hex = res["hex"]
        # res = self.nodes[0].testmempoolaccept([hex])
        # assert not res[0]["allowed"]
        # assert_equal(res[0]["reject-reason"], "...") # low fee
        # assert_fee_amount(fee, Decimal(len(res["hex"]) / 2), Decimal("0.000001"))

        self.log.info("If inputs are specified, do not automatically add more...")
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=51, inputs=[], add_to_wallet=False)
        assert res["complete"]
        utxo1 = w0.listunspent()[0]
        assert_equal(utxo1["amount"], 50)
        ERR_NOT_ENOUGH_PRESET_INPUTS = "The preselected coins total amount does not cover the transaction target. " \
                                       "Please allow other inputs to be automatically selected or include more coins manually"
        self.test_send(from_wallet=w0, to_wallet=w1, amount=51, inputs=[utxo1],
                       expect_error=(-4, ERR_NOT_ENOUGH_PRESET_INPUTS))
        self.test_send(from_wallet=w0, to_wallet=w1, amount=51, inputs=[utxo1], add_inputs=False,
                       expect_error=(-4, ERR_NOT_ENOUGH_PRESET_INPUTS))
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=51, inputs=[utxo1], add_inputs=True, add_to_wallet=False)
        assert res["complete"]

        self.log.info("Manual change address and position...")
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, change_address="not an address",
                       expect_error=(-5, "Change address must be a valid bitcoin address"))
        change_address = w0.getnewaddress()
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=False, change_address=change_address)
        assert res["complete"]
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=False, change_address=change_address, change_position=0)
        assert res["complete"]
        assert_equal(self.nodes[0].decodepsbt(res["psbt"])["tx"]["vout"][0]["scriptPubKey"]["address"], change_address)
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=False, change_type="legacy", change_position=0)
        assert res["complete"]
        change_address = self.nodes[0].decodepsbt(res["psbt"])["tx"]["vout"][0]["scriptPubKey"]["address"]
        assert change_address[0] == "m" or change_address[0] == "n"

        self.log.info("Set lock time...")
        height = self.nodes[0].getblockchaininfo()["blocks"]
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, locktime=height + 1)
        assert res["complete"]
        assert res["txid"]
        txid = res["txid"]
        # Although the wallet finishes the transaction, it can't be added to the mempool yet:
        hex = self.nodes[0].gettransaction(res["txid"])["hex"]
        res = self.nodes[0].testmempoolaccept([hex])
        assert not res[0]["allowed"]
        assert_equal(res[0]["reject-reason"], "non-final")
        # It shouldn't be confirmed in the next block
        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].gettransaction(txid)["confirmations"], 0)
        # The mempool should allow it now:
        res = self.nodes[0].testmempoolaccept([hex])
        assert res[0]["allowed"]
        # Don't wait for wallet to add it to the mempool:
        res = self.nodes[0].sendrawtransaction(hex)
        self.generate(self.nodes[0], 1)
        assert_equal(self.nodes[0].gettransaction(txid)["confirmations"], 1)

        self.log.info("Lock unspents...")
        utxo1 = w0.listunspent()[0]
        assert_greater_than(utxo1["amount"], 1)
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, inputs=[utxo1], add_to_wallet=False, lock_unspents=True)
        assert res["complete"]
        locked_coins = w0.listlockunspent()
        assert_equal(len(locked_coins), 1)
        # Locked coins are automatically unlocked when manually selected
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, inputs=[utxo1], add_to_wallet=False)
        assert res["complete"]

        self.log.info("Replaceable...")
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=True, replaceable=True)
        assert res["complete"]
        assert_equal(self.nodes[0].gettransaction(res["txid"])["bip125-replaceable"], "yes")
        res = self.test_send(from_wallet=w0, to_wallet=w1, amount=1, add_to_wallet=True, replaceable=False)
        assert res["complete"]
        assert_equal(self.nodes[0].gettransaction(res["txid"])["bip125-replaceable"], "no")

        self.log.info("Subtract fee from output")
        self.test_send(from_wallet=w0, to_wallet=w1, amount=1, subtract_fee_from_outputs=[0])

        self.log.info("Include unsafe inputs")
        self.nodes[1].createwallet(wallet_name="w5")
        w5 = self.nodes[1].get_wallet_rpc("w5")
        self.test_send(from_wallet=w0, to_wallet=w5, amount=2)
        self.test_send(from_wallet=w5, to_wallet=w0, amount=1, expect_error=(-4, "Insufficient funds"))
        res = self.test_send(from_wallet=w5, to_wallet=w0, amount=1, include_unsafe=True)
        assert res["complete"]

        self.log.info("Minconf")
        self.nodes[1].createwallet(wallet_name="minconfw")
        minconfw= self.nodes[1].get_wallet_rpc("minconfw")
        self.test_send(from_wallet=w0, to_wallet=minconfw, amount=2)
        self.generate(self.nodes[0], 3)
        self.test_send(from_wallet=minconfw, to_wallet=w0, amount=1, minconf=4, expect_error=(-4, "Insufficient funds"))
        self.test_send(from_wallet=minconfw, to_wallet=w0, amount=1, minconf=-4, expect_error=(-8, "Negative minconf"))
        res = self.test_send(from_wallet=minconfw, to_wallet=w0, amount=1, minconf=3)
        assert res["complete"]

        self.log.info("External outputs")
        privkey, _ = generate_keypair(wif=True)

        self.nodes[1].createwallet("extsend")
        ext_wallet = self.nodes[1].get_wallet_rpc("extsend")
        self.nodes[1].createwallet("extfund")
        ext_fund = self.nodes[1].get_wallet_rpc("extfund")

        # Make a weird but signable script. sh(wsh(pkh())) descriptor accomplishes this
        desc = descsum_create("sh(wsh(pkh({})))".format(privkey))
        if self.options.descriptors:
            res = ext_fund.importdescriptors([{"desc": desc, "timestamp": "now"}])
        else:
            res = ext_fund.importmulti([{"desc": desc, "timestamp": "now"}])
        assert res[0]["success"]
        addr = self.nodes[0].deriveaddresses(desc)[0]
        addr_info = ext_fund.getaddressinfo(addr)

        self.nodes[0].sendtoaddress(addr, 10)
        self.nodes[0].sendtoaddress(ext_wallet.getnewaddress(), 10)
        self.generate(self.nodes[0], 6)
        ext_utxo = ext_fund.listunspent(addresses=[addr])[0]

        # An external input without solving data should result in an error
        self.test_send(from_wallet=ext_wallet, to_wallet=self.nodes[0], amount=15, inputs=[ext_utxo], add_inputs=True, psbt=True, include_watching=True, expect_error=(-4, "Not solvable pre-selected input COutPoint(%s, %s)" % (ext_utxo["txid"][0:10], ext_utxo["vout"])))

        # But funding should work when the solving data is provided
        res = self.test_send(from_wallet=ext_wallet, to_wallet=self.nodes[0], amount=15, inputs=[ext_utxo], add_inputs=True, psbt=True, include_watching=True, solving_data={"pubkeys": [addr_info['pubkey']], "scripts": [addr_info["embedded"]["scriptPubKey"], addr_info["embedded"]["embedded"]["scriptPubKey"]]})
        signed = ext_wallet.walletprocesspsbt(res["psbt"])
        signed = ext_fund.walletprocesspsbt(res["psbt"])
        assert signed["complete"]

        res = self.test_send(from_wallet=ext_wallet, to_wallet=self.nodes[0], amount=15, inputs=[ext_utxo], add_inputs=True, psbt=True, include_watching=True, solving_data={"descriptors": [desc]})
        signed = ext_wallet.walletprocesspsbt(res["psbt"])
        signed = ext_fund.walletprocesspsbt(res["psbt"])
        assert signed["complete"]

        dec = self.nodes[0].decodepsbt(signed["psbt"])
        for i, txin in enumerate(dec["tx"]["vin"]):
            if txin["txid"] == ext_utxo["txid"] and txin["vout"] == ext_utxo["vout"]:
                input_idx = i
                break
        psbt_in = dec["inputs"][input_idx]
        scriptsig_hex = psbt_in["final_scriptSig"]["hex"] if "final_scriptSig" in psbt_in else ""
        witness_stack_hex = psbt_in["final_scriptwitness"] if "final_scriptwitness" in psbt_in else None
        input_weight = calculate_input_weight(scriptsig_hex, witness_stack_hex)

        # Input weight error conditions
        assert_raises_rpc_error(
            -8,
            "Input weights should be specified in inputs rather than in options.",
            ext_wallet.send,
            outputs={self.nodes[0].getnewaddress(): 15},
            options={"inputs": [ext_utxo], "input_weights": [{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": 1000}]}
        )

        target_fee_rate_sat_vb = 10
        # Funding should also work when input weights are provided
        res = self.test_send(
            from_wallet=ext_wallet,
            to_wallet=self.nodes[0],
            amount=15,
            inputs=[{"txid": ext_utxo["txid"], "vout": ext_utxo["vout"], "weight": input_weight}],
            add_inputs=True,
            psbt=True,
            include_watching=True,
            fee_rate=target_fee_rate_sat_vb
        )
        signed = ext_wallet.walletprocesspsbt(res["psbt"])
        signed = ext_fund.walletprocesspsbt(res["psbt"])
        assert signed["complete"]
        testres = self.nodes[0].testmempoolaccept([signed["hex"]])[0]
        assert_equal(testres["allowed"], True)
        actual_fee_rate_sat_vb = Decimal(testres["fees"]["base"]) * Decimal(1e8) / Decimal(testres["vsize"])
        # Due to ECDSA signatures not always being the same length, the actual fee rate may be slightly different
        # but rounded to nearest integer, it should be the same as the target fee rate
        assert_equal(round(actual_fee_rate_sat_vb), target_fee_rate_sat_vb)

if __name__ == '__main__':
    WalletSendTest().main()
