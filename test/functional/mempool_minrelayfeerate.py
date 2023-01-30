#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Expected node behaviors from accepting packages with below-minrelayfeerate transactions, evicting
all 0-fee mempool entries, and mining without a blockmintxfee."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
)
from test_framework.wallet import (
    MiniWallet,
)

class MempoolMinRelayFeerate(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_low_feerate_mined(self):
        node = self.nodes[0]
        self.generate(self.wallet, 100) # blocks generated for inputs
        # Package with 24 0-fee parents and 1 child.
        package_hex = []
        parent_txids = []
        child_utxos = []
        confirmed_utxo = self.wallet.get_utxo(mark_as_spent=True)
        other_parent_utxos = []
        for _ in range(24):
            parent = self.wallet.create_self_transfer_multi(num_outputs=3, fee_per_output=0)
            child_utxos.append(parent["new_utxos"][0])
            other_parent_utxos.extend(parent["new_utxos"][1:])
            package_hex.append(parent["hex"])
            parent_txids.append(parent["tx"].rehash())
        child = self.wallet.create_self_transfer_multi(utxos_to_spend=child_utxos + [confirmed_utxo], fee_per_output=8000)
        package_hex.append(child["hex"])
        child_replacer = self.wallet.create_self_transfer(utxo_to_spend=confirmed_utxo, fee=Decimal("0.0002"))

        self.generate(self.wallet, COINBASE_MATURITY) # mature these utxos

        min_relay_feerate = node.getmempoolinfo()["minrelaytxfee"]

        self.log.info("Submit package with 24 0-fee parents and 1 child bumping them")
        node.submitpackage(package_hex)
        mempool0 = node.getrawmempool()
        assert_equal(len(mempool0), 25)
        assert all([txid in mempool0 for txid in parent_txids])
        assert child["tx"].rehash() in mempool0

        self.log.info("Add 2 more children from each 0-fee parent, each with 1.9x the min relay feerate")
        extra_children_utxos = []
        for utxo in other_parent_utxos:
            extra_child = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=Decimal("0.00001900"))
            extra_children_utxos.append(extra_child["new_utxo"])
        mempool1 = node.getrawmempool()
        assert_equal(len(mempool1), 73)
        for txid in parent_txids:
            entry = node.getmempoolentry(txid)
            assert_equal(entry["descendantcount"], 4)
            assert_greater_than(entry["vsize"] * min_relay_feerate, entry["fees"]["base"] * 1000)
            assert_greater_than_or_equal(entry["fees"]["descendant"] * 1000, entry["descendantsize"] * min_relay_feerate)

        self.log.info("Replace child with a tx that does not depend on any of the parents")
        node.sendrawtransaction(child_replacer["hex"])
        mempool2 = node.getrawmempool()
        assert_equal(len(mempool2), len(mempool1))
        assert child_replacer["tx"].rehash() in mempool2
        assert not child["tx"].rehash() in mempool2
        assert all([txid in mempool2 for txid in parent_txids])

        # None of these UTXOs are spent, meaning the parents are no longer adequately bumped.
        for utxo in child_utxos:
            utxo_dict = [{ "txid": utxo["txid"], "vout": utxo["vout"] }]
            assert_equal(node.gettxspendingprevout(utxo_dict), utxo_dict)

        # None of the parents were evicted because their descendant feerates are still above min relay feerate.
        for txid in parent_txids:
            entry = node.getmempoolentry(txid)
            assert_equal(entry["descendantcount"], 3)
            assert_greater_than_or_equal(entry["fees"]["descendant"] * 1000, entry["descendantsize"] * min_relay_feerate)

        # Each transaction's ancestor feerate is below min relay feerate.
        for txid in mempool2:
            if txid == child_replacer["tx"].rehash():
                break
            entry = node.getmempoolentry(txid)
            assert_equal(entry["ancestorcount"], 2)
            assert_greater_than(entry["ancestorsize"] * min_relay_feerate, entry["fees"]["ancestor"])

        # Further descendants can be added.
        for utxo in extra_children_utxos:
            extra_child = self.wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=Decimal("0.00001"))
        mempool3 = node.getrawmempool()
        assert_equal(len(mempool3), 121)
        for txid in mempool3:
            if txid == child_replacer["tx"].rehash():
                break
            entry = node.getmempoolentry(txid)
            assert_greater_than(entry["ancestorsize"] * min_relay_feerate, entry["fees"]["ancestor"])

        # The average feerate of these "cheap" transactions is still above min relay feerate.
        cheap_total_fees = node.getmempoolinfo()["total_fee"] - node.getmempoolentry(child_replacer["txid"])["fees"]["modified"]
        cheap_total_vsize = node.getmempoolinfo()["bytes"] - node.getmempoolentry(child_replacer["txid"])["vsize"]
        assert_greater_than_or_equal(cheap_total_fees * 1000, min_relay_feerate * cheap_total_vsize)

        # All transactions are selected for mining, even though their ancestor feerates are below min relay feerate.
        last_block = self.generate(node, 1)
        assert_equal(len(self.nodes[0].getblock(blockhash=last_block[0])["tx"]), 122)
        assert_equal(node.getmempoolinfo()["size"], 0)

    def test_fee_dependency_evictions(self):
        node = self.nodes[0]
        self.generate(self.wallet, 120) # blocks generated for inputs
        self.log.info("Test that 0-fee, no-longer-bumped transactions are removed in RBF")
        packages = []
        confirmed_utxos = []
        for _ in range(5):
            package_hex = []
            child_utxos = []
            confirmed_utxo = self.wallet.get_utxo(mark_as_spent=True)
            confirmed_utxos.append(confirmed_utxo)
            for _ in range(24):
                parent = self.wallet.create_self_transfer(fee=0, fee_rate=0)
                child_utxos.append(parent["new_utxo"])
                package_hex.append(parent["hex"])
            child = self.wallet.create_self_transfer_multi(utxos_to_spend=child_utxos + [confirmed_utxo], fee_per_output=8000)
            package_hex.append(child["hex"])
            packages.append(package_hex)
        child_replacer = self.wallet.create_self_transfer_multi(utxos_to_spend=confirmed_utxos, fee_per_output=9000*5)

        self.generate(node, 100)
        assert_equal(node.getmempoolinfo()["size"], 0)
        for package_hex in packages:
            node.submitpackage(package_hex)
        prev_size = node.getmempoolinfo()["size"]
        assert_equal(prev_size, 5 * 25)
        node.sendrawtransaction(child_replacer["hex"])
        after_size = node.getmempoolinfo()["size"]
        assert_equal(after_size, 1)
        # This replacement evicted more than 100 transactions.
        assert_greater_than(prev_size - (after_size - 1), 100)
        self.generate(node, 1)

    def test_deprioritsation(self):
        node = self.nodes[0]
        self.log.info("Test that de-prioritisation of a transaction can cause eviction from mempool")
        tx_deprio = self.wallet.send_self_transfer(from_node=node)
        assert_equal([tx_deprio["txid"]], node.getrawmempool())
        node.prioritisetransaction(tx_deprio["txid"], 0, -999999)
        tx_new = self.wallet.send_self_transfer(from_node=node)
        assert_equal([tx_new["txid"]], node.getrawmempool())


    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.test_low_feerate_mined()
        self.test_fee_dependency_evictions()
        self.test_deprioritsation()


if __name__ == "__main__":
    MempoolMinRelayFeerate().main()
