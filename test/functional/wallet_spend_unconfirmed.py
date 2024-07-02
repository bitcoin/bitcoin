#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal, getcontext

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_greater_than_or_equal,
    assert_equal,
    find_vout_for_address,
)

class UnconfirmedInputTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        getcontext().prec=9
        self.setup_clean_chain = True
        self.num_nodes = 1

    def setup_and_fund_wallet(self, walletname):
        self.nodes[0].createwallet(walletname)
        wallet = self.nodes[0].get_wallet_rpc(walletname)

        self.def_wallet.sendtoaddress(address=wallet.getnewaddress(), amount=2)
        self.generate(self.nodes[0], 1) # confirm funding tx
        return wallet

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def calc_fee_rate(self, tx):
        fee = Decimal(-1e8) * tx["fee"]
        vsize = tx["decoded"]["vsize"]
        return fee / vsize

    def calc_set_fee_rate(self, txs):
        fees = Decimal(-1e8) * sum([tx["fee"] for tx in txs]) # fee is negative!
        vsizes = sum([tx["decoded"]["vsize"] for tx in txs])
        return fees / vsizes

    def assert_spends_only_parents(self, tx, parent_txids):
        parent_checklist = parent_txids.copy()
        number_inputs = len(tx["decoded"]["vin"])
        assert_equal(number_inputs, len(parent_txids))
        for i in range(number_inputs):
            txid_of_input = tx["decoded"]["vin"][i]["txid"]
            assert txid_of_input in parent_checklist
            parent_checklist.remove(txid_of_input)

    def assert_undershoots_target(self, tx):
        resulting_fee_rate = self.calc_fee_rate(tx)
        assert_greater_than_or_equal(self.target_fee_rate, resulting_fee_rate)

    def assert_beats_target(self, tx):
        resulting_fee_rate = self.calc_fee_rate(tx)
        assert_greater_than_or_equal(resulting_fee_rate, self.target_fee_rate)

    # Meta-Test: try feerate testing function on confirmed UTXO
    def test_target_feerate_confirmed(self):
        self.log.info("Start test feerate with confirmed input")
        wallet = self.setup_and_fund_wallet("confirmed_wallet")

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_beats_target(ancestor_aware_tx)

        wallet.unloadwallet()

    # Spend unconfirmed UTXO from high-feerate parent
    def test_target_feerate_unconfirmed_high(self):
        self.log.info("Start test feerate with high feerate unconfirmed input")
        wallet = self.setup_and_fund_wallet("unconfirmed_high_wallet")

        # Send unconfirmed transaction with high feerate to testing wallet
        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=3*self.target_fee_rate)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)
        self.assert_beats_target(parent_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)

        wallet.unloadwallet()

    # Spend unconfirmed UTXO from low-feerate parent. Expect that parent gets
    # bumped to target feerate.
    def test_target_feerate_unconfirmed_low(self):
        self.log.info("Start test feerate with low feerate unconfirmed input")
        wallet = self.setup_and_fund_wallet("unconfirmed_low_wallet")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=1)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(parent_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Spend UTXO with unconfirmed low feerate parent and grandparent
    # txs. Expect that both ancestors get bumped to target feerate.
    def test_chain_of_unconfirmed_low(self):
        self.log.info("Start test with parent and grandparent tx")
        wallet = self.setup_and_fund_wallet("unconfirmed_low_chain_wallet")

        grandparent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.8, fee_rate=1)
        gp_tx = wallet.gettransaction(txid=grandparent_txid, verbose=True)

        self.assert_undershoots_target(gp_tx)

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=2)
        p_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(p_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=1.3, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([gp_tx, p_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Spend unconfirmed UTXOs from two low feerate parent txs.
    def test_two_low_feerate_unconfirmed_parents(self):
        self.log.info("Start test with two unconfirmed parent txs")
        wallet = self.setup_and_fund_wallet("two_parents_wallet")

        # Add second UTXO to tested wallet
        self.def_wallet.sendtoaddress(address=wallet.getnewaddress(), amount=2)
        self.generate(self.nodes[0], 1) # confirm funding tx

        parent_one_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=2)
        p_one_tx = wallet.gettransaction(txid=parent_one_txid, verbose=True)
        self.assert_undershoots_target(p_one_tx)

        parent_two_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=1)
        p_two_tx = wallet.gettransaction(txid=parent_two_txid, verbose=True)
        self.assert_undershoots_target(p_two_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=2.8, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_one_txid, parent_two_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([p_one_tx, p_two_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Spend two unconfirmed inputs, one each from low and high feerate parents
    def test_mixed_feerate_unconfirmed_parents(self):
        self.log.info("Start test with two unconfirmed parent txs one of which has a higher feerate")
        wallet = self.setup_and_fund_wallet("two_mixed_parents_wallet")

        # Add second UTXO to tested wallet
        self.def_wallet.sendtoaddress(address=wallet.getnewaddress(), amount=2)
        self.generate(self.nodes[0], 1) # confirm funding tx

        high_parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=self.target_fee_rate*2)
        p_high_tx = wallet.gettransaction(txid=high_parent_txid, verbose=True)
        # This time the parent is greater than the child
        self.assert_beats_target(p_high_tx)

        parent_low_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=1)
        p_low_tx = wallet.gettransaction(txid=parent_low_txid, verbose=True)
        # Other parent needs bump
        self.assert_undershoots_target(p_low_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=2.8, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_low_txid, high_parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([p_high_tx, p_low_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)

        resulting_bumped_ancestry_fee_rate = self.calc_set_fee_rate([p_low_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_bumped_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_bumped_ancestry_fee_rate)

        wallet.unloadwallet()

    # Spend from chain with high feerate grandparent and low feerate parent
    def test_chain_of_high_low(self):
        self.log.info("Start test with low parent and high grandparent tx")
        wallet = self.setup_and_fund_wallet("high_low_chain_wallet")

        grandparent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.8, fee_rate=self.target_fee_rate * 10)
        gp_tx = wallet.gettransaction(txid=grandparent_txid, verbose=True)
        # grandparent has higher feerate
        self.assert_beats_target(gp_tx)

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=1)
        # parent is low feerate
        p_tx = wallet.gettransaction(txid=parent_txid, verbose=True)
        self.assert_undershoots_target(p_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=1.3, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([p_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)
        resulting_ancestry_fee_rate_with_high_feerate_gp = self.calc_set_fee_rate([gp_tx, p_tx, ancestor_aware_tx])
        # Check that we bumped the parent without relying on the grandparent
        assert_greater_than_or_equal(resulting_ancestry_fee_rate_with_high_feerate_gp, self.target_fee_rate*1.1)

        wallet.unloadwallet()

    # Spend UTXO from chain of unconfirmed transactions with low feerate
    # grandparent and even lower feerate parent
    def test_chain_of_high_low_below_target_feerate(self):
        self.log.info("Start test with low parent and higher low grandparent tx")
        wallet = self.setup_and_fund_wallet("low_and_lower_chain_wallet")

        grandparent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.8, fee_rate=5)
        gp_tx = wallet.gettransaction(txid=grandparent_txid, verbose=True)

        # grandparent has higher feerate, but below target
        self.assert_undershoots_target(gp_tx)

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1.5, fee_rate=1)
        p_tx = wallet.gettransaction(txid=parent_txid, verbose=True)
        # parent even lower
        self.assert_undershoots_target(p_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=1.3, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([gp_tx, p_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Test fee calculation when bumping while using subtract fee from output (SFFO)
    def test_target_feerate_unconfirmed_low_sffo(self):
        self.log.info("Start test feerate with low feerate unconfirmed input, while subtracting from output")
        wallet = self.setup_and_fund_wallet("unconfirmed_low_wallet_sffo")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=1)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(parent_tx)

        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate, subtractfeefromamount=True)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Test that parents of preset unconfirmed inputs get cpfp'ed
    def test_preset_input_cpfp(self):
        self.log.info("Start test with preset input from low feerate unconfirmed transaction")
        wallet = self.setup_and_fund_wallet("preset_input")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=1)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(parent_tx)

        number_outputs = len(parent_tx["decoded"]["vout"])
        assert_equal(number_outputs, 2)

        # we don't care which of the two outputs we spent, they're both ours
        ancestor_aware_txid = wallet.send(outputs=[{self.def_wallet.getnewaddress(): 0.5}], fee_rate=self.target_fee_rate, options={"add_inputs": True, "inputs": [{"txid": parent_txid, "vout": 0}]})["txid"]
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Test that RBFing a transaction with unconfirmed input gets the right feerate
    def test_rbf_bumping(self):
        self.log.info("Start test to rbf a transaction unconfirmed input to bump it")
        wallet = self.setup_and_fund_wallet("bump")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=1)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(parent_tx)

        to_be_rbfed_ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=to_be_rbfed_ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        bumped_ancestor_aware_txid = wallet.bumpfee(txid=to_be_rbfed_ancestor_aware_txid, options={"fee_rate": self.target_fee_rate * 2} )["txid"]
        bumped_ancestor_aware_tx = wallet.gettransaction(txid=bumped_ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        resulting_bumped_fee_rate = self.calc_fee_rate(bumped_ancestor_aware_tx)
        assert_greater_than_or_equal(resulting_bumped_fee_rate, 2*self.target_fee_rate)
        resulting_bumped_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, bumped_ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_bumped_ancestry_fee_rate, 2*self.target_fee_rate)
        assert_greater_than_or_equal(2*self.target_fee_rate*1.01, resulting_bumped_ancestry_fee_rate)

        wallet.unloadwallet()

    # Test that transaction spending two UTXOs with overlapping ancestry does not bump shared ancestors twice
    def test_target_feerate_unconfirmed_low_overlapping_ancestry(self):
        self.log.info("Start test where two UTXOs have overlapping ancestry")
        wallet = self.setup_and_fund_wallet("overlapping_ancestry_wallet")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=1)
        two_output_parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(two_output_parent_tx)

        # spend both outputs from parent transaction
        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=1.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid, parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([two_output_parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Test that new transaction ignores sibling transaction with low feerate
    def test_sibling_tx_gets_ignored(self):
        self.log.info("Start test where a low-fee sibling tx gets created and check that bumping ignores it")
        wallet = self.setup_and_fund_wallet("ignore-sibling")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=2)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(parent_tx)

        # create sibling tx
        sibling_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.9, fee_rate=1)
        sibling_tx = wallet.gettransaction(txid=sibling_txid, verbose=True)
        self.assert_undershoots_target(sibling_tx)

        # spend both outputs from parent transaction
        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Test that new transaction only pays for itself when high feerate sibling pays for parent
    def test_sibling_tx_bumps_parent(self):
        self.log.info("Start test where a high-fee sibling tx bumps the parent")
        wallet = self.setup_and_fund_wallet("generous-sibling")

        parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=1)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)
        self.assert_undershoots_target(parent_tx)

        # create sibling tx
        sibling_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.9, fee_rate=3*self.target_fee_rate)
        sibling_tx = wallet.gettransaction(txid=sibling_txid, verbose=True)
        self.assert_beats_target(sibling_tx)

        # spend both outputs from parent transaction
        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=0.5, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        # Child is only paying for itself…
        resulting_fee_rate = self.calc_fee_rate(ancestor_aware_tx)
        assert_greater_than_or_equal(1.05 * self.target_fee_rate, resulting_fee_rate)
        # …because sibling bumped to parent to ~50 s/vB, while our target is 30 s/vB
        resulting_ancestry_fee_rate_sibling = self.calc_set_fee_rate([parent_tx, sibling_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate_sibling, self.target_fee_rate)
        # and our resulting "ancestry feerate" is therefore BELOW target feerate
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(self.target_fee_rate, resulting_ancestry_fee_rate)

        wallet.unloadwallet()

    # Spend a confirmed and an unconfirmed input at the same time
    def test_confirmed_and_unconfirmed_parent(self):
        self.log.info("Start test with one unconfirmed and one confirmed input")
        wallet = self.setup_and_fund_wallet("confirmed_and_unconfirmed_wallet")
        confirmed_parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, fee_rate=self.target_fee_rate)
        self.generate(self.nodes[0], 1) # Wallet has two confirmed UTXOs of ~1BTC each
        unconfirmed_parent_txid = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=0.5, fee_rate=0.5*self.target_fee_rate)

        # wallet has one confirmed UTXO of 1BTC and two unconfirmed UTXOs of ~0.5BTC each
        ancestor_aware_txid = wallet.sendtoaddress(address=self.def_wallet.getnewaddress(), amount=1.4, fee_rate=self.target_fee_rate)
        ancestor_aware_tx = wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)
        self.assert_spends_only_parents(ancestor_aware_tx, [confirmed_parent_txid, unconfirmed_parent_txid])
        resulting_fee_rate = self.calc_fee_rate(ancestor_aware_tx)
        assert_greater_than_or_equal(resulting_fee_rate, self.target_fee_rate)

        wallet.unloadwallet()

    def test_external_input_unconfirmed_low(self):
        self.log.info("Send funds to an external wallet then build tx that bumps parent by spending external input")
        wallet = self.setup_and_fund_wallet("test_external_wallet")

        external_address = self.def_wallet.getnewaddress()
        address_info = self.def_wallet.getaddressinfo(external_address)
        external_descriptor = address_info["desc"]
        parent_txid = wallet.sendtoaddress(address=external_address, amount=1, fee_rate=1)
        parent_tx = wallet.gettransaction(txid=parent_txid, verbose=True)

        self.assert_undershoots_target(parent_tx)

        spend_res = wallet.send(outputs=[{self.def_wallet.getnewaddress(): 0.5}], fee_rate=self.target_fee_rate, options={"inputs":[{"txid":parent_txid, "vout":find_vout_for_address(self.nodes[0], parent_txid, external_address)}], "solving_data":{"descriptors":[external_descriptor]}})
        signed_psbt = self.def_wallet.walletprocesspsbt(spend_res["psbt"])
        external_tx = self.def_wallet.finalizepsbt(signed_psbt["psbt"])
        ancestor_aware_txid = self.def_wallet.sendrawtransaction(external_tx["hex"])

        ancestor_aware_tx = self.def_wallet.gettransaction(txid=ancestor_aware_txid, verbose=True)

        self.assert_spends_only_parents(ancestor_aware_tx, [parent_txid])

        self.assert_beats_target(ancestor_aware_tx)
        resulting_ancestry_fee_rate = self.calc_set_fee_rate([parent_tx, ancestor_aware_tx])
        assert_greater_than_or_equal(resulting_ancestry_fee_rate, self.target_fee_rate)
        assert_greater_than_or_equal(self.target_fee_rate*1.01, resulting_ancestry_fee_rate)

        wallet.unloadwallet()


    def run_test(self):
        self.log.info("Starting UnconfirmedInputTest!")
        self.target_fee_rate = 30
        self.def_wallet  = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.generate(self.nodes[0], 110)

        self.test_target_feerate_confirmed()

        self.test_target_feerate_unconfirmed_high()

        self.test_target_feerate_unconfirmed_low()

        self.test_chain_of_unconfirmed_low()

        self.test_two_low_feerate_unconfirmed_parents()

        self.test_mixed_feerate_unconfirmed_parents()

        self.test_chain_of_high_low()

        self.test_chain_of_high_low_below_target_feerate()

        self.test_target_feerate_unconfirmed_low_sffo()

        self.test_preset_input_cpfp()

        self.test_rbf_bumping()

        self.test_target_feerate_unconfirmed_low_overlapping_ancestry()

        self.test_sibling_tx_gets_ignored()

        self.test_sibling_tx_bumps_parent()

        self.test_confirmed_and_unconfirmed_parent()

        self.test_external_input_unconfirmed_low()

if __name__ == '__main__':
    UnconfirmedInputTest(__file__).main()
