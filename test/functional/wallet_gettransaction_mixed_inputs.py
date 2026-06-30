#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.
"""Test wallet history RPC accounting for mixed-input transactions."""

from decimal import Decimal

from test_framework.messages import (
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    SEQUENCE_FINAL,
    tx_from_hex,
)
from test_framework.script import CScript, OP_RETURN
from test_framework.script_util import PAY_TO_ANCHOR
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, find_vout_for_address


class WalletGetTransactionMixedInputsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_mixed_fields(self, entry, wallet_debit, wallet_credit, expect_fee=False):
        assert_equal(entry["involves_mixed_inputs"], True)
        assert_equal(entry["wallet_debit"], wallet_debit)
        assert_equal(entry["wallet_credit"], wallet_credit)
        if not expect_fee:
            assert "fee" not in entry

    def assert_unattributed_aggregate_send(self, entry, txid, expected_amount, wallet_debit, wallet_credit, include_txid):
        if include_txid:
            assert_equal(entry["txid"], txid)
        # The conservative fallback is a single aggregate "send" entry: the wallet
        # provably spent all of its own inputs but cannot attribute the foreign
        # outputs or its fee share, so it has no address, vout, or fee.
        assert_equal(entry["category"], "send")
        assert_equal(entry["amount"], expected_amount)
        assert "address" not in entry
        assert "vout" not in entry
        assert "fee" not in entry
        self.assert_mixed_fields(entry, wallet_debit, wallet_credit)

    def assert_receive_entry(self, entry, txid, address, vout, amount, wallet_debit, wallet_credit, include_txid):
        if include_txid:
            assert_equal(entry["txid"], txid)
        assert_equal(entry["category"], "receive")
        assert_equal(entry["address"], address)
        assert_equal(entry["vout"], vout)
        assert_equal(entry["amount"], amount)
        self.assert_mixed_fields(entry, wallet_debit, wallet_credit)

    def assert_mixed_history_entries(self, entries, txid, expected_net, address, vout, wallet_debit, wallet_credit, include_txid=True):
        """Check that a conservative mixed-input transaction is reported as a single
        unattributed aggregate send carrying the negative wallet debit plus a receive
        entry for the wallet-owned output (and nothing else), with entry amounts
        summing to the wallet net change."""
        assert_equal(len(entries), 2)
        assert_equal(sum(entry["amount"] for entry in entries), expected_net)

        send_entries = [entry for entry in entries if entry["category"] == "send"]
        assert_equal(len(send_entries), 1)
        self.assert_unattributed_aggregate_send(send_entries[0], txid, -wallet_debit, wallet_debit, wallet_credit, include_txid=include_txid)

        receive_entries = [entry for entry in entries if entry["category"] == "receive"]
        assert_equal(len(receive_entries), 1)
        self.assert_receive_entry(receive_entries[0], txid, address, vout, wallet_credit, wallet_debit, wallet_credit, include_txid=include_txid)

    def assert_wallet_view(self, wallet_name, wallet, txid, label, address, vout, wallet_debit, wallet_credit, before_blockhash):
        expected_amount = wallet_credit - wallet_debit

        tx_info = wallet.gettransaction(txid)
        assert_equal(tx_info["amount"], expected_amount)
        self.assert_mixed_fields(tx_info, wallet_debit, wallet_credit)
        self.assert_mixed_history_entries(tx_info["details"], txid, expected_amount, address, vout, wallet_debit, wallet_credit, include_txid=False)

        history = [entry for entry in wallet.listtransactions("*", 100) if entry["txid"] == txid]
        self.assert_mixed_history_entries(history, txid, expected_amount, address, vout, wallet_debit, wallet_credit)

        labeled_history = [entry for entry in wallet.listtransactions(label, 100) if entry["txid"] == txid]
        assert_equal(len(labeled_history), 1)
        self.assert_receive_entry(labeled_history[0], txid, address, vout, wallet_credit, wallet_debit, wallet_credit, include_txid=True)

        since = [entry for entry in wallet.listsinceblock(before_blockhash)["transactions"] if entry["txid"] == txid]
        self.assert_mixed_history_entries(since, txid, expected_amount, address, vout, wallet_debit, wallet_credit)

        self.log.debug("%s mixed-input wallet net amount: %s", wallet_name, expected_amount)

    def script_for_address(self, wallet, address):
        return CScript(bytes.fromhex(wallet.getaddressinfo(address)["scriptPubKey"]))

    def assert_wallet_funded_mixed_input_view(self, wallet, txid, external_address, wallet_debit, external_amount, burn_amount, fee):
        wallet_credit = Decimal("0.00000000")
        expected_amount = -(external_amount + burn_amount)

        tx_info = wallet.gettransaction(txid)
        assert_equal(tx_info["amount"], expected_amount)
        assert_equal(tx_info["fee"], -fee)
        self.assert_mixed_fields(tx_info, wallet_debit, wallet_credit, expect_fee=True)
        # Fully attributable: every send entry is a real per-output send with a
        # vout and fee, so there is no unattributed aggregate fallback entry.
        assert_equal(any(detail["category"] == "send" and ("vout" not in detail or "fee" not in detail) for detail in tx_info["details"]), False)

        send_details = [entry for entry in tx_info["details"] if entry["category"] == "send"]
        assert_equal(len(send_details), 2)
        external_details = [entry for entry in send_details if entry.get("address") == external_address]
        burn_details = [entry for entry in send_details if "address" not in entry]
        assert_equal(len(external_details), 1)
        assert_equal(len(burn_details), 1)

        for entry, amount in [(external_details[0], external_amount), (burn_details[0], burn_amount)]:
            assert_equal(entry["amount"], -amount)
            assert_equal(entry["fee"], -fee)
            self.assert_mixed_fields(entry, wallet_debit, wallet_credit, expect_fee=True)

        history = [entry for entry in wallet.listtransactions("*", 100) if entry["txid"] == txid]
        assert_equal(len(history), 2)
        assert_equal(any(entry["category"] == "send" and ("vout" not in entry or "fee" not in entry) for entry in history), False)
        assert_equal(sum(entry["amount"] for entry in history), expected_amount)

    def test_zero_value_foreign_anchor_input(self, node, wallet, external_wallet):
        self.log.info("Test zero-value foreign anchor inputs keep wallet fee/send accounting attributable")
        wallet_input = next(utxo for utxo in wallet.listunspent() if utxo["amount"] >= Decimal("1.00000000"))
        wallet_debit = wallet_input["amount"]
        wallet_debit_sat = int(wallet_debit * COIN)
        fee_sat = 1000
        burn_sat = 1
        fee = Decimal(fee_sat) / COIN
        burn_amount = Decimal(burn_sat) / COIN
        external_amount = Decimal(wallet_debit_sat - fee_sat - burn_sat) / COIN

        parent_address = wallet.getrawchangeaddress()
        parent_script = self.script_for_address(wallet, parent_address)

        parent_tx = CTransaction()
        parent_tx.version = 3
        parent_tx.vin.append(CTxIn(COutPoint(int(wallet_input["txid"], 16), wallet_input["vout"]), b"", SEQUENCE_FINAL))
        parent_tx.vout.append(CTxOut(wallet_debit_sat, parent_script))
        parent_tx.vout.append(CTxOut(0, PAY_TO_ANCHOR))
        parent_signed = wallet.signrawtransactionwithwallet(parent_tx.serialize().hex())
        assert_equal(parent_signed["complete"], True)
        parent_hex = parent_signed["hex"]
        parent_tx = tx_from_hex(parent_hex)
        parent_txid = parent_tx.txid_hex

        external_address = external_wallet.getnewaddress()
        external_script = self.script_for_address(external_wallet, external_address)
        child_tx = CTransaction()
        child_tx.version = 3
        child_tx.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 0), b"", SEQUENCE_FINAL))
        child_tx.vin.append(CTxIn(COutPoint(int(parent_txid, 16), 1), b"", SEQUENCE_FINAL))
        child_tx.vout.append(CTxOut(wallet_debit_sat - fee_sat - burn_sat, external_script))
        child_tx.vout.append(CTxOut(burn_sat, CScript([OP_RETURN])))
        child_signed = wallet.signrawtransactionwithwallet(child_tx.serialize().hex(), [
            {"txid": parent_txid, "vout": 0, "scriptPubKey": parent_script.hex(), "amount": wallet_debit},
            {"txid": parent_txid, "vout": 1, "scriptPubKey": PAY_TO_ANCHOR.hex(), "amount": Decimal("0.00000000")},
        ])
        assert_equal(child_signed["complete"], True)
        child_hex = child_signed["hex"]
        child_tx = tx_from_hex(child_hex)

        submit_res = node.submitpackage([parent_hex, child_hex], maxburnamount=burn_amount)
        assert_equal(submit_res["package_msg"], "success")
        node.syncwithvalidationinterfacequeue()

        self.assert_wallet_funded_mixed_input_view(wallet, child_tx.txid_hex, external_address, wallet_debit, external_amount, burn_amount, fee)
        self.generatetoaddress(node, 1, external_wallet.getnewaddress())
        self.assert_wallet_funded_mixed_input_view(wallet, child_tx.txid_hex, external_address, wallet_debit, external_amount, burn_amount, fee)

    def test_basic_mixed_input_transaction(self, node, funder, alice, bob):
        self.log.info("Test basic mixed-input transaction accounting")
        funder.sendtoaddress(alice.getnewaddress(), Decimal("1.0"))
        funder.sendtoaddress(bob.getnewaddress(), Decimal("1.0"))
        self.generatetoaddress(node, 1, funder.getnewaddress())
        before_mixed_input_blockhash = node.getbestblockhash()

        alice_input = alice.listunspent()[0]
        bob_input = bob.listunspent()[0]
        alice_credit = Decimal("1.50000000")
        bob_credit = Decimal("0.49998000")
        alice_label = "alice_mixed"
        bob_label = "bob_mixed"
        alice_output = alice.getnewaddress(alice_label)
        bob_output = bob.getnewaddress(bob_label)

        raw_tx = node.createrawtransaction(
            inputs=[
                {"txid": alice_input["txid"], "vout": alice_input["vout"]},
                {"txid": bob_input["txid"], "vout": bob_input["vout"]},
            ],
            outputs={
                alice_output: alice_credit,
                bob_output: bob_credit,
            },
        )
        raw_tx = alice.signrawtransactionwithwallet(raw_tx)["hex"]
        raw_tx = bob.signrawtransactionwithwallet(raw_tx)["hex"]

        txid = node.sendrawtransaction(raw_tx)
        node.syncwithvalidationinterfacequeue()
        assert_equal(node.getmempoolentry(txid)["fees"]["base"], Decimal("0.00002000"))

        wallet_views = [
            {
                "wallet_name": "alice",
                "wallet": alice,
                "label": alice_label,
                "address": alice_output,
                "vout": find_vout_for_address(node, txid, alice_output),
                "wallet_debit": alice_input["amount"],
                "wallet_credit": alice_credit,
            },
            {
                "wallet_name": "bob",
                "wallet": bob,
                "label": bob_label,
                "address": bob_output,
                "vout": find_vout_for_address(node, txid, bob_output),
                "wallet_debit": bob_input["amount"],
                "wallet_credit": bob_credit,
            },
        ]

        for view in wallet_views:
            self.assert_wallet_view(txid=txid, before_blockhash=before_mixed_input_blockhash, **view)

        self.generatetoaddress(node, 1, funder.getnewaddress())

        for view in wallet_views:
            self.assert_wallet_view(txid=txid, before_blockhash=before_mixed_input_blockhash, **view)

        return {"txid": txid, "wallet_views": wallet_views}

    def test_reorged_mixed_input_removed_entries(self, node, funder, mixed_tx):
        self.log.info("Test reorged mixed-input transactions appear in listsinceblock removed entries")
        reorged_blockhash = node.getbestblockhash()
        node.invalidateblock(reorged_blockhash)
        self.generatetoaddress(node, 2, funder.getnewaddress())

        txid = mixed_tx["txid"]
        for view in mixed_tx["wallet_views"]:
            wallet = view["wallet"]
            address = view["address"]
            vout = view["vout"]
            wallet_debit = view["wallet_debit"]
            wallet_credit = view["wallet_credit"]
            expected_net = wallet_credit - wallet_debit
            since = wallet.listsinceblock(reorged_blockhash)
            removed = [entry for entry in since["removed"] if entry["txid"] == txid]
            self.assert_mixed_history_entries(removed, txid, expected_net, address, vout, wallet_debit, wallet_credit)
            reincluded = [entry for entry in since["transactions"] if entry["txid"] == txid]
            self.assert_mixed_history_entries(reincluded, txid, expected_net, address, vout, wallet_debit, wallet_credit)

    def run_test(self):
        node = self.nodes[0]
        funder = node.get_wallet_rpc(self.default_wallet_name)

        node.createwallet("alice")
        node.createwallet("bob")
        alice = node.get_wallet_rpc("alice")
        bob = node.get_wallet_rpc("bob")

        self.generatetoaddress(node, 101, funder.getnewaddress())

        mixed_tx = self.test_basic_mixed_input_transaction(node, funder, alice, bob)

        self.test_reorged_mixed_input_removed_entries(
            node=node,
            funder=funder,
            mixed_tx=mixed_tx,
        )

        self.test_zero_value_foreign_anchor_input(node, alice, funder)
        self.test_wallet_change_output(node, funder, alice, bob)

    def test_wallet_change_output(self, node, funder, alice, bob):
        self.log.info("Test a wallet-owned change output in a conservative mixed-input transaction is reported as a receive entry")
        # Fund alice and bob with positive-value inputs. The foreign (bob) input
        # has positive value, so the transaction is always reported conservatively
        # (the absent fee on the entries below confirms the conservative view).
        alice_funding_txid = funder.sendtoaddress(alice.getnewaddress(), Decimal("1.0"))
        bob_funding_txid = funder.sendtoaddress(bob.getnewaddress(), Decimal("1.0"))
        self.generatetoaddress(node, 1, funder.getnewaddress())
        before_blockhash = node.getbestblockhash()

        alice_input = next(u for u in alice.listunspent() if u["txid"] == alice_funding_txid)
        bob_input = next(u for u in bob.listunspent() if u["txid"] == bob_funding_txid)

        fee = Decimal("0.00002000")
        # Alice keeps part of her input as change to a raw change address, so the
        # wallet classifies the output as change (not a labeled receive); the
        # remainder funds bob's output.
        alice_change_amount = Decimal("0.4")
        alice_change_address = alice.getrawchangeaddress()
        bob_credit = alice_input["amount"] + bob_input["amount"] - alice_change_amount - fee

        raw_tx = node.createrawtransaction(
            inputs=[
                {"txid": alice_input["txid"], "vout": alice_input["vout"]},
                {"txid": bob_input["txid"], "vout": bob_input["vout"]},
            ],
            outputs={
                alice_change_address: alice_change_amount,
                bob.getnewaddress(): bob_credit,
            },
        )
        raw_tx = alice.signrawtransactionwithwallet(raw_tx)["hex"]
        raw_tx = bob.signrawtransactionwithwallet(raw_tx)["hex"]
        txid = node.sendrawtransaction(raw_tx)
        node.syncwithvalidationinterfacequeue()

        # Premise: the wallet-owned output is genuinely change, not a labeled receive.
        assert_equal(alice.getaddressinfo(alice_change_address)["ischange"], True)

        alice_debit = alice_input["amount"]
        alice_credit = alice_change_amount
        expected_net = alice_credit - alice_debit
        change_vout = find_vout_for_address(node, txid, alice_change_address)

        # The conservative aggregate send view does not skip change: the change
        # output is reported as a receive entry alongside the aggregate send, so
        # the entry amounts still sum to alice's net change.
        tx_info = alice.gettransaction(txid)
        assert_equal(tx_info["amount"], expected_net)
        self.assert_mixed_fields(tx_info, alice_debit, alice_credit)
        self.assert_mixed_history_entries(tx_info["details"], txid, expected_net, alice_change_address, change_vout, alice_debit, alice_credit, include_txid=False)

        history = [entry for entry in alice.listtransactions("*", 100) if entry["txid"] == txid]
        self.assert_mixed_history_entries(history, txid, expected_net, alice_change_address, change_vout, alice_debit, alice_credit)

        since = [entry for entry in alice.listsinceblock(before_blockhash)["transactions"] if entry["txid"] == txid]
        self.assert_mixed_history_entries(since, txid, expected_net, alice_change_address, change_vout, alice_debit, alice_credit)

        # The same conservative shape holds after confirmation.
        self.generatetoaddress(node, 1, funder.getnewaddress())
        confirmed = [entry for entry in alice.listtransactions("*", 100) if entry["txid"] == txid]
        self.assert_mixed_history_entries(confirmed, txid, expected_net, alice_change_address, change_vout, alice_debit, alice_credit)


if __name__ == '__main__':
    WalletGetTransactionMixedInputsTest(__file__).main()
