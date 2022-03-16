#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.messages import (
    COIN
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_approx,
    assert_greater_than,
)


class CoinselectionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[
            "-consolidatefeerate=0.0001",  # 10 sat/vb
        ]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_few_small_coins()
        self.test_one_big_and_many_small_coins()
        self.test_one_big_coin()
        self.test_one_big_and_small_coin()

    def test_few_small_coins(self):
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet(wallet_name="test_few_small_coins")
        w1 = self.nodes[0].get_wallet_rpc("test_few_small_coins")

        # seed the wallet with three coins
        tx1 = w0.sendtoaddress(w1.getnewaddress("", "bech32"), 1)
        tx2 = w0.sendtoaddress(w1.getnewaddress("", "bech32"), 1)
        tx3 = w0.sendtoaddress(w1.getnewaddress("", "legacy"), 1)
        self.generate(self.nodes[0], 1)

        # TODO: set long_term_fee_rate explicitly
        # fee rate is set above long_term_fee_rate (10 sat/vbyte)
        fee_rate = 20

        # There are four possible transactions with 2 or less inputs:
        # 1x bech32      waste = 680  = 68*20 - 68*10
        #                fee = 2200
        # 1x legacy      waste = 1480 = 148*20 - 148*10
        #                fee = 3800
        # 2x bech32      waste = 2660 = (68*20 - 68*10)*2 + 68*10 + 31*20
        #                fee = 4180
        # legacy+bech32  waste = 3460 = (68*20 - 68*10) + (148*20 - 148*10) + 68*10 + 31*20
        #                fee = 5780

        dummyaddress = w0.getnewaddress()

        def select_coins_for(target):
            result = w1.fundrawtransaction(w1.createrawtransaction([], {dummyaddress: target}), {"fee_rate": fee_rate})
            return w1.decoderawtransaction(result['hex'])

        # 1-input-1-output tx is 110vbytes (11+68+31) which results in 2200sat fee at 20sat/vbyte
        # so we have to use 2 inputs and create a change
        tx = select_coins_for(0.99997801)
        assert_equal(len(tx['vin']), 2)
        assert_equal(len(tx['vout']), 2)

        # max amount we can send with one coin
        tx = select_coins_for(0.99997800)
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 1)
        assert(tx['vin'][0]['txid'] in [tx1, tx2])

        # cost of change is 31*20 + 68*10 = 1300sat
        # so it's economically efficient to drop anything below that amount to fees
        tx = select_coins_for(0.99996500)
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 1)
        assert(tx['vin'][0]['txid'] in [tx1, tx2])

        # adding one more input to create a change increases waste by (68*20 - 68*10) = 680sat
        # so it's still efficient to drop that much more to fees
        tx = select_coins_for(0.99996201)
        # TODO: implement optimization and uncomment the assertions
        # assert_equal(len(tx['vin']), 1)
        # assert_equal(len(tx['vout']), 1)
        # assert(tx['vin'][0]['txid'] in [tx1, tx2])

        # fees for 1 legacy input tx = (11+148+31)*20 = 3800sat
        # at this point we're dropping to fees enough so we can spend heavier input with less waste
        tx = select_coins_for(0.99996200)
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 1)
        assert_equal(tx['vin'][0]['txid'], tx3)

        # paying enough fees for 2x bech32 input tx
        # but using one legacy and dropping for fees is still more efficient in terms of waste
        tx = select_coins_for(0.99995820)
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 1)
        assert_equal(tx['vin'][0]['txid'], tx3)

        # max amount when dropping for fees is still more efficient than creating a change
        # With fee = 4980, we'd be dropping 1180sat excess which brings waste to 2660
        # which is equal to the waste of 2x bech32 solution
        tx = select_coins_for(0.99995021)
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 1)
        assert_equal(tx['vin'][0]['txid'], tx3)

        # The best solution is the one with two bech32 inputs and a change
        # However knapsack prefers legacy + bech32 solution as it's closer to the target due to higher weight
        # SRD could the best solution if lucky.
        tx = select_coins_for(0.99995020)
        # TODO: randomness here, could be one or two inputs
        # print(tx['vin'])
        # print("{} / {}".format(len(tx['vin']), len(tx['vout'])))

        # With fee at 5101sat we would be dropping 1301sat for fess
        # This is more than cost of change and BnB won't find a solution with single input
        # We either get
        # 1) 2x bech32 solution (SRD only)
        # 2) legacy + bech32 solution
        tx = select_coins_for(0.99994899)
        # legacy + dropping more for fees is still better in terms of waste than legacy + bech32
        # TODO: implement optimization and update asserts
        assert_equal(len(tx['vin']), 2)
        assert_equal(len(tx['vout']), 2)

        # a solution with change is the best solution
        # 2x bech32 is still more efficient than bech32-and-legacy, but only could be found with SRD
        tx = select_coins_for(0.99994220)
        assert_equal(len(tx['vin']), 2)
        assert_equal(len(tx['vout']), 2)

    def test_one_big_and_many_small_coins(self):
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet(wallet_name="test_one_big_and_many_small_coins")
        w1 = self.nodes[0].get_wallet_rpc("test_one_big_and_many_small_coins")

        # Create a bunch of small coins
        for i in range(100):
            w0.sendtoaddress(w1.getnewaddress("", "legacy"), 0.05)

        # We will target a solution with 20 coins that will add up exactly to 1btc.
        # Add a coin that will cover all the fees and exactly match the target.
        # TODO: fix that knapsack targets change even if there is no change needed
        # (11+148*20+31*2)*350+5000000 = 6061550
        w0.sendtoaddress(w1.getnewaddress("", "legacy"), 0.0606155)
        self.generate(self.nodes[0], 6)

        fee_rate = 350
        dummyaddress = w0.getnewaddress()

        def select_coins(big_coin, target, iterations=100):
            txid = w0.sendtoaddress(w1.getnewaddress("", "bech32"), big_coin)
            self.generate(self.nodes[0], 6)

            fee_total = 0
            max_fee = 0
            for _ in range(iterations):
                result = w1.fundrawtransaction(w1.createrawtransaction([], {dummyaddress: target}), {"fee_rate": fee_rate})
                fee_total += result['fee']
                max_fee = max(max_fee, result['fee'])

            # lock big coin
            w1.lockunspent(False, [tx for tx in w1.listunspent() if tx['txid'] == txid])
            return fee_total, max_fee

        # in all the scenarios below the best solution would be to take one big coin and create a change output
        best_fee = (11 + 68 + 31 + 31) * fee_rate / COIN

        # 0) no exact (down to the last sat) match is possible, the big coin is closest to target + MIN_CHANGE,
        # knapsack always finds the best solution
        fee_total, max_fee = select_coins(big_coin=1.01051, target=1.00000001, iterations=100)
        assert_approx(max_fee, best_fee)
        assert_approx(fee_total, best_fee * 100)

        # 1) knapsack latches to the exact (down to the last sat) match solution and can't find the best solution
        # SRD can find a solution better than knapsack if draws the big coin in the first 21 tries
        # chances for SRD to find it are roughly 2*(21/102) = ±40%.
        # Chances are doubled due to coinselection run twice with and without APS enabled
        fee_total, max_fee = select_coins(big_coin=1.01051, target=1.0, iterations=100)
        knapsack_fee = (42 + 31 + 148*20) * fee_rate / COIN
        assert_approx(max_fee, knapsack_fee)
        # total fee is lower due to lucky SRD solutions
        assert_greater_than(knapsack_fee * 100, fee_total)
        assert_greater_than(fee_total, best_fee * 100)

        # 2) no exact (down to the last sat) match, effective value of the big coin is smaller than target + MIN_CHANGE
        # BnB will find the same solution from previous test but dropping the change output to fees
        # SRD still occasionally find better solution
        fee_total, max_fee = select_coins(big_coin=1.01031, target=1.00000001, iterations=100)
        dropped_to_fees = 31 * fee_rate
        bnb_fee = ((42 + 148*20) * fee_rate + dropped_to_fees) / COIN
        assert_approx(max_fee, bnb_fee)
        assert_greater_than(bnb_fee * 100, fee_total)
        assert_greater_than(fee_total, best_fee * 100)

        # 3) target is too far away for BnB to find a solution
        # effective value of the big coin is smaller than target + MIN_CHANGE
        #  * BnB can't find a solution as the distance from target is farther than cost of change
        #    6061550 + 5000000*19-(42+148*20)*350-99999319 = 11531
        #    cost_of_change = 11530 = 31*350 + 68*10
        #  * knapsack finds closest solution to target + MIN_CHANGE with 21 small coins
        #  * SRD still occasionally find better solution
        fee_total, max_fee = select_coins(big_coin=1.01031, target=0.99999319, iterations=100)
        srd_fee = (41 + 147*21 + 31) * fee_rate / COIN
        assert_approx(max_fee, srd_fee)
        assert_greater_than(srd_fee * 100, fee_total)
        assert_greater_than(fee_total, best_fee * 100)

        # 4) knapsack latches to the target + MIN_CHANGES (matched down to the last sat)
        # Same test case as 1) but both target and big_coin reduced by MIN_CHANGE
        # SRD still occasionally find better solution
        fee_total, max_fee = select_coins(big_coin=1.00051, target=0.99, iterations=100)
        knapsack_fee = (41 + 31 + 147*20) * fee_rate / COIN
        assert_approx(max_fee, knapsack_fee)
        assert_greater_than(knapsack_fee * 100, fee_total)
        assert_greater_than(fee_total, best_fee * 100)

    def test_one_big_coin(self):
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet(wallet_name="test_one_big_coin")
        w1 = self.nodes[0].get_wallet_rpc("test_one_big_coin")

        w0.sendtoaddress(w1.getnewaddress("", "legacy"), 50)
        self.generate(self.nodes[0], 1)

        # change is less than MIN_CHANGE but we still create it as we don't have other options
        result = w1.fundrawtransaction(w1.createrawtransaction([], {w0.getnewaddress(): 49.9999}), {"fee_rate": 10})
        tx = w1.decoderawtransaction(result['hex'])
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 2)

    def test_one_big_and_small_coin(self):
        w0 = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet(wallet_name="test_one_big_and_small_coin")
        w1 = self.nodes[0].get_wallet_rpc("test_one_big_and_small_coin")

        w0.sendtoaddress(w1.getnewaddress("", "legacy"), 0.011)
        w0.sendtoaddress(w1.getnewaddress("", "legacy"), 0.011)
        w0.sendtoaddress(w1.getnewaddress("", "legacy"), 0.011)
        w0.sendtoaddress(w1.getnewaddress("", "legacy"), 50)
        self.generate(self.nodes[0], 1)

        dummyaddress = w0.getnewaddress()
        fee_rate = 20

        def select_coins_for(target):
            result = w1.fundrawtransaction(w1.createrawtransaction([], {dummyaddress: target}), {"fee_rate": fee_rate})
            return w1.decoderawtransaction(result['hex'])

        # if change is > MIN_CHANGE than we just spend big coin and create a change
        tx = select_coins_for(49.9)
        assert_equal(len(tx['vin']), 1)
        assert_equal(len(tx['vout']), 2)

        # if small coins > MIN_CHANGE than we just add one small coin to the big one and create change
        tx = select_coins_for(50)
        assert_equal(len(tx['vin']), 2)
        assert_equal(len(tx['vout']), 2)


if __name__ == '__main__':
    CoinselectionTest().main()
