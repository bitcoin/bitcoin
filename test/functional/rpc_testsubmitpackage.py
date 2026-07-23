#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Functional tests for the testsubmitpackage RPC.

testsubmitpackage includes a client-side pre-flight check for a 1p1c CPFP package:
[parent, child], where the parent currently fails individual mempool acceptance
for fee-related reasons (or is already in the mempool). It runs the full
package-relay policy in test_accept mode and never mutates the mempool.

"""

import copy
from decimal import Decimal
from hashlib import sha256

from test_framework.key import ECKey
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    MAX_BIP125_RBF_SEQUENCE,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_CHECKSIG,
    OP_TRUE,
    sign_input_segwitv0,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    COIN,
    DEFAULT_FEE,
    MiniWallet,
)


def _mempool_snapshot(node):
    """Capture observable mempool state to assert it's unchanged across a call."""
    info = node.getmempoolinfo()
    return {
        "raw": sorted(node.getrawmempool()),
        "size": info["size"],
        "bytes": info["bytes"],
        "total_fee": info["total_fee"],
    }


class TestSubmitPackageTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 200)

        self.test_input_validation()
        self.test_topology_rejected()
        self.test_parent_passes_individually_rejected()
        self.test_basic_1p1c_cpfp()
        self.test_no_mempool_mutation_on_success()
        self.test_parent_already_in_mempool()
        self.test_parent_diff_wtxid_in_mempool()
        self.test_both_txs_already_in_mempool()
        self.test_maxfeerate_in_parent_in_mempool_branch()
        self.test_parent_fails_non_fee_reason()
        self.test_basic_rbf_via_child()
        self.test_rbf_insufficient_fee()
        self.test_truc_1p1c_cpfp()
        self.test_truc_version_mismatch()
        self.test_insufficient_package_feerate()
        self.test_truc_sibling_eviction()
        self.test_single_tx_basic()
        self.test_single_tx_rbf()
        self.test_single_tx_truc_sibling_eviction()
        self.test_single_tx_already_in_mempool()
        self.test_maxfeerate_exceeded()
        self.test_maxburnamount_check()

    def test_input_validation(self):
        self.log.info("Empty and oversize packages are rejected; size 1 and 2 are accepted")
        node = self.nodes[0]

        assert_raises_rpc_error(-8, "Package must contain 1 or 2 transactions",
                                node.testsubmitpackage, [])
        # Three or more txs is rejected, even if topologically a valid child-with-parents.
        p1 = self.wallet.create_self_transfer()
        p2 = self.wallet.create_self_transfer()
        c = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[p1["new_utxo"], p2["new_utxo"]],
            fee_per_output=int(DEFAULT_FEE * COIN),
        )
        assert_raises_rpc_error(-8, "Package must contain 1 or 2 transactions",
                                node.testsubmitpackage, [p1["hex"], p2["hex"], c["hex"]])

        # Garbage hex is rejected with a decode error.
        assert_raises_rpc_error(-22, "TX decode failed",
                                node.testsubmitpackage, ["deadbeef"])

    def test_topology_rejected(self):
        self.log.info("Two unrelated txs (no parent->child edge) are rejected")
        node = self.nodes[0]
        a = self.wallet.create_self_transfer()
        b = self.wallet.create_self_transfer()
        # b does not spend a; topology check fails.
        assert_raises_rpc_error(-25, "package topology disallowed",
                                node.testsubmitpackage, [a["hex"], b["hex"]])

    def test_parent_passes_individually_rejected(self):
        self.log.info("Parent above minfee is rejected (caller should use testsubmitpackage with parent alone)")
        node = self.nodes[0]
        # Both parent and child pay normal fees: parent would be accepted on its own.
        parent = self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        assert_raises_rpc_error(-26, "does not require CPFP",
                                node.testsubmitpackage, [parent["hex"], child["hex"]])

        assert_equal(node.testsubmitpackage([parent["hex"]])["package_msg"], "success")
        # No rbf, should work here too
        assert node.testmempoolaccept([parent["hex"]])[0]["allowed"]

    def _make_1p1c_cpfp(self, *, child_fee=Decimal("0.0001"), parent_kwargs=None, child_kwargs=None):
        """Build a 1p1c bundle where the parent pays zero fee and the child carries the package.

        Returns a dict with parent, child, hex list, and snapshot of starting mempool.
        """
        parent_kwargs = parent_kwargs or {}
        child_kwargs = child_kwargs or {}
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), fee=Decimal("0"), **parent_kwargs)
        child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"],
            fee=child_fee,
            **child_kwargs,
        )
        return parent, child

    def test_basic_1p1c_cpfp(self):
        self.log.info("Basic 1p1c CPFP: parent fee=0, child pays for both")
        node = self.nodes[0]
        parent, child = self._make_1p1c_cpfp()

        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        assert_equal(result["package_msg"], "success")

        # Both txs have an entry, child has an effective-feerate covering both.
        assert_equal(set(result["tx-results"].keys()), {parent["wtxid"], child["wtxid"]})
        parent_entry = result["tx-results"][parent["wtxid"]]
        child_entry = result["tx-results"][child["wtxid"]]
        assert "error" not in parent_entry, parent_entry
        assert "error" not in child_entry, child_entry

        # The child's effective-includes should cover both wtxids (package-feerate aggregation).
        eff = child_entry["fees"].get("effective-includes", [])
        assert set(eff) == {parent["wtxid"], child["wtxid"]}, eff
        assert_greater_than(child_entry["fees"]["effective-feerate"], Decimal("0"))
        # No replacements happened in this clean accept.
        assert_equal(result["replaced-transactions"], [])

    def test_no_mempool_mutation_on_success(self):
        self.log.info("Successful testsubmitpackage leaves the mempool unchanged")
        node = self.nodes[0]
        parent, child = self._make_1p1c_cpfp()
        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        assert_equal(result["package_msg"], "success")
        after = _mempool_snapshot(node)
        assert_equal(before, after)
        assert parent["txid"] not in node.getrawmempool()
        assert child["txid"] not in node.getrawmempool()

    def test_parent_already_in_mempool(self):
        self.log.info("Parent already in mempool: testsubmitpackage validates the child alone")
        node = self.nodes[0]
        parent = self.wallet.create_self_transfer()
        node.sendrawtransaction(parent["hex"])
        assert parent["txid"] in node.getrawmempool()

        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        # Parent shows up as MEMPOOL_ENTRY: vsize + base fee, but no effective-feerate.
        parent_entry = result["tx-results"][parent["wtxid"]]
        assert_equal(parent_entry["txid"], parent["txid"])
        assert "vsize" in parent_entry
        assert "base" in parent_entry["fees"]
        assert "effective-feerate" not in parent_entry["fees"]
        # Child shows up as VALID.
        child_entry = result["tx-results"][child["wtxid"]]
        assert "error" not in child_entry, child_entry
        assert "effective-feerate" in child_entry["fees"]

        # Mempool state is identical -- child was not added.
        assert_equal(before, after)
        assert child["txid"] not in node.getrawmempool()

        self.generate(self.wallet, 1)

    def test_both_txs_already_in_mempool(self):
        self.log.info("Both txs already in mempool: dedup reports success, no re-validation")
        node = self.nodes[0]
        parent = self.wallet.create_self_transfer()
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])
        node.sendrawtransaction(parent["hex"])
        node.sendrawtransaction(child["hex"])
        assert parent["txid"] in node.getrawmempool()
        assert child["txid"] in node.getrawmempool()

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        # Both report as MempoolTx: vsize + base fee, no effective-feerate.
        for tx in (parent, child):
            entry = result["tx-results"][tx["wtxid"]]
            assert "vsize" in entry
            assert "base" in entry["fees"]
            assert "effective-feerate" not in entry["fees"]
            assert "error" not in entry
        assert_equal(result["replaced-transactions"], [])
        assert_equal(before, after)

        self.generate(self.wallet, 1)

    def test_maxfeerate_in_parent_in_mempool_branch(self):
        self.log.info("maxfeerate is enforced on the child when parent is already in mempool")
        node = self.nodes[0]
        parent = self.wallet.create_self_transfer()
        node.sendrawtransaction(parent["hex"])
        # Child pays an absurd fee (0.5 BTC on ~110 vbytes), well above default maxfeerate.
        child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"], fee=Decimal("0.5"),
        )

        # Default maxfeerate (0.10 BTC/kvB) -> child rejected with "max feerate exceeded".
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        assert_equal(result["package_msg"], "transaction failed")
        child_entry = result["tx-results"][child["wtxid"]]
        assert "max feerate exceeded" in child_entry.get("error", ""), child_entry

        # With maxfeerate=0 (disabled), the same package goes through.
        result_ok = node.testsubmitpackage([parent["hex"], child["hex"]], maxfeerate=0)
        assert_equal(result_ok["package_msg"], "success")

        self.generate(self.wallet, 1)

    def test_parent_diff_wtxid_in_mempool(self):
        """Same-txid-different-wtxid parent already in mempool.

        We fund a P2WSH(<pk> OP_CHECKSIG) script, then sign the spending tx twice
        using random ECDSA nonces -- yielding two valid signatures and hence two
        wtxids for the same txid. One version is submitted via sendrawtransaction;
        we then run testsubmitpackage with the other version as the parent. The
        RPC must hit the MempoolTxDifferentWitness branch and report `other-wtxid`
        pointing at the in-mempool version.
        """
        self.log.info("Parent in mempool under a different wtxid: other-wtxid reported")
        node = self.nodes[0]

        # Build a P2WSH script that requires an ECDSA signature.
        key = ECKey()
        key.generate()
        pubkey = key.get_pubkey().get_bytes()
        witness_script = CScript([pubkey, OP_CHECKSIG])
        wsh_program = sha256(bytes(witness_script)).digest()
        wsh_spk = CScript([OP_0, wsh_program])

        # Fund the script via MiniWallet, then mine so it's a confirmed output.
        funding = self.wallet.send_to(
            from_node=node, scriptPubKey=wsh_spk, amount=int(Decimal("0.5") * COIN),
        )
        self.generate(self.wallet, 1)
        fund_value = int(Decimal("0.5") * COIN)
        fund_txid_int = int(funding["txid"], 16)
        fund_vout = funding["sent_vout"]

        # Build the spending tx (= our "parent") template: unsigned, witness stack
        # pre-populated with just the witness script. The signature gets inserted
        # at index 0 by sign_input_segwitv0.
        op_true_program = sha256(bytes(CScript([OP_TRUE]))).digest()
        op_true_spk = CScript([OP_0, op_true_program])
        parent_template = CTransaction()
        parent_template.vin.append(CTxIn(COutPoint(fund_txid_int, fund_vout)))
        parent_out_value = fund_value - 5000  # leave 5000 sat fee
        parent_template.vout.append(CTxOut(parent_out_value, op_true_spk))
        parent_template.wit.vtxinwit.append(CTxInWitness())
        parent_template.wit.vtxinwit[0].scriptWitness.stack = [bytes(witness_script)]

        # Sign two copies with independent random nonces.
        parent_A = copy.deepcopy(parent_template)
        sign_input_segwitv0(parent_A, 0, bytes(witness_script), fund_value, key)
        parent_B = copy.deepcopy(parent_template)
        sign_input_segwitv0(parent_B, 0, bytes(witness_script), fund_value, key)

        # Same txid, different wtxid.
        assert_equal(parent_A.txid_hex, parent_B.txid_hex)
        assert parent_A.wtxid_hex != parent_B.wtxid_hex, (
            "expected different witnesses to yield different wtxids"
        )

        # Submit version A to the mempool.
        node.sendrawtransaction(parent_A.serialize().hex())
        assert parent_A.txid_hex in node.getrawmempool()

        # Build a child that spends parent's output (P2WSH(OP_TRUE) -> witness is just the OP_TRUE script).
        child = CTransaction()
        child.vin.append(CTxIn(COutPoint(int(parent_A.txid_hex, 16), 0)))
        child.vout.append(CTxOut(parent_out_value - 5000, op_true_spk))
        child.wit.vtxinwit.append(CTxInWitness())
        child.wit.vtxinwit[0].scriptWitness.stack = [bytes(CScript([OP_TRUE]))]

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent_B.serialize().hex(), child.serialize().hex()])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        # Parent (version B) reports other-wtxid pointing at version A.
        parent_entry = result["tx-results"][parent_B.wtxid_hex]
        assert_equal(parent_entry["txid"], parent_B.txid_hex)
        assert_equal(parent_entry["other-wtxid"], parent_A.wtxid_hex)
        assert "vsize" not in parent_entry  # DIFFERENT_WITNESS result has no fee/vsize fields.
        # Child validates successfully against the in-mempool parent.
        child_entry = result["tx-results"][child.wtxid_hex]
        assert "error" not in child_entry, child_entry
        assert "effective-feerate" in child_entry["fees"]

        # Mempool unchanged (parent_A still in, child not added).
        assert_equal(before, after)
        assert parent_A.txid_hex in node.getrawmempool()
        assert child.txid_hex not in node.getrawmempool()

        self.generate(self.wallet, 1)

    def test_parent_fails_non_fee_reason(self):
        self.log.info("Parent that fails for a non-fee reason produces a submitpackage-shaped failure")
        node = self.nodes[0]
        # Parent spends a non-existent coin -> bad-txns-inputs-missingorspent.
        bogus = {"txid": "00" * 32, "vout": 0, "value": Decimal("1.0"), "height": 0}
        parent = self.wallet.create_self_transfer(utxo_to_spend=bogus)
        child = self.wallet.create_self_transfer(utxo_to_spend=parent["new_utxo"])

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "transaction failed")
        parent_entry = result["tx-results"][parent["wtxid"]]
        child_entry = result["tx-results"][child["wtxid"]]
        # AcceptPackage tries both txs individually; both report their real failure reasons.
        # Parent fails with missing inputs; child fails the same way since it references parent's
        # outputs which don't exist.
        assert "missing" in parent_entry.get("error", ""), parent_entry
        assert "missing" in child_entry.get("error", ""), child_entry
        assert_equal(before, after)

    def test_basic_rbf_via_child(self):
        self.log.info("Child of the package RBFs an existing mempool tx")
        node = self.nodes[0]
        # Victim: a low-fee mempool tx that the package's child will displace.
        victim_coin = self.wallet.get_utxo(confirmed_only=True)
        victim = self.wallet.create_self_transfer(
            utxo_to_spend=victim_coin,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee=Decimal("0.00001"),
        )
        node.sendrawtransaction(victim["hex"])
        assert victim["txid"] in node.getrawmempool()

        # 1p1c bundle: sub-minfee parent + child that also spends victim_coin (RBF).
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), fee=Decimal("0"))
        child = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[parent["new_utxo"], victim_coin],
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee_per_output=int(Decimal("0.01") * COIN),
        )
        # First, sanity-check the parent fails on its own (sub-minfee).
        single = node.testmempoolaccept([parent["hex"]])[0]
        assert not single["allowed"]

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        assert_equal(result["replaced-transactions"], [victim["txid"]])
        # Mempool unchanged: victim still there, replacement and child are not.
        assert_equal(before, after)
        assert victim["txid"] in node.getrawmempool()
        assert child["txid"] not in node.getrawmempool()

        self.generate(self.wallet, 1)

    def test_rbf_insufficient_fee(self):
        self.log.info("Package RBF rejected when total fee doesn't beat the conflict")
        node = self.nodes[0]
        # Victim: high-fee mempool tx that the package's child will conflict with.
        victim_coin = self.wallet.get_utxo(confirmed_only=True)
        victim = self.wallet.create_self_transfer(
            utxo_to_spend=victim_coin,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee=Decimal("0.01"),  # 1,000,000 sat
        )
        node.sendrawtransaction(victim["hex"])
        assert victim["txid"] in node.getrawmempool()

        # 1p1c bundle: parent fee=0, child spending both parent_output AND victim_coin
        # with TOTAL fee < victim's fee. PackageRBFChecks will reject.
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), fee=Decimal("0"))
        child = self.wallet.create_self_transfer_multi(
            utxos_to_spend=[parent["new_utxo"], victim_coin],
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee_per_output=int(Decimal("0.005") * COIN),  # 500,000 sat total
        )

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        # Package-level RBF failure: package_msg carries the package-RBF failure reason.
        # Per-tx entries carry their individual failure errors (populated by AcceptPackage's
        # per-tx-first pass), so the caller sees both views.
        assert "package RBF failed" in result["package_msg"], result["package_msg"]
        for entry in result["tx-results"].values():
            assert "error" in entry, entry
        assert_equal(result["replaced-transactions"], [])
        # Victim still in mempool, package not added.
        assert victim["txid"] in node.getrawmempool()
        assert_equal(before, after)

        self.generate(self.wallet, 1)

    def test_truc_1p1c_cpfp(self):
        self.log.info("TRUC 1p1c CPFP: sub-minfee TRUC parent + TRUC child via package eval")
        node = self.nodes[0]
        # Parent: TRUC (version=3) with fee=0.
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), fee=Decimal("0"), version=3)
        # Child: TRUC, paying enough fee to lift the package above the floating minfee.
        child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"], fee=Decimal("0.0001"), version=3,
        )

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        assert_equal(set(result["tx-results"].keys()), {parent["wtxid"], child["wtxid"]})
        child_entry = result["tx-results"][child["wtxid"]]
        assert set(child_entry["fees"]["effective-includes"]) == {parent["wtxid"], child["wtxid"]}
        assert_equal(before, after)

    def test_truc_version_mismatch(self):
        self.log.info("Non-TRUC parent + TRUC child: TRUC-violation surfaced in result")
        node = self.nodes[0]
        # Parent: non-TRUC (default version=2) with fee=0.
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), fee=Decimal("0"))
        # Child: TRUC spending non-TRUC parent -- forbidden by TRUC rules.
        child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"], fee=Decimal("0.0001"), version=3,
        )

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        # PackageTRUCChecks fails at the package level, so package_msg carries the violation
        # reason. Per-tx entries carry their individual errors from AcceptPackage's per-tx
        # pass (both fail with min-relay-fee since the parent is sub-minfee and the child
        # references a non-existent-yet parent output).
        assert "TRUC-violation" in result["package_msg"], result["package_msg"]
        for entry in result["tx-results"].values():
            assert "error" in entry, entry
        assert_equal(before, after)

    def test_insufficient_package_feerate(self):
        self.log.info("Child fee too low to lift parent over minfee: package fails")
        node = self.nodes[0]
        parent = self.wallet.create_self_transfer(fee_rate=Decimal("0"), fee=Decimal("0"))
        # Child fee 1 sat -> package feerate well below 1 sat/vbyte minrelay.
        child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxo"], fee=Decimal("0.00000001"),
        )

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "transaction failed")
        # The last tx in the package gets a fee-failure entry per AcceptMultipleTransactionsInternal.
        child_entry = result["tx-results"][child["wtxid"]]
        assert "error" in child_entry, child_entry
        assert "fee" in child_entry["error"].lower(), child_entry
        assert_equal(before, after)

    def test_truc_sibling_eviction(self):
        self.log.info("TRUC sibling eviction triggers in the parent-in-mempool branch")
        node = self.nodes[0]
        # TRUC parent in mempool with two outputs.
        parent = self.wallet.send_self_transfer_multi(
            from_node=node, num_outputs=2, version=3,
        )
        # Existing TRUC child to be sibling-evicted.
        sibling = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=parent["new_utxos"][0],
            fee_rate=DEFAULT_FEE * 2,
            version=3,
        )
        assert sibling["txid"] in node.getrawmempool()

        # New TRUC child of the same parent, paying enough to evict the sibling.
        new_child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxos"][1],
            fee_rate=DEFAULT_FEE * 10,
            version=3,
        )

        # The "package" is [parent (already in mempool), new_child]. Our RPC's
        # parent-in-mempool branch validates the child via path which
        # allows sibling eviction
        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([parent["hex"], new_child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        assert_equal(result["replaced-transactions"], [sibling["txid"]])
        # Mempool unchanged: the original sibling is still there.
        assert_equal(before, after)
        assert sibling["txid"] in node.getrawmempool()
        assert new_child["txid"] not in node.getrawmempool()

        self.generate(self.wallet, 1)

    def test_single_tx_basic(self):
        self.log.info("Single tx: clean accept routes through SingleInPackageAccept")
        node = self.nodes[0]
        tx = self.wallet.create_self_transfer()

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([tx["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        assert_equal(set(result["tx-results"].keys()), {tx["wtxid"]})
        entry = result["tx-results"][tx["wtxid"]]
        assert_equal(entry["txid"], tx["txid"])
        assert "error" not in entry, entry
        assert "vsize" in entry
        assert "effective-feerate" in entry["fees"]
        # Single-tx effective-includes is just this tx.
        assert_equal(entry["fees"]["effective-includes"], [tx["wtxid"]])
        assert_equal(result["replaced-transactions"], [])
        assert_equal(before, after)
        assert tx["txid"] not in node.getrawmempool()

    def test_single_tx_rbf(self):
        self.log.info("Single tx replaces a mempool victim via RBF; replaced-transactions reports the victim")
        node = self.nodes[0]
        coin = self.wallet.get_utxo(confirmed_only=True)
        victim = self.wallet.create_self_transfer(
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee=Decimal("0.00001"),
        )
        node.sendrawtransaction(victim["hex"])
        assert victim["txid"] in node.getrawmempool()

        replacement = self.wallet.create_self_transfer(
            utxo_to_spend=coin,
            sequence=MAX_BIP125_RBF_SEQUENCE,
            fee=Decimal("0.001"),
        )

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([replacement["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        assert_equal(result["replaced-transactions"], [victim["txid"]])
        # Mempool unchanged: victim still there, replacement not added.
        assert victim["txid"] in node.getrawmempool()
        assert replacement["txid"] not in node.getrawmempool()
        assert_equal(before, after)

        self.generate(self.wallet, 1)

    def test_single_tx_truc_sibling_eviction(self):
        self.log.info("Single TRUC tx triggers sibling eviction against an in-mempool sibling")
        node = self.nodes[0]
        parent = self.wallet.send_self_transfer_multi(
            from_node=node, num_outputs=2, version=3,
        )
        sibling = self.wallet.send_self_transfer(
            from_node=node,
            utxo_to_spend=parent["new_utxos"][0],
            fee_rate=DEFAULT_FEE * 2,
            version=3,
        )
        assert sibling["txid"] in node.getrawmempool()

        new_child = self.wallet.create_self_transfer(
            utxo_to_spend=parent["new_utxos"][1],
            fee_rate=DEFAULT_FEE * 10,
            version=3,
        )

        before = _mempool_snapshot(node)
        result = node.testsubmitpackage([new_child["hex"]])
        after = _mempool_snapshot(node)

        assert_equal(result["package_msg"], "success")
        assert_equal(result["replaced-transactions"], [sibling["txid"]])
        assert sibling["txid"] in node.getrawmempool()
        assert new_child["txid"] not in node.getrawmempool()
        assert_equal(before, after)

        self.generate(self.wallet, 1)

    def test_single_tx_already_in_mempool(self):
        self.log.info("Single tx already in mempool: dedup reports MempoolTx success")
        node = self.nodes[0]
        tx = self.wallet.create_self_transfer()
        node.sendrawtransaction(tx["hex"])
        assert tx["txid"] in node.getrawmempool()

        result = node.testsubmitpackage([tx["hex"]])
        assert_equal(result["package_msg"], "success")
        entry = result["tx-results"][tx["wtxid"]]
        # MempoolTx: vsize + base fee, no effective-feerate.
        assert "vsize" in entry
        assert "base" in entry["fees"]
        assert "effective-feerate" not in entry["fees"]
        assert "error" not in entry, entry

        self.generate(self.wallet, 1)

    def test_maxfeerate_exceeded(self):
        self.log.info("maxfeerate clamps the child's effective feerate")
        node = self.nodes[0]
        parent, child = self._make_1p1c_cpfp(child_fee=Decimal("0.5"))
        # 0.5 BTC fee on ~110vbytes is way above the default maxfeerate 0.10 BTC/kvB.
        result = node.testsubmitpackage([parent["hex"], child["hex"]])
        assert_equal(result["package_msg"], "transaction failed")
        child_entry = result["tx-results"][child["wtxid"]]
        assert "max feerate exceeded" in child_entry.get("error", ""), child_entry

        # With maxfeerate=0 (disabled), the same package goes through.
        result_ok = node.testsubmitpackage([parent["hex"], child["hex"]], maxfeerate=0)
        assert_equal(result_ok["package_msg"], "success")

    def test_maxburnamount_check(self):
        self.log.info("maxburnamount is enforced at decode time")
        from test_framework.messages import tx_from_hex
        node = self.nodes[0]
        # Build a chained 1p1c, then mutate the parent's last output into an unspendable
        # scriptPubKey carrying the full output value. This trips the burn-amount check
        # at decode time, before any validation runs.
        chained = self.wallet.create_self_transfer_chain(
            chain_length=2,
            utxo_to_spend=self.wallet.get_utxo(confirmed_only=True),
        )
        parent_hex = chained[0]["hex"]
        parent_tx = tx_from_hex(parent_hex)
        parent_tx.vout[-1].scriptPubKey = b'a' * 10001  # >10k bytes -> IsUnspendable
        burned_parent_hex = parent_tx.serialize().hex()
        # Child's hex is irrelevant: parent fails decode-time burn check first.
        assert_raises_rpc_error(-25, "Unspendable output exceeds maximum configured by user",
                                node.testsubmitpackage, [burned_parent_hex, chained[1]["hex"]])


if __name__ == "__main__":
    TestSubmitPackageTest(__file__).main()
