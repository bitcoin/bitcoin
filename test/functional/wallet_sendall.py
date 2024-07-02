#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the sendall RPC command."""

from decimal import Decimal, getcontext

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)

# Decorator to reset activewallet to zero utxos
def cleanup(func):
    def wrapper(self):
        try:
            func(self)
        finally:
            if 0 < self.wallet.getbalances()["mine"]["trusted"]:
                self.wallet.sendall([self.remainder_target])
            assert_equal(0, self.wallet.getbalances()["mine"]["trusted"]) # wallet is empty
    return wrapper

class SendallTest(BitcoinTestFramework):
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def set_test_params(self):
        getcontext().prec=10
        self.num_nodes = 1
        self.setup_clean_chain = True

    def assert_balance_swept_completely(self, tx, balance):
        output_sum = sum([o["value"] for o in tx["decoded"]["vout"]])
        assert_equal(output_sum, balance + tx["fee"])
        assert_equal(0, self.wallet.getbalances()["mine"]["trusted"]) # wallet is empty

    def assert_tx_has_output(self, tx, addr, value=None):
        for output in tx["decoded"]["vout"]:
            if addr == output["scriptPubKey"]["address"] and value is None or value == output["value"]:
                return
        raise AssertionError("Output to {} not present or wrong amount".format(addr))

    def assert_tx_has_outputs(self, tx, expected_outputs):
        assert_equal(len(expected_outputs), len(tx["decoded"]["vout"]))
        for eo in expected_outputs:
            self.assert_tx_has_output(tx, eo["address"], eo["value"])

    def add_utxos(self, amounts):
        for a in amounts:
            self.def_wallet.sendtoaddress(self.wallet.getnewaddress(), a)
        self.generate(self.nodes[0], 1)
        assert_greater_than(self.wallet.getbalances()["mine"]["trusted"], 0)
        return self.wallet.getbalances()["mine"]["trusted"]

    # Helper schema for success cases
    def test_sendall_success(self, sendall_args, remaining_balance = 0):
        sendall_tx_receipt = self.wallet.sendall(sendall_args)
        self.generate(self.nodes[0], 1)
        # wallet has remaining balance (usually empty)
        assert_equal(remaining_balance, self.wallet.getbalances()["mine"]["trusted"])

        assert_equal(sendall_tx_receipt["complete"], True)
        return self.wallet.gettransaction(txid = sendall_tx_receipt["txid"], verbose = True)

    @cleanup
    def gen_and_clean(self):
        self.add_utxos([15, 2, 4])

    def test_cleanup(self):
        self.log.info("Test that cleanup wrapper empties wallet")
        self.gen_and_clean()
        assert_equal(0, self.wallet.getbalances()["mine"]["trusted"]) # wallet is empty

    # Actual tests
    @cleanup
    def sendall_two_utxos(self):
        self.log.info("Testing basic sendall case without specific amounts")
        pre_sendall_balance = self.add_utxos([10,11])
        tx_from_wallet = self.test_sendall_success(sendall_args = [self.remainder_target])

        self.assert_tx_has_outputs(tx = tx_from_wallet,
            expected_outputs = [
                { "address": self.remainder_target, "value": pre_sendall_balance + tx_from_wallet["fee"] } # fee is neg
            ]
        )
        self.assert_balance_swept_completely(tx_from_wallet, pre_sendall_balance)

    @cleanup
    def sendall_split(self):
        self.log.info("Testing sendall where two recipients have unspecified amount")
        pre_sendall_balance = self.add_utxos([1, 2, 3, 15])
        tx_from_wallet = self.test_sendall_success([self.remainder_target, self.split_target])

        half = (pre_sendall_balance + tx_from_wallet["fee"]) / 2
        self.assert_tx_has_outputs(tx_from_wallet,
            expected_outputs = [
                { "address": self.split_target, "value": half },
                { "address": self.remainder_target, "value": half }
            ]
        )
        self.assert_balance_swept_completely(tx_from_wallet, pre_sendall_balance)

    @cleanup
    def sendall_and_spend(self):
        self.log.info("Testing sendall in combination with paying specified amount to recipient")
        pre_sendall_balance = self.add_utxos([8, 13])
        tx_from_wallet = self.test_sendall_success([{self.recipient: 5}, self.remainder_target])

        self.assert_tx_has_outputs(tx_from_wallet,
            expected_outputs = [
                { "address": self.recipient, "value": 5 },
                { "address": self.remainder_target, "value": pre_sendall_balance - 5 + tx_from_wallet["fee"] }
            ]
        )
        self.assert_balance_swept_completely(tx_from_wallet, pre_sendall_balance)

    @cleanup
    def sendall_invalid_recipient_addresses(self):
        self.log.info("Test having only recipient with specified amount, missing recipient with unspecified amount")
        self.add_utxos([12, 9])

        assert_raises_rpc_error(
                -8,
                "Must provide at least one address without a specified amount" ,
                self.wallet.sendall,
                [{self.recipient: 5}]
            )

    @cleanup
    def sendall_duplicate_recipient(self):
        self.log.info("Test duplicate destination")
        self.add_utxos([1, 8, 3, 9])

        assert_raises_rpc_error(
                -8,
                "Invalid parameter, duplicated address: {}".format(self.remainder_target),
                self.wallet.sendall,
                [self.remainder_target, self.remainder_target]
            )

    @cleanup
    def sendall_invalid_amounts(self):
        self.log.info("Test sending more than balance")
        pre_sendall_balance = self.add_utxos([7, 14])

        expected_tx = self.wallet.sendall(recipients=[{self.recipient: 5}, self.remainder_target], add_to_wallet=False)
        tx = self.wallet.decoderawtransaction(expected_tx['hex'])
        fee = 21 - sum([o["value"] for o in tx["vout"]])

        assert_raises_rpc_error(-6, "Assigned more value to outputs than available funds.", self.wallet.sendall,
                [{self.recipient: pre_sendall_balance + 1}, self.remainder_target])
        assert_raises_rpc_error(-6, "Insufficient funds for fees after creating specified outputs.", self.wallet.sendall,
                [{self.recipient: pre_sendall_balance}, self.remainder_target])
        assert_raises_rpc_error(-8, "Specified output amount to {} is below dust threshold".format(self.recipient),
                self.wallet.sendall, [{self.recipient: 0.00000001}, self.remainder_target])
        assert_raises_rpc_error(-6, "Dynamically assigned remainder results in dust output.", self.wallet.sendall,
                [{self.recipient: pre_sendall_balance - fee}, self.remainder_target])
        assert_raises_rpc_error(-6, "Dynamically assigned remainder results in dust output.", self.wallet.sendall,
                [{self.recipient: pre_sendall_balance - fee - Decimal(0.00000010)}, self.remainder_target])

    # @cleanup not needed because different wallet used
    def sendall_negative_effective_value(self):
        self.log.info("Test that sendall fails if all UTXOs have negative effective value")
        # Use dedicated wallet for dust amounts and unload wallet at end
        self.nodes[0].createwallet("dustwallet")
        dust_wallet = self.nodes[0].get_wallet_rpc("dustwallet")

        self.def_wallet.sendtoaddress(dust_wallet.getnewaddress(), 0.00000400)
        self.def_wallet.sendtoaddress(dust_wallet.getnewaddress(), 0.00000300)
        self.generate(self.nodes[0], 1)
        assert_greater_than(dust_wallet.getbalances()["mine"]["trusted"], 0)

        assert_raises_rpc_error(-6, "Total value of UTXO pool too low to pay for transaction."
                + " Try using lower feerate or excluding uneconomic UTXOs with 'send_max' option.",
                dust_wallet.sendall, recipients=[self.remainder_target], fee_rate=300)

        dust_wallet.unloadwallet()

    @cleanup
    def sendall_with_send_max(self):
        self.log.info("Check that `send_max` option causes negative value UTXOs to be left behind")
        self.add_utxos([0.00000400, 0.00000300, 1])

        # sendall with send_max
        sendall_tx_receipt = self.wallet.sendall(recipients=[self.remainder_target], fee_rate=300, send_max=True)
        tx_from_wallet = self.wallet.gettransaction(txid = sendall_tx_receipt["txid"], verbose = True)

        assert_equal(len(tx_from_wallet["decoded"]["vin"]), 1)
        self.assert_tx_has_outputs(tx_from_wallet, [{"address": self.remainder_target, "value": 1 + tx_from_wallet["fee"]}])
        assert_equal(self.wallet.getbalances()["mine"]["trusted"], Decimal("0.00000700"))

        self.def_wallet.sendtoaddress(self.wallet.getnewaddress(), 1)
        self.generate(self.nodes[0], 1)

    @cleanup
    def sendall_specific_inputs(self):
        self.log.info("Test sendall with a subset of UTXO pool")
        self.add_utxos([17, 4])
        utxo = self.wallet.listunspent()[0]

        sendall_tx_receipt = self.wallet.sendall(recipients=[self.remainder_target], inputs=[utxo])
        tx_from_wallet = self.wallet.gettransaction(txid = sendall_tx_receipt["txid"], verbose = True)
        assert_equal(len(tx_from_wallet["decoded"]["vin"]), 1)
        assert_equal(len(tx_from_wallet["decoded"]["vout"]), 1)
        assert_equal(tx_from_wallet["decoded"]["vin"][0]["txid"], utxo["txid"])
        assert_equal(tx_from_wallet["decoded"]["vin"][0]["vout"], utxo["vout"])
        self.assert_tx_has_output(tx_from_wallet, self.remainder_target)

        self.generate(self.nodes[0], 1)
        assert_greater_than(self.wallet.getbalances()["mine"]["trusted"], 0)

    @cleanup
    def sendall_fails_on_missing_input(self):
        # fails because UTXO was previously spent, and wallet is empty
        self.log.info("Test sendall fails because specified UTXO is not available")
        self.add_utxos([16, 5])
        spent_utxo = self.wallet.listunspent()[0]

        # fails on out of bounds vout
        assert_raises_rpc_error(-8,
                "Input not found. UTXO ({}:{}) is not part of wallet.".format(spent_utxo["txid"], 1000),
                self.wallet.sendall, recipients=[self.remainder_target], inputs=[{"txid": spent_utxo["txid"], "vout": 1000}])

        # fails on unconfirmed spent UTXO
        self.wallet.sendall(recipients=[self.remainder_target])
        assert_raises_rpc_error(-8,
                "Input not available. UTXO ({}:{}) was already spent.".format(spent_utxo["txid"], spent_utxo["vout"]),
                self.wallet.sendall, recipients=[self.remainder_target], inputs=[spent_utxo])

        # fails on specific previously spent UTXO, while other UTXOs exist
        self.generate(self.nodes[0], 1)
        self.add_utxos([19, 2])
        assert_raises_rpc_error(-8,
                "Input not available. UTXO ({}:{}) was already spent.".format(spent_utxo["txid"], spent_utxo["vout"]),
                self.wallet.sendall, recipients=[self.remainder_target], inputs=[spent_utxo])

        # fails because UTXO is unknown, while other UTXOs exist
        foreign_utxo = self.def_wallet.listunspent()[0]
        assert_raises_rpc_error(-8, "Input not found. UTXO ({}:{}) is not part of wallet.".format(foreign_utxo["txid"],
            foreign_utxo["vout"]), self.wallet.sendall, recipients=[self.remainder_target],
            inputs=[foreign_utxo])

    @cleanup
    def sendall_fails_on_no_address(self):
        self.log.info("Test sendall fails because no address is provided")
        self.add_utxos([19, 2])

        assert_raises_rpc_error(
                -8,
                "Must provide at least one address without a specified amount" ,
                self.wallet.sendall,
                []
            )

    @cleanup
    def sendall_fails_on_specific_inputs_with_send_max(self):
        self.log.info("Test sendall fails because send_max is used while specific inputs are provided")
        self.add_utxos([15, 6])
        utxo = self.wallet.listunspent()[0]

        assert_raises_rpc_error(-8,
            "Cannot combine send_max with specific inputs.",
            self.wallet.sendall,
            recipients=[self.remainder_target],
            inputs=[utxo], send_max=True)

    @cleanup
    def sendall_fails_on_high_fee(self):
        self.log.info("Test sendall fails if the transaction fee exceeds the maxtxfee")
        self.add_utxos([21])

        assert_raises_rpc_error(
                -4,
                "Fee exceeds maximum configured by user",
                self.wallet.sendall,
                recipients=[self.remainder_target],
                fee_rate=100000)

    @cleanup
    def sendall_fails_on_low_fee(self):
        self.log.info("Test sendall fails if the transaction fee is lower than the minimum fee rate setting")
        assert_raises_rpc_error(-8, "Fee rate (0.999 sat/vB) is lower than the minimum fee rate setting (1.000 sat/vB)",
        self.wallet.sendall, recipients=[self.recipient], fee_rate=0.999)

    @cleanup
    def sendall_watchonly_specific_inputs(self):
        self.log.info("Test sendall with a subset of UTXO pool in a watchonly wallet")
        self.add_utxos([17, 4])
        utxo = self.wallet.listunspent()[0]

        self.nodes[0].createwallet(wallet_name="watching", disable_private_keys=True)
        watchonly = self.nodes[0].get_wallet_rpc("watching")

        import_req = [{
            "desc": utxo["desc"],
            "timestamp": 0,
        }]
        watchonly.importdescriptors(import_req)

        sendall_tx_receipt = watchonly.sendall(recipients=[self.remainder_target], inputs=[utxo])
        psbt = sendall_tx_receipt["psbt"]
        decoded = self.nodes[0].decodepsbt(psbt)
        assert_equal(len(decoded["inputs"]), 1)
        assert_equal(len(decoded["outputs"]), 1)
        assert_equal(decoded["tx"]["vin"][0]["txid"], utxo["txid"])
        assert_equal(decoded["tx"]["vin"][0]["vout"], utxo["vout"])
        assert_equal(decoded["tx"]["vout"][0]["scriptPubKey"]["address"], self.remainder_target)

    @cleanup
    def sendall_with_minconf(self):
        # utxo of 17 bicoin has 6 confirmations, utxo of 4 has 3
        self.add_utxos([17])
        self.generate(self.nodes[0], 2)
        self.add_utxos([4])
        self.generate(self.nodes[0], 2)

        self.log.info("Test sendall fails because minconf is negative")

        assert_raises_rpc_error(-8,
            "Invalid minconf (minconf cannot be negative): -2",
            self.wallet.sendall,
            recipients=[self.remainder_target],
            options={"minconf": -2})
        self.log.info("Test sendall fails because minconf is used while specific inputs are provided")

        utxo = self.wallet.listunspent()[0]
        assert_raises_rpc_error(-8,
            "Cannot combine minconf or maxconf with specific inputs.",
            self.wallet.sendall,
            recipients=[self.remainder_target],
            options={"inputs": [utxo], "minconf": 2})

        self.log.info("Test sendall fails because there are no utxos with enough confirmations specified by minconf")

        assert_raises_rpc_error(-6,
            "Total value of UTXO pool too low to pay for transaction. Try using lower feerate or excluding uneconomic UTXOs with 'send_max' option.",
            self.wallet.sendall,
            recipients=[self.remainder_target],
            options={"minconf": 7})

        self.log.info("Test sendall only spends utxos with a specified number of confirmations when minconf is used")
        self.wallet.sendall(recipients=[self.remainder_target], fee_rate=300, options={"minconf": 6})

        assert_equal(len(self.wallet.listunspent()), 1)
        assert_equal(self.wallet.listunspent()[0]['confirmations'], 3)

        # decrease minconf and show the remaining utxo is picked up
        self.wallet.sendall(recipients=[self.remainder_target], fee_rate=300, options={"minconf": 3})
        assert_equal(self.wallet.getbalance(), 0)

    @cleanup
    def sendall_with_maxconf(self):
        # utxo of 17 bicoin has 6 confirmations, utxo of 4 has 3
        self.add_utxos([17])
        self.generate(self.nodes[0], 2)
        self.add_utxos([4])
        self.generate(self.nodes[0], 2)

        self.log.info("Test sendall fails because there are no utxos with enough confirmations specified by maxconf")
        assert_raises_rpc_error(-6,
            "Total value of UTXO pool too low to pay for transaction. Try using lower feerate or excluding uneconomic UTXOs with 'send_max' option.",
            self.wallet.sendall,
            recipients=[self.remainder_target],
            options={"maxconf": 1})

        self.log.info("Test sendall only spends utxos with a specified number of confirmations when maxconf is used")
        self.wallet.sendall(recipients=[self.remainder_target], fee_rate=300, options={"maxconf":4})
        assert_equal(len(self.wallet.listunspent()), 1)
        assert_equal(self.wallet.listunspent()[0]['confirmations'], 6)

    @cleanup
    def sendall_spends_unconfirmed_change(self):
        self.log.info("Test that sendall spends unconfirmed change")
        self.add_utxos([17])
        self.wallet.sendtoaddress(self.remainder_target, 10)
        assert_greater_than(self.wallet.getbalances()["mine"]["trusted"], 6)
        self.test_sendall_success(sendall_args = [self.remainder_target])

        assert_equal(self.wallet.getbalance(), 0)

    @cleanup
    def sendall_spends_unconfirmed_inputs_if_specified(self):
        self.log.info("Test that sendall spends specified unconfirmed inputs")
        self.def_wallet.sendtoaddress(self.wallet.getnewaddress(), 17)
        self.wallet.syncwithvalidationinterfacequeue()
        assert_equal(self.wallet.getbalances()["mine"]["untrusted_pending"], 17)
        unspent = self.wallet.listunspent(minconf=0)[0]

        self.wallet.sendall(recipients=[self.remainder_target], inputs=[unspent])
        assert_equal(self.wallet.getbalance(), 0)

    @cleanup
    def sendall_does_ancestor_aware_funding(self):
        self.log.info("Test that sendall does ancestor aware funding for unconfirmed inputs")

        # higher parent feerate
        self.def_wallet.sendtoaddress(address=self.wallet.getnewaddress(), amount=17, fee_rate=20)
        self.wallet.syncwithvalidationinterfacequeue()

        assert_equal(self.wallet.getbalances()["mine"]["untrusted_pending"], 17)
        unspent = self.wallet.listunspent(minconf=0)[0]

        parent_txid = unspent["txid"]
        assert_equal(self.wallet.gettransaction(parent_txid)["confirmations"], 0)

        res_1 = self.wallet.sendall(recipients=[self.def_wallet.getnewaddress()], inputs=[unspent], fee_rate=20, add_to_wallet=False, lock_unspents=True)
        child_hex = res_1["hex"]

        child_tx = self.wallet.decoderawtransaction(child_hex)
        higher_parent_feerate_amount = child_tx["vout"][0]["value"]

        # lower parent feerate
        self.def_wallet.sendtoaddress(address=self.wallet.getnewaddress(), amount=17, fee_rate=10)
        self.wallet.syncwithvalidationinterfacequeue()
        assert_equal(self.wallet.getbalances()["mine"]["untrusted_pending"], 34)
        unspent = self.wallet.listunspent(minconf=0)[0]

        parent_txid = unspent["txid"]
        assert_equal(self.wallet.gettransaction(parent_txid)["confirmations"], 0)

        res_2 = self.wallet.sendall(recipients=[self.def_wallet.getnewaddress()], inputs=[unspent], fee_rate=20, add_to_wallet=False, lock_unspents=True)
        child_hex = res_2["hex"]

        child_tx = self.wallet.decoderawtransaction(child_hex)
        lower_parent_feerate_amount = child_tx["vout"][0]["value"]

        assert_greater_than(higher_parent_feerate_amount, lower_parent_feerate_amount)

    # This tests needs to be the last one otherwise @cleanup will fail with "Transaction too large" error
    def sendall_fails_with_transaction_too_large(self):
        self.log.info("Test that sendall fails if resulting transaction is too large")

        # Force the wallet to bulk-generate the addresses we'll need
        self.wallet.keypoolrefill(1600)

        # create many inputs
        outputs = {self.wallet.getnewaddress(): 0.000025 for _ in range(1600)}
        self.def_wallet.sendmany(amounts=outputs)
        self.generate(self.nodes[0], 1)

        assert_raises_rpc_error(
                -4,
                "Transaction too large.",
                self.wallet.sendall,
                recipients=[self.remainder_target])

    def run_test(self):
        self.nodes[0].createwallet("activewallet")
        self.wallet = self.nodes[0].get_wallet_rpc("activewallet")
        self.def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 101)
        self.recipient = self.def_wallet.getnewaddress() # payee for a specific amount
        self.remainder_target = self.def_wallet.getnewaddress() # address that receives everything left after payments and fees
        self.split_target = self.def_wallet.getnewaddress() # 2nd target when splitting rest

        # Test cleanup
        self.test_cleanup()

        # Basic sweep: everything to one address
        self.sendall_two_utxos()

        # Split remainder to two addresses with equal amounts
        self.sendall_split()

        # Pay recipient and sweep remainder
        self.sendall_and_spend()

        # sendall fails if no recipient has unspecified amount
        self.sendall_invalid_recipient_addresses()

        # Sendall fails if same destination is provided twice
        self.sendall_duplicate_recipient()

        # Sendall fails when trying to spend more than the balance
        self.sendall_invalid_amounts()

        # Sendall fails when wallet has no economically spendable UTXOs
        self.sendall_negative_effective_value()

        # Leave dust behind if using send_max
        self.sendall_with_send_max()

        # Sendall succeeds with specific inputs
        self.sendall_specific_inputs()

        # Fails for the right reasons on missing or previously spent UTXOs
        self.sendall_fails_on_missing_input()

        # Sendall fails when no address is provided
        self.sendall_fails_on_no_address()

        # Sendall fails when using send_max while specifying inputs
        self.sendall_fails_on_specific_inputs_with_send_max()

        # Sendall fails when providing a fee that is too high
        self.sendall_fails_on_high_fee()

        # Sendall fails when fee rate is lower than minimum
        self.sendall_fails_on_low_fee()

        # Sendall succeeds with watchonly wallets spending specific UTXOs
        self.sendall_watchonly_specific_inputs()

        # Sendall only uses outputs with at least a give number of confirmations when using minconf
        self.sendall_with_minconf()

        # Sendall only uses outputs with less than a given number of confirmation when using minconf
        self.sendall_with_maxconf()

        # Sendall spends unconfirmed change
        self.sendall_spends_unconfirmed_change()

        # Sendall spends unconfirmed inputs if they are specified
        self.sendall_spends_unconfirmed_inputs_if_specified()

        # Sendall does ancestor aware funding when spending an unconfirmed UTXO
        self.sendall_does_ancestor_aware_funding()

        # Sendall fails when many inputs result to too large transaction
        self.sendall_fails_with_transaction_too_large()

if __name__ == '__main__':
    SendallTest().main()
