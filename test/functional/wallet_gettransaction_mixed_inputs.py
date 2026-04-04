#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.
"""Test wallet history RPC accounting for mixed-input wallet transactions.

This test constructs a single transaction with one input from wallet "alice"
and one input from wallet "bob". Each wallet receives one output back to a
fresh address it controls.

For a collaborative transaction with mixed inputs, the wallet can objectively
report the net amount that left the wallet and the wallet-owned outputs it
received. It cannot objectively attribute the full transaction fee or invent
"send" entries for outputs funded by other participants.
"""

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, find_vout_for_address


class WalletGetTransactionMixedInputsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    @staticmethod
    def normalize_entries(entries):
        normalized = []
        for entry in entries:
            item = {
                "address": entry["address"],
                "amount": entry["amount"],
                "category": entry["category"],
                "vout": entry["vout"],
            }
            if "txid" in entry:
                item["txid"] = entry["txid"]
            if "fee" in entry:
                item["fee"] = entry["fee"]
            normalized.append(item)
        return sorted(
            normalized,
            key=lambda item: (item.get("txid", ""), item["vout"], item["category"], item["amount"]),
        )

    def collect_wallet_view_errors(self, wallet_name, wallet, txid, address, vout, input_amount, output_amount, before_blockhash):
        errors = []
        expected_amount = output_amount - input_amount
        expected_detail = [{
            "address": address,
            "amount": output_amount,
            "category": "receive",
            "vout": vout,
        }]
        expected_history = [{
            "address": address,
            "amount": output_amount,
            "category": "receive",
            "txid": txid,
            "vout": vout,
        }]

        tx_info = wallet.gettransaction(txid)
        if tx_info["amount"] != expected_amount:
            errors.append(
                f'{wallet_name} gettransaction amount: expected {expected_amount}, got {tx_info["amount"]}'
            )
        if "fee" in tx_info:
            errors.append(
                f'{wallet_name} gettransaction fee: expected field to be absent, got {tx_info["fee"]}'
            )

        actual_details = self.normalize_entries(tx_info["details"])
        if actual_details != expected_detail:
            errors.append(
                f"{wallet_name} gettransaction details: expected {expected_detail}, got {actual_details}"
            )

        actual_history = self.normalize_entries(
            [entry for entry in wallet.listtransactions("*", 100) if entry["txid"] == txid]
        )
        if actual_history != expected_history:
            errors.append(
                f"{wallet_name} listtransactions: expected {expected_history}, got {actual_history}"
            )

        actual_since = self.normalize_entries(
            [
                entry
                for entry in wallet.listsinceblock(before_blockhash)["transactions"]
                if entry["txid"] == txid
            ]
        )
        if actual_since != expected_history:
            errors.append(
                f"{wallet_name} listsinceblock: expected {expected_history}, got {actual_since}"
            )

        return errors

    def run_test(self):
        node = self.nodes[0]
        funder = node.get_wallet_rpc(self.default_wallet_name)

        node.createwallet("alice")
        node.createwallet("bob")
        alice = node.get_wallet_rpc("alice")
        bob = node.get_wallet_rpc("bob")

        node.generatetoaddress(101, funder.getnewaddress(), called_by_framework=True)

        funder.sendtoaddress(alice.getnewaddress(), Decimal("1.0"))
        funder.sendtoaddress(bob.getnewaddress(), Decimal("1.0"))
        node.generatetoaddress(1, funder.getnewaddress(), called_by_framework=True)
        before_mixed_input_blockhash = node.getbestblockhash()

        alice_input = alice.listunspent()[0]
        bob_input = bob.listunspent()[0]
        mixed_output_amount = Decimal("0.99999")
        alice_output = alice.getnewaddress()
        bob_output = bob.getnewaddress()

        raw_tx = node.createrawtransaction(
            inputs=[
                {"txid": alice_input["txid"], "vout": alice_input["vout"]},
                {"txid": bob_input["txid"], "vout": bob_input["vout"]},
            ],
            outputs={
                alice_output: mixed_output_amount,
                bob_output: mixed_output_amount,
            },
        )
        raw_tx = alice.signrawtransactionwithwallet(raw_tx)["hex"]
        raw_tx = bob.signrawtransactionwithwallet(raw_tx)["hex"]

        txid = node.sendrawtransaction(raw_tx)
        node.syncwithvalidationinterfacequeue()

        assert_equal(node.getmempoolentry(txid)["fees"]["base"], Decimal("0.00002000"))

        errors = []
        errors.extend(
            self.collect_wallet_view_errors(
                wallet_name="alice",
                wallet=alice,
                txid=txid,
                address=alice_output,
                vout=find_vout_for_address(node, txid, alice_output),
                input_amount=alice_input["amount"],
                output_amount=mixed_output_amount,
                before_blockhash=before_mixed_input_blockhash,
            )
        )
        errors.extend(
            self.collect_wallet_view_errors(
                wallet_name="bob",
                wallet=bob,
                txid=txid,
                address=bob_output,
                vout=find_vout_for_address(node, txid, bob_output),
                input_amount=bob_input["amount"],
                output_amount=mixed_output_amount,
                before_blockhash=before_mixed_input_blockhash,
            )
        )

        if errors:
            raise AssertionError("Mixed-input wallet history mismatches:\n- " + "\n- ".join(errors))


if __name__ == '__main__':
    WalletGetTransactionMixedInputsTest(__file__).main()
