#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""RPCs that handle raw transaction packages."""

from decimal import Decimal
import random

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.mempool_util import (
    fill_mempool,
)
from test_framework.messages import (
    MAX_BIP125_RBF_SEQUENCE,
    tx_from_hex,
)
from test_framework.p2p import P2PTxInvStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_fee_amount,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)


MAX_PACKAGE_COUNT = 25


class RPCPackagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

    def assert_testres_equal(self, package_hex, testres_expected):
        """Shuffle package_hex and assert that the testmempoolaccept result matches testres_expected. This should only
        be used to test packages where the order does not matter. The ordering of transactions in package_hex and
        testres_expected must match.
        """
        shuffled_indices = list(range(len(package_hex)))
        random.shuffle(shuffled_indices)
        shuffled_package = [package_hex[i] for i in shuffled_indices]
        shuffled_testres = [testres_expected[i] for i in shuffled_indices]
        assert_equal(shuffled_testres, self.nodes[0].testmempoolaccept(shuffled_package))

    def run_test(self):
        node = self.nodes[0]

        # get an UTXO that requires signature to be spent
        deterministic_address = node.get_deterministic_priv_key().address
        blockhash = self.generatetoaddress(node, 1, deterministic_address)[0]
        coinbase = node.getblock(blockhash=blockhash, verbosity=2)["tx"][0]
        coin = {
                "txid": coinbase["txid"],
                "amount": coinbase["vout"][0]["value"],
                "scriptPubKey": coinbase["vout"][0]["scriptPubKey"],
                "vout": 0,
                "height": 0
            }

        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, COINBASE_MATURITY + 100)  # blocks generated for inputs

        self.log.info("Create some transactions")
        # Create some transactions that can be reused throughout the test. Never submit these to mempool.
        self.independent_txns_hex = []
        self.independent_txns_testres = []
        for _ in range(3):
            tx_hex = self.wallet.create_self_transfer(fee_rate=Decimal("0.0001"))["hex"]
            testres = self.nodes[0].testmempoolaccept([tx_hex])
            assert testres[0]["allowed"]
            self.independent_txns_hex.append(tx_hex)
            # testmempoolaccept returns a list of length one, avoid creating a 2D list
            self.independent_txns_testres.append(testres[0])
        self.independent_txns_testres_blank = [{
            "txid": res["txid"], "wtxid": res["wtxid"]} for res in self.independent_txns_testres]

        self.test_submitpackage_with_ancestors()
        self.test_independent(coin)
        self.test_chain()
        self.test_multiple_children()
        self.test_multiple_parents()
        self.test_conflicting()
        self.test_rbf()
        self.test_submitpackage()
        self.test_maxfeerate_submitpackage()
        self.test_maxburn_submitpackage()

    def test_independent(self, coin):
        self.log.info("Test multiple independent transactions in a package")
        node = self.nodes[0]
        # For independent transactions, order doesn't matter.
        self.assert_testres_equal(self.independent_txns_hex, self.independent_txns_testres)

        self.log.info("Test an otherwise valid package with an extra garbage tx appended")
        address = node.get_deterministic_priv_key().address
        garbage_tx = node.createrawtransaction([{"txid": "00" * 32, "vout": 5}], {address: 1})
        tx = tx_from_hex(garbage_tx)
        # Only the txid and wtxids are returned because validation is incomplete for the independent txns.
        # Package validation is atomic: if the node cannot find a UTXO for any single tx in the package,
        # it terminates immediately to avoid unnecessary, expensive signature verification.
        package_bad = self.independent_txns_hex + [garbage_tx]
        testres_bad = self.independent_txns_testres_blank + [{"txid": tx.txid_hex, "wtxid": tx.wtxid_hex, "allowed": False, "reject-reason": "missing-inputs"}]
        self.assert_testres_equal(package_bad, testres_bad)

        self.log.info("Check testmempoolaccept tells us when some transactions completed validation successfully")
        tx_bad_sig_hex = node.createrawtransaction([{"txid": coin["txid"], "vout": coin["vout"]}],
                                           {address : coin["amount"] - Decimal("0.0001")})
        tx_bad_sig = tx_from_hex(tx_bad_sig_hex)
        testres_bad_sig = node.testmempoolaccept(self.independent_txns_hex + [tx_bad_sig_hex])
        # By the time the signature for the last transaction is checked, all the other transactions
        # have been fully validated, which is why the node returns full validation results for all
        # transactions here but empty results in other cases.
        tx_bad_sig_txid = tx_bad_sig.txid_hex
        tx_bad_sig_wtxid = tx_bad_sig.wtxid_hex
        assert_equal(testres_bad_sig, self.independent_txns_testres + [{
            "txid": tx_bad_sig_txid,
            "wtxid": tx_bad_sig_wtxid, "allowed": False,
            "reject-reason": "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)",
            "reject-details": "mandatory-script-verify-flag-failed (Operation not valid with the current stack size), " +
                              f"input 0 of {tx_bad_sig_txid} (wtxid {tx_bad_sig_wtxid}), spending {coin['txid']}:{coin['vout']}"
        }])

        self.log.info("Check testmempoolaccept reports txns in packages that exceed max feerate")
        tx_high_fee = self.wallet.create_self_transfer(fee=Decimal("0.999"))
        testres_high_fee = node.testmempoolaccept([tx_high_fee["hex"]])
        assert_equal(testres_high_fee, [
            {"txid": tx_high_fee["txid"], "wtxid": tx_high_fee["wtxid"], "allowed": False, "reject-reason": "max-fee-exceeded"}
        ])
        package_high_fee = [tx_high_fee["hex"]] + self.independent_txns_hex
        testres_package_high_fee = node.testmempoolaccept(package_high_fee)
        assert_equal(testres_package_high_fee, testres_high_fee + self.independent_txns_testres_blank)

    def test_chain(self):
        node = self.nodes[0]

        chain = self.wallet.create_self_transfer_chain(chain_length=25)
        chain_hex = [t["hex"] for t in chain]
        chain_txns = [t["tx"] for t in chain]

        self.log.info("Check that testmempoolaccept requires packages to be sorted by dependency")
        assert_equal(node.testmempoolaccept(rawtxs=chain_hex[::-1]),
                [{"txid": tx.txid_hex, "wtxid": tx.wtxid_hex, "package-error": "package-not-sorted"} for tx in chain_txns[::-1]])

        self.log.info("Testmempoolaccept a chain of 25 transactions")
        testres_multiple = node.testmempoolaccept(rawtxs=chain_hex)

        testres_single = []
        # Test accept and then submit each one individually, which should be identical to package test accept
        for rawtx in chain_hex:
            testres = node.testmempoolaccept([rawtx])
            testres_single.append(testres[0])
            # Submit the transaction now so its child should have no problem validating
            node.sendrawtransaction(rawtx)
        assert_equal(testres_single, testres_multiple)

        # Clean up by clearing the mempool
        self.generate(node, 1)

    def test_multiple_children(self):
        node = self.nodes[0]
        self.log.info("Testmempoolaccept a package in which a transaction has two children within the package")

        parent_tx = self.wallet.create_self_transfer_multi(num_outputs=2)
        assert node.testmempoolaccept([parent_tx["hex"]])[0]["allowed"]

        # Child A
        child_a_tx = self.wallet.create_self_transfer(utxo_to_spend=parent_tx["new_utxos"][0])
        assert not node.testmempoolaccept([child_a_tx["hex"]])[0]["allowed"]

        # Child B
        child_b_tx = self.wallet.create_self_transfer(utxo_to_spend=parent_tx["new_utxos"][1])
        assert not node.testmempoolaccept([child_b_tx["hex"]])[0]["allowed"]

        self.log.info("Testmempoolaccept with entire package, should work with children in either order")
        testres_multiple_ab = node.testmempoolaccept(rawtxs=[parent_tx["hex"], child_a_tx["hex"], child_b_tx["hex"]])
        testres_multiple_ba = node.testmempoolaccept(rawtxs=[parent_tx["hex"], child_b_tx["hex"], child_a_tx["hex"]])
        assert all([testres["allowed"] for testres in testres_multiple_ab + testres_multiple_ba])

        testres_single = []
        # Test accept and then submit each one individually, which should be identical to package testaccept
        for rawtx in [parent_tx["hex"], child_a_tx["hex"], child_b_tx["hex"]]:
            testres = node.testmempoolaccept([rawtx])
            testres_single.append(testres[0])
            # Submit the transaction now so its child should have no problem validating
            node.sendrawtransaction(rawtx)
        assert_equal(testres_single, testres_multiple_ab)

    def test_multiple_parents(self):
        node = self.nodes[0]
        self.log.info("Testmempoolaccept a package in which a transaction has multiple parents within the package")

        for num_parents in [2, 10, 24]:
            # Test a package with num_parents parents and 1 child transaction.
            parent_coins = []
            package_hex = []

            for _ in range(num_parents):
                # Package accept should work with the parents in any order (as long as parents come before child)
                parent_tx = self.wallet.create_self_transfer()
                parent_coins.append(parent_tx["new_utxo"])
                package_hex.append(parent_tx["hex"])

            child_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=parent_coins, fee_per_output=2000)
            for _ in range(10):
                random.shuffle(package_hex)
                testres_multiple = node.testmempoolaccept(rawtxs=package_hex + [child_tx['hex']])
                assert all([testres["allowed"] for testres in testres_multiple])

            testres_single = []
            # Test accept and then submit each one individually, which should be identical to package testaccept
            for rawtx in package_hex + [child_tx["hex"]]:
                testres_single.append(node.testmempoolaccept([rawtx])[0])
                # Submit the transaction now so its child should have no problem validating
                node.sendrawtransaction(rawtx)
            assert_equal(testres_single, testres_multiple)

    def test_conflicting(self):
        node = self.nodes[0]
        coin = self.wallet.get_utxo()

        # tx1 and tx2 share the same inputs
        tx1 = self.wallet.create_self_transfer(utxo_to_spend=coin, fee_rate=DEFAULT_FEE)
        tx2 = self.wallet.create_self_transfer(utxo_to_spend=coin, fee_rate=2*DEFAULT_FEE)

        # Ensure tx1 and tx2 are valid by themselves
        assert node.testmempoolaccept([tx1["hex"]])[0]["allowed"]
        assert node.testmempoolaccept([tx2["hex"]])[0]["allowed"]

        self.log.info("Test duplicate transactions in the same package")
        testres = node.testmempoolaccept([tx1["hex"], tx1["hex"]])
        assert_equal(testres, [
            {"txid": tx1["txid"], "wtxid": tx1["wtxid"], "package-error": "package-contains-duplicates"},
            {"txid": tx1["txid"], "wtxid": tx1["wtxid"], "package-error": "package-contains-duplicates"}
        ])

        self.log.info("Test conflicting transactions in the same package")
        testres = node.testmempoolaccept([tx1["hex"], tx2["hex"]])
        assert_equal(testres, [
            {"txid": tx1["txid"], "wtxid": tx1["wtxid"], "package-error": "conflict-in-package"},
            {"txid": tx2["txid"], "wtxid": tx2["wtxid"], "package-error": "conflict-in-package"}
        ])

        # Add a child that spends both at high feerate to submit via submitpackage
        tx_child = self.wallet.create_self_transfer_multi(
            fee_per_output=int(DEFAULT_FEE * 5 * COIN),
            utxos_to_spend=[tx1["new_utxo"], tx2["new_utxo"]],
        )

        testres = node.testmempoolaccept([tx1["hex"], tx2["hex"], tx_child["hex"]])

        assert_equal(testres, [
            {"txid": tx1["txid"], "wtxid": tx1["wtxid"], "package-error": "conflict-in-package"},
            {"txid": tx2["txid"], "wtxid": tx2["wtxid"], "package-error": "conflict-in-package"},
            {"txid": tx_child["txid"], "wtxid": tx_child["wtxid"], "package-error": "conflict-in-package"}
        ])

        submitres = node.submitpackage([tx1["hex"], tx2["hex"], tx_child["hex"]])
        assert_equal(submitres, {'package_msg': 'conflict-in-package', 'tx-results': {}, 'replaced-transactions': []})

        # Submit tx1 to mempool, then try the same package again
        node.sendrawtransaction(tx1["hex"])

        submitres = node.submitpackage([tx1["hex"], tx2["hex"], tx_child["hex"]])
        assert_equal(submitres, {'package_msg': 'conflict-in-package', 'tx-results': {}, 'replaced-transactions': []})
        assert tx_child["txid"] not in node.getrawmempool()

        # without the in-mempool ancestor tx1 included in the call, tx2 can be submitted, but
        # tx_child is missing an input.
        submitres = node.submitpackage([tx2["hex"], tx_child["hex"]])
        assert_equal(submitres["tx-results"][tx_child["wtxid"]], {"txid": tx_child["txid"], "error": "bad-txns-inputs-missingorspent"})
        assert tx2["txid"] in node.getrawmempool()

        # Regardless of error type, the child can never enter the mempool
        assert tx_child["txid"] not in node.getrawmempool()

    def test_rbf(self):
        node = self.nodes[0]

        coin = self.wallet.get_utxo()
        fee = Decimal("0.00125000")
        replaceable_tx = self.wallet.create_self_transfer(utxo_to_spend=coin, sequence=MAX_BIP125_RBF_SEQUENCE, fee = fee)
        testres_replaceable = node.testmempoolaccept([replaceable_tx["hex"]])[0]
        assert_equal(testres_replaceable["txid"], replaceable_tx["txid"])
        assert_equal(testres_replaceable["wtxid"], replaceable_tx["wtxid"])
        assert testres_replaceable["allowed"]
        assert_equal(testres_replaceable["vsize"], replaceable_tx["tx"].get_vsize())
        assert_equal(testres_replaceable["fees"]["base"], fee)
        assert_fee_amount(fee, replaceable_tx["tx"].get_vsize(), testres_replaceable["fees"]["effective-feerate"])
        assert_equal(testres_replaceable["fees"]["effective-includes"], [replaceable_tx["wtxid"]])

        # Replacement transaction is identical except has double the fee
        replacement_tx = self.wallet.create_self_transfer(utxo_to_spend=coin, sequence=MAX_BIP125_RBF_SEQUENCE, fee = 2 * fee)
        testres_rbf_conflicting = node.testmempoolaccept([replaceable_tx["hex"], replacement_tx["hex"]])
        assert_equal(testres_rbf_conflicting, [
            {"txid": replaceable_tx["txid"], "wtxid": replaceable_tx["wtxid"], "package-error": "conflict-in-package"},
            {"txid": replacement_tx["txid"], "wtxid": replacement_tx["wtxid"], "package-error": "conflict-in-package"}
        ])

        self.log.info("Test that packages cannot conflict with mempool transactions, even if a valid BIP125 RBF")
        # This transaction is a valid BIP125 replace-by-fee
        self.wallet.sendrawtransaction(from_node=node, tx_hex=replaceable_tx["hex"])
        testres_rbf_single = node.testmempoolaccept([replacement_tx["hex"]])
        assert testres_rbf_single[0]["allowed"]
        testres_rbf_package = self.independent_txns_testres_blank + [{
            "txid": replacement_tx["txid"], "wtxid": replacement_tx["wtxid"], "allowed": False,
            "reject-reason": "bip125-replacement-disallowed",
            "reject-details": "bip125-replacement-disallowed"
        }]
        self.assert_testres_equal(self.independent_txns_hex + [replacement_tx["hex"]], testres_rbf_package)

    def assert_equal_package_results(self, node, testmempoolaccept_result, submitpackage_result):
        """Assert that a successful submitpackage result is consistent with testmempoolaccept
        results and getmempoolentry info. Note that the result structs are different and, due to
        policy differences between testmempoolaccept and submitpackage (i.e. package feerate),
        some information may be different.
        """
        for testres_tx in testmempoolaccept_result:
            # Grab this result from the submitpackage_result
            submitres_tx = submitpackage_result["tx-results"][testres_tx["wtxid"]]
            assert_equal(submitres_tx["txid"], testres_tx["txid"])
            # No "allowed" if the tx was already in the mempool
            if "allowed" in testres_tx and testres_tx["allowed"]:
                assert_equal(submitres_tx["vsize"], testres_tx["vsize"])
                assert_equal(submitres_tx["fees"]["base"], testres_tx["fees"]["base"])
            entry_info = node.getmempoolentry(submitres_tx["txid"])
            assert_equal(submitres_tx["vsize"], entry_info["vsize"])
            assert_equal(submitres_tx["fees"]["base"], entry_info["fees"]["base"])

    def test_submit_child_with_parents(self, num_parents, partial_submit):
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PTxInvStore())

        package_txns = []
        presubmitted_wtxids = set()
        for _ in range(num_parents):
            parent_tx = self.wallet.create_self_transfer(fee=DEFAULT_FEE)
            package_txns.append(parent_tx)
            if partial_submit and random.choice([True, False]):
                node.sendrawtransaction(parent_tx["hex"])
                presubmitted_wtxids.add(parent_tx["wtxid"])
        child_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=[tx["new_utxo"] for tx in package_txns], fee_per_output=10000) #DEFAULT_FEE
        package_txns.append(child_tx)

        testmempoolaccept_result = node.testmempoolaccept(rawtxs=[tx["hex"] for tx in package_txns])
        submitpackage_result = node.submitpackage(package=[tx["hex"] for tx in package_txns])

        # Check that each result is present, with the correct size and fees
        assert_equal(submitpackage_result["package_msg"], "success")
        for package_txn in package_txns:
            tx = package_txn["tx"]
            assert tx.wtxid_hex in submitpackage_result["tx-results"]
            wtxid = tx.wtxid_hex
            assert wtxid in submitpackage_result["tx-results"]
            tx_result = submitpackage_result["tx-results"][wtxid]
            assert_equal(tx_result["txid"], tx.txid_hex)
            assert_equal(tx_result["vsize"], tx.get_vsize())
            assert_equal(tx_result["fees"]["base"], DEFAULT_FEE)
            if wtxid not in presubmitted_wtxids:
                assert_fee_amount(DEFAULT_FEE, tx.get_vsize(), tx_result["fees"]["effective-feerate"])
                assert_equal(tx_result["fees"]["effective-includes"], [wtxid])

        # submitpackage result should be consistent with testmempoolaccept and getmempoolentry
        self.assert_equal_package_results(node, testmempoolaccept_result, submitpackage_result)

        # The node should announce each transaction. No guarantees for propagation.
        peer.wait_for_broadcast([tx["tx"].wtxid_hex for tx in package_txns])
        self.generate(node, 1)

    def test_submitpackage(self):
        node = self.nodes[0]

        self.log.info("Submitpackage only allows valid hex inputs")
        valid_tx_list = self.wallet.create_self_transfer_chain(chain_length=2)
        hex_list = [valid_tx_list[0]["hex"][:-1] + 'X', valid_tx_list[1]["hex"]]
        txid_list = [valid_tx_list[0]["txid"], valid_tx_list[1]["txid"]]
        assert_raises_rpc_error(-22, "TX decode failed:", node.submitpackage, hex_list)
        assert txid_list[0] not in node.getrawmempool()
        assert txid_list[1] not in node.getrawmempool()

        self.log.info("Submitpackage valid packages with 1 child and some number of parents (or none)")
        for num_parents in [0, 1, 2, 24]:
            self.test_submit_child_with_parents(num_parents, False)
            self.test_submit_child_with_parents(num_parents, True)

        self.log.info("Submitpackage only allows packages of 1 child with its parents")
        # Chain of 3 transactions has too many generations
        legacy_pool = node.getrawmempool()
        chain_hex = [t["hex"] for t in self.wallet.create_self_transfer_chain(chain_length=3)]
        assert_raises_rpc_error(-25, "package topology disallowed", node.submitpackage, chain_hex)
        assert_equal(legacy_pool, node.getrawmempool())

        assert_raises_rpc_error(-8, f"Array must contain between 1 and {MAX_PACKAGE_COUNT} transactions.", node.submitpackage, [])
        assert_raises_rpc_error(
            -8, f"Array must contain between 1 and {MAX_PACKAGE_COUNT} transactions.",
            node.submitpackage, [chain_hex[0]] * (MAX_PACKAGE_COUNT + 1)
        )

        # Create a transaction chain such as only the parent gets accepted (by making the child's
        # version non-standard). Make sure the parent does get broadcast.
        self.log.info("If a package is partially submitted, transactions included in mempool get broadcast")
        peer = node.add_p2p_connection(P2PTxInvStore())
        txs = self.wallet.create_self_transfer_chain(chain_length=2)
        bad_child = tx_from_hex(txs[1]["hex"])
        bad_child.version = 0xffffffff
        hex_partial_acceptance = [txs[0]["hex"], bad_child.serialize().hex()]
        res = node.submitpackage(hex_partial_acceptance)
        assert_equal(res["package_msg"], "transaction failed")
        first_wtxid = txs[0]["tx"].wtxid_hex
        assert "error" not in res["tx-results"][first_wtxid]
        sec_wtxid = bad_child.wtxid_hex
        assert_equal(res["tx-results"][sec_wtxid]["error"], "version")
        peer.wait_for_broadcast([first_wtxid])

    def test_maxfeerate_submitpackage(self):
        node = self.nodes[0]
        # clear mempool
        deterministic_address = node.get_deterministic_priv_key().address
        self.generatetoaddress(node, 1, deterministic_address)

        self.log.info("Submitpackage maxfeerate arg testing")
        chained_txns = self.wallet.create_self_transfer_chain(chain_length=2)
        minrate_btc_kvb = min([chained_txn["fee"] / chained_txn["tx"].get_vsize() * 1000 for chained_txn in chained_txns])
        chain_hex = [t["hex"] for t in chained_txns]
        pkg_result = node.submitpackage(chain_hex, maxfeerate=minrate_btc_kvb - Decimal("0.00000001"))

        # First tx failed in single transaction evaluation, so package message is generic
        assert_equal(pkg_result["package_msg"], "transaction failed")
        assert_equal(pkg_result["tx-results"][chained_txns[0]["wtxid"]]["error"], "max feerate exceeded")
        assert_equal(pkg_result["tx-results"][chained_txns[1]["wtxid"]]["error"], "bad-txns-inputs-missingorspent")
        assert_equal(node.getrawmempool(), [])

        # Make chain of two transactions where parent doesn't make minfee threshold
        # but child is too high fee
        # Lower mempool limit to make it easier to fill_mempool
        self.restart_node(0, extra_args=[
            "-maxmempool=5",
            "-persistmempool=0",
        ])
        self.wallet.rescan_utxos()

        fill_mempool(self, node)

        minrelay = node.getmempoolinfo()["minrelaytxfee"]
        parent = self.wallet.create_self_transfer(
            fee_rate=minrelay,
            confirmed_only=True,
        )

        child = self.wallet.create_self_transfer(
            fee_rate=DEFAULT_FEE,
            utxo_to_spend=parent["new_utxo"],
        )

        pkg_result = node.submitpackage([parent["hex"], child["hex"]], maxfeerate=DEFAULT_FEE - Decimal("0.00000001"))

        # Child is connected even though parent is invalid and still reports fee exceeded
        # this implies sub-package evaluation of both entries together.
        assert_equal(pkg_result["package_msg"], "transaction failed")
        assert "mempool min fee not met" in pkg_result["tx-results"][parent["wtxid"]]["error"]
        assert_equal(pkg_result["tx-results"][child["wtxid"]]["error"], "max feerate exceeded")
        assert parent["txid"] not in node.getrawmempool()
        assert child["txid"] not in node.getrawmempool()

        # Reset maxmempool, reset dynamic mempool minimum feerate, and empty mempool.
        self.restart_node(0)
        self.wallet.rescan_utxos()

        assert_equal(node.getrawmempool(), [])

    def test_maxburn_submitpackage(self):
        node = self.nodes[0]

        assert_equal(node.getrawmempool(), [])

        self.log.info("Submitpackage maxburnamount arg testing")
        chained_txns_burn = self.wallet.create_self_transfer_chain(
            chain_length=2,
            utxo_to_spend=self.wallet.get_utxo(confirmed_only=True),
        )
        chained_burn_hex = [t["hex"] for t in chained_txns_burn]

        tx = tx_from_hex(chained_burn_hex[1])
        tx.vout[-1].scriptPubKey = b'a' * 10001 # scriptPubKey bigger than 10k IsUnspendable
        chained_burn_hex = [chained_burn_hex[0], tx.serialize().hex()]
        # burn test is run before any package evaluation; nothing makes it in and we get broader exception
        assert_raises_rpc_error(-25, "Unspendable output exceeds maximum configured by user", node.submitpackage, chained_burn_hex, 0, chained_txns_burn[1]["new_utxo"]["value"] - Decimal("0.00000001"))
        assert_equal(node.getrawmempool(), [])

        minrate_btc_kvb_burn = min([chained_txn_burn["fee"] / chained_txn_burn["tx"].get_vsize() * 1000 for chained_txn_burn in chained_txns_burn])

        # Relax the restrictions for both and send it; parent gets through as own subpackage
        pkg_result = node.submitpackage(chained_burn_hex, maxfeerate=minrate_btc_kvb_burn, maxburnamount=chained_txns_burn[1]["new_utxo"]["value"])
        assert "error" not in pkg_result["tx-results"][chained_txns_burn[0]["wtxid"]]
        assert_equal(pkg_result["tx-results"][tx.wtxid_hex]["error"], "scriptpubkey")
        assert_equal(node.getrawmempool(), [chained_txns_burn[0]["txid"]])

    def test_submitpackage_with_ancestors(self):
        self.log.info("Test that submitpackage can send a package that has in-mempool ancestors")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PTxInvStore())

        parent_tx = self.wallet.create_self_transfer()
        child_tx = self.wallet.create_self_transfer(utxo_to_spend=parent_tx["new_utxo"])
        grandchild_tx = self.wallet.create_self_transfer(utxo_to_spend=child_tx["new_utxo"])
        ggrandchild_tx = self.wallet.create_self_transfer(utxo_to_spend=grandchild_tx["new_utxo"])

        # Submitting them all together doesn't work, as the topology is not child-with-parents
        assert_raises_rpc_error(-25, "package topology disallowed", node.submitpackage, [parent_tx["hex"], child_tx["hex"], grandchild_tx["hex"], ggrandchild_tx["hex"]])

        # Submit older package and check acceptance
        result_submit_older = node.submitpackage(package=[parent_tx["hex"], child_tx["hex"]])
        assert_equal(result_submit_older["package_msg"], "success")
        mempool = node.getrawmempool()
        assert parent_tx["txid"] in mempool
        assert child_tx["txid"] in mempool

        # Submit younger package and check acceptance
        result_submit_younger = node.submitpackage(package=[grandchild_tx["hex"], ggrandchild_tx["hex"]])
        assert_equal(result_submit_younger["package_msg"], "success")
        mempool = node.getrawmempool()

        assert parent_tx["txid"] in mempool
        assert child_tx["txid"] in mempool
        assert grandchild_tx["txid"] in mempool
        assert ggrandchild_tx["txid"] in mempool

        # The node should announce each transaction.
        peer.wait_for_broadcast([tx["tx"].wtxid_hex for tx in [parent_tx, child_tx, grandchild_tx, ggrandchild_tx]])
        self.generate(node, 1)


if __name__ == "__main__":
    RPCPackagesTest(__file__).main()
