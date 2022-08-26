#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the Coin Selection."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_greater_than_or_equal

from decimal import Decimal

# Constants from bitcoin core
BECH32_COIN_SIZE = Decimal(68)
BECH32_VOUT_SIZE = Decimal(31)
HIGHEST_TARGET = Decimal(1008)
RELAY_FEE_RATE_SpVB = Decimal(3)

# Constants that we will set
FEE_RATE_SpVB = Decimal(100)
CONSOLIDATE_FEE_RATE_SpVB = Decimal(10)
DISCARD_RATE_SpVB = Decimal(10)


class CoinSelectionTests(BitcoinTestFramework):

    def getDiscardFeeRateSpVB(self):
        discard_rate = 0
        result = self.nodes[0].estimatesmartfee(
            int(HIGHEST_TARGET), "CONSERVATIVE")
        if ("feerate" in result):
            discard_rate = int(result["feerate"]) * 100000  # btc/kvb to sat/vb

        discard_rate = discard_rate == 0 and DISCARD_RATE_SpVB or min(
            discard_rate, DISCARD_RATE_SpVB)
        discard_rate = max(discard_rate, RELAY_FEE_RATE_SpVB)
        return discard_rate

    def getChangeCost(self):
        # cost of spending change + cost of creating change
        return self.getDiscardFeeRateSpVB() * BECH32_COIN_SIZE + FEE_RATE_SpVB * BECH32_VOUT_SIZE

    def set_test_params(self):
        # nodes[0] where coins are generated and the other (nodes[1]) with a
        # clean and predictable wallet we can run tests on
        self.num_nodes = 2
        self.setup_clean_chain = True

        self.extra_args = [['-whitelist=noban@127.0.0.1',  # This test isn't testing tx relay. Set whitelist on the peers for instant tx relay.
                            f'-consolidatefeerate={CONSOLIDATE_FEE_RATE_SpVB / 100000 }',
                            f'-discardfee={DISCARD_RATE_SpVB / 100000}',
                            ]] * self.num_nodes

        self.rpc_timeout = 90  # to prevent timeouts in `test_transaction_too_large`

    def calculate_waste(self, selected_coins, target_coins, isChange, use_effective_value):
        # use_effective = !subtract_fee_outputs;

        fee = BECH32_COIN_SIZE * FEE_RATE_SpVB
        consolidate_fee = BECH32_COIN_SIZE * CONSOLIDATE_FEE_RATE_SpVB

        waste = len(selected_coins) * (fee - consolidate_fee)

        if isChange:
            # Consider the cost of making change and spending it in the future
            # If we aren't making change, the caller should've set change_cost to 0
            waste += self.getChangeCost()
        else:
            # When we are not making change (change_cost == 0), consider the excess we are throwing away to fees
            selected_effective_value = sum(
                selected_coins) - (use_effective_value and fee * len(selected_coins) or 0)
            waste += (selected_effective_value - sum(target_coins)) * 100000000

        return waste

    def lock_all_coins(self, wallet):
        """Lock all existing coins in the UTXO pool of wallet"""
        # lock all coins from wallet of node[1]
        to_lock = [{"txid": utxo["txid"], "vout": utxo["vout"]}
                   for utxo in wallet.listunspent()]
        wallet.lockunspent(False, to_lock)

    def send_coins(self, src, dest, newCoins):
        """Send UTXOs of values newCoins to it from src"""

        # send newCoins valued coins to wallet of node[1]
        # use bech32 exclusively to easily calculate fees
        rawtxn = src.createrawtransaction(
            [], [{dest.getnewaddress("", "bech32"): coin} for coin in newCoins])
        fundtxn = src.fundrawtransaction(rawtxn)
        signedtxn = src.signrawtransactionwithwallet(fundtxn["hex"])
        src.sendrawtransaction(signedtxn["hex"])

        self.generate(src, 6)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        self.connect_nodes(0, 1)

    def run_test(self):

        # This test is not meant to test fee estimation and we'd like
        # to be sure all txs are sent at a consistent desired feerate
        for i in range(len(self.nodes)):
            # fee rate in btc/kVb
            self.nodes[i].settxfee(FEE_RATE_SpVB / 100000)

        self.generate(self.nodes[0], 121)

        # Each element is a test scenario containing
        # the scenario name, input coins, output coins, solution with upper bound of waste and optionally subtract_fee_outputs
        self.test_scenario("Basic", [2], [1], [2]),
        self.test_scenario("Three Inputs", [1, 3, 2], [3], [1, 3]),
        self.test_scenario("Three Inputs with subtract fee outputs", [
                           1, 3, 2], [3], [1, 2], [0]),

    def test_scenario(self, name, inputs, outputs, upperbound_selection, subtract_fee_outputs=None):

        self.log.info("CoinSelection Scenario: " + name)

        if (subtract_fee_outputs is None):
            subtract_fee_outputs = []

        inputs = list(map(Decimal, inputs))
        outputs = list(map(Decimal, outputs))
        upperbound_selection = list(map(Decimal, upperbound_selection))

        # lock all coins inside 2nd node
        self.lock_all_coins(self.nodes[1])
        # send input coins to 2nd node, on which coin selection will be performed
        self.send_coins(self.nodes[0], self.nodes[1], inputs)

        # Execute the transaction generatetoaddressgeneratetoaddress
        rawtx = self.nodes[1].createrawtransaction(
            [], {self.nodes[0].getnewaddress(): i for i in outputs})
        rawtxfund = self.nodes[1].fundrawtransaction(
            rawtx, {"subtractFeeFromOutputs": subtract_fee_outputs})
        dec_tx = self.nodes[1].decoderawtransaction(rawtxfund['hex'])

        selected_coins = [self.nodes[1].gettxout(coin["txid"], coin["vout"])[
            "value"] for coin in dec_tx["vin"]]

        changeExists = len(dec_tx["vout"]) > len(outputs)

        assert_greater_than_or_equal(self.calculate_waste(upperbound_selection, outputs, 0, 0),
                                     self.calculate_waste(selected_coins, outputs, changeExists, 0))


if __name__ == '__main__':
    CoinSelectionTests().main()
