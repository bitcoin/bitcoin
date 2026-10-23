#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getscripthistory RPC and txoscriptindex."""

import hashlib
from decimal import Decimal

from test_framework.messages import CTxOut
from test_framework.script import CScript, OP_RETURN
from test_framework.script_util import key_to_p2pkh_script, key_to_p2wpkh_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet, getnewdestination


def scripthash(script_hex):
    """SHA256(scriptPubKey) in Bitcoin / Electrum display byte order."""
    return hashlib.sha256(bytes.fromhex(script_hex)).digest()[::-1].hex()


def assert_output_status(tx, index, *, spent, spending_txid=None):
    output, = [output for output in tx["vout"] if output["n"] == index]
    assert output["spent"] is spent, output
    if spending_txid is None:
        assert "spending_txid" not in output
    else:
        assert_equal(output["spending_txid"], spending_txid)


def txs_by_id(history):
    return {tx["txid"]: tx for tx in history["transactions"]}


def spending_by_id(history):
    return {tx["txid"]: tx for tx in history.get("spending_transactions", [])}


class GetScriptHistoryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-txoscriptindex", "-txospenderindex", "-txindex"]]

    def wait_for_indexes(self, node):
        self.wait_until(lambda: all(i["synced"] for i in node.getindexinfo().values()))

    def check_omitted_spenders(self, node, query):
        for verbosity in (0, 1, 2):
            for include_mempool in (False, True):
                options = {"include_mempool": include_mempool}
                expected = node.getscripthistory(query, verbosity, options)
                expected.pop("spending_transactions", None)
                # Keep annotations referencing confirmed or mempool matches; only
                # extra bodies and annotations referencing those extras disappear.
                matching_txids = {tx["txid"] for tx in expected["transactions"]}
                for tx in expected["transactions"]:
                    for output in tx["vout"]:
                        if output.get("spending_txid") not in matching_txids:
                            output.pop("spending_txid", None)
                assert_equal(node.getscripthistory(query, verbosity, dict(options, include_spending_transactions=False)), expected)

    def send_without_change(self, node, wallet, utxo, script):
        tx = wallet.create_self_transfer(utxo_to_spend=utxo, fee=Decimal("0.00001"))["tx"]
        tx.vout[0].scriptPubKey = script
        wallet.sign_tx(tx)
        return wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex())

    def test_unspendable_outputs(self, node):
        self.log.info("Unspendable and genesis outputs are not spent")
        script = CScript([OP_RETURN, b"scripthistory"])
        tx = self.wallet.create_self_transfer()["tx"]
        tx.vout.append(CTxOut(0, script))
        self.wallet.sign_tx(tx)
        txid = self.wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex())
        for confirmed in (False, True):
            if confirmed:
                self.generate(self.wallet, 1)
                self.wait_for_indexes(node)
            history = node.getscripthistory(scripthash(script.hex()))
            assert_output_status(txs_by_id(history)[txid], 1, spent=False)
            assert "spending_transactions" not in history
        genesis = node.getblock(node.getblockhash(0), 2)["tx"][0]
        history = node.getscripthistory(scripthash(genesis["vout"][0]["scriptPubKey"]["hex"]), 0, {"end_height": 0})
        assert_output_status(txs_by_id(history)[genesis["txid"]], 0, spent=False)

    def test_confirmed_transaction_order(self, node):
        self.log.info("Confirmed matches and separate spenders follow block transaction order")
        query_wallet = MiniWallet(node, tag_name="scripthistory-order")
        funding_txids = [
            self.send_without_change(node, self.wallet, self.wallet.get_utxo(confirmed_only=True), query_wallet.get_output_script())
            for _ in range(3)
        ]
        # Reverse txid order distinguishes block order from txid order.
        expected_matches = sorted(funding_txids, reverse=True)
        self.generateblock(node, self.wallet.get_address(), expected_matches)
        self.wait_for_indexes(node)
        history = node.getscripthistory(query_wallet.get_address())
        assert_equal([tx["txid"] for tx in history["transactions"]], expected_matches)
        assert "spending_transactions" not in history

        query_wallet.rescan_utxos()
        _, destination_script, _ = getnewdestination()
        spending_txids = [
            self.send_without_change(node, query_wallet, query_wallet.get_utxo(txid=txid), destination_script)
            for txid in funding_txids
        ]
        expected_spenders = sorted(spending_txids, reverse=True)
        self.generateblock(node, self.wallet.get_address(), expected_spenders)
        self.wait_for_indexes(node)
        history = node.getscripthistory(query_wallet.get_address())
        assert_equal([tx["txid"] for tx in history["transactions"]], expected_matches)
        assert_equal([tx["txid"] for tx in history["spending_transactions"]], expected_spenders)
        self.wallet.rescan_utxos()

    def test_shared_spenders(self, node):
        self.log.info("Shared spenders resolve outputs across funding blocks")
        query_wallet = MiniWallet(node, tag_name="scripthistory-shared-spenders")
        funding_txids = []
        for _ in range(2):
            tx = self.wallet.create_self_transfer(confirmed_only=True)["tx"]
            tx.vout[0].nValue -= 4 * 100_000
            tx.vout.extend(CTxOut(100_000, query_wallet.get_output_script()) for _ in range(4))
            self.wallet.sign_tx(tx)
            funding_txids.append(tx.txid_hex)
            self.generateblock(node, self.wallet.get_address(), [tx.serialize().hex()])
            self.wallet.rescan_utxos()

        query_wallet.rescan_utxos()
        utxos = sorted(query_wallet.get_utxos(), key=lambda utxo: (utxo["txid"], utxo["vout"]))
        assert_equal(len(utxos), 8)
        # Interleave inputs from both funding blocks and leave one output unspent.
        expected_spenders = {}
        for inputs in (utxos[:-1:2], utxos[1:-1:2]):
            tx = query_wallet.create_self_transfer_multi(utxos_to_spend=inputs)["tx"]
            tx.vout[0].scriptPubKey = self.wallet.get_output_script()
            query_wallet.sign_tx(tx)
            for utxo in inputs:
                expected_spenders[utxo["txid"], utxo["vout"]] = tx.txid_hex
            self.generateblock(node, self.wallet.get_address(), [tx.serialize().hex()])

        self.wait_for_indexes(node)
        history = node.getscripthistory(query_wallet.get_address(), 0, {"include_mempool": False})
        assert_equal([tx["txid"] for tx in history["transactions"]], funding_txids)
        assert_equal(sorted(tx["txid"] for tx in history["spending_transactions"]), sorted(set(expected_spenders.values())))
        for tx in history["transactions"]:
            assert_equal([output["n"] for output in tx["vout"]], [1, 2, 3, 4])
            for output in tx["vout"]:
                spender = expected_spenders.get((tx["txid"], output["n"]))
                assert_output_status(tx, output["n"], spent=spender is not None, spending_txid=spender)
        self.wallet.rescan_utxos()

    def test_serialized_size_limit(self, node):
        self.log.info("A shared 50 MiB budget covers index reads and additional transaction bodies")
        self.restart_node(0, extra_args=self.extra_args[0])
        self.generate(self.wallet, 154)
        query_wallet = MiniWallet(node, tag_name="scripthistory-size")
        address = query_wallet.get_address()
        (_, pubkey), _, _ = getnewdestination(address_type="bech32")
        scripts = [key_to_p2pkh_script(pubkey), key_to_p2wpkh_script(pubkey)]
        descriptor = f"combo({pubkey.hex()})"
        start_height = node.getblockcount() + 1
        txids = []
        sizes = []
        stripped_sizes = []
        for i in range(54):
            tx = self.wallet.create_self_transfer(confirmed_only=True, target_vsize=916_000)["tx"]
            tx.vout[0].scriptPubKey = query_wallet.get_output_script()
            # Ten overlaps make repeated index reads exceed the limit
            # even when the unique transaction bodies fit.
            for script in scripts if i < 10 else [scripts[i // 27]]:
                tx.vout.append(CTxOut(0, script))
            self.wallet.sign_tx(tx)
            # A consensus-valid Taproot annex makes witness bytes decisive:
            # all 54 stripped transactions fit, but their full serialization does not.
            tx.wit.vtxinwit[0].scriptWitness.stack.append(b"\x50" + bytes(63_999))
            raw = tx.serialize().hex()
            sizes.append(len(raw) // 2)
            stripped_sizes.append(len(tx.serialize_without_witness()))
            txids.append(tx.txid_hex)
            self.generateblock(node, self.wallet.get_address(), [raw])
        self.wait_for_indexes(node)
        confirmed_only = {"start_height": start_height, "end_height": start_height + 52, "include_mempool": False}
        with_mempool = dict(confirmed_only, include_mempool=True)
        without_spenders = dict(with_mempool, include_spending_transactions=False)
        limit = 50 * 1024 * 1024
        assert sum(sizes[:53]) < limit < sum(sizes)
        assert sum(stripped_sizes) < limit
        error = f"Script history exceeds the {limit} byte budget. "
        lookup_error = "Script history index lookup limit exceeded while scanning block "
        mempool_error = error + "Set include_mempool=false."
        spender_error = error + "Set include_spending_transactions=false."
        # Every script fits separately. Together, repeated reads of overlapping
        # transactions exceed the work budget even though unique bodies fit.
        assert sum(sizes[:53]) + sum(sizes[:10]) > limit
        for script in scripts:
            assert node.getscripthistory(scripthash(script.hex()), 0)["transactions"]
        assert_raises_rpc_error(-1, lookup_error, node.getscripthistory, descriptor, 0, confirmed_only)
        narrowed = dict(confirmed_only, end_height=start_height + 39)
        assert_equal(len(node.getscripthistory(descriptor, 0, narrowed)["transactions"]), 40)
        assert_raises_rpc_error(-1, f"{lookup_error}{start_height + 53}.", node.getscripthistory, address, 0,
                                {"include_spending_transactions": False})

        # Repeated index reads leave too little room for mempool matches, even
        # though all unique returned transaction bodies fit within the budget.
        combined_range = dict(confirmed_only, end_height=start_height + 42, include_mempool=True)
        assert_equal(len(node.getscripthistory(descriptor, 0, combined_range)["transactions"]), 43)
        mempool_txids = []
        mempool_bytes = 0
        for _ in range(5):
            tx = self.wallet.create_self_transfer(confirmed_only=True, target_vsize=99_000)["tx"]
            tx.vout[0].nValue -= 1000
            tx.vout.append(CTxOut(1000, scripts[0]))
            self.wallet.sign_tx(tx)
            raw = tx.serialize().hex()
            mempool_bytes += len(raw) // 2
            mempool_txids.append(self.wallet.sendrawtransaction(from_node=node, tx_hex=raw))
        assert sum(sizes[:43]) + mempool_bytes < limit < sum(sizes[:43]) + sum(sizes[:10]) + mempool_bytes
        assert_raises_rpc_error(-1, mempool_error, node.getscripthistory, descriptor, 0, combined_range)
        self.generateblock(node, self.wallet.get_address(), mempool_txids)
        self.wait_for_indexes(node)

        query_wallet.rescan_utxos()
        utxos = {utxo["txid"]: utxo for utxo in query_wallet.get_utxos()}
        # Four 100 kB transactions fit, but five do not. Each spends two
        # matching outputs and is itself a match: none may be counted twice.
        for i in range(4):
            query_wallet.send_self_transfer_multi(
                from_node=node,
                utxos_to_spend=[utxos[txid] for txid in txids[2 * i:2 * i + 2]],
                target_vsize=100_000, fee_per_output=100_000)
        assert sum(sizes[:53]) + 400_000 < limit < sum(sizes[:53]) + 500_000
        mempool_history = node.getscripthistory(address, 0, with_mempool)
        assert_equal(len(mempool_history["transactions"]), 57)
        assert "spending_transactions" not in mempool_history
        first_four = node.getrawmempool()
        query_wallet.send_self_transfer(from_node=node, utxo_to_spend=utxos[txids[8]], target_vsize=100_000)
        assert_raises_rpc_error(-1, mempool_error, node.getscripthistory, address, 0, without_spenders)
        self.generateblock(node, self.wallet.get_address(), first_four)
        self.wait_for_indexes(node)
        # Outside the height range they are extra spenders, still counted once
        # despite spending two matching outputs apiece.
        history = node.getscripthistory(address, 0, confirmed_only)
        assert_equal(len(history["transactions"]), 53)
        assert_equal(len(history["spending_transactions"]), 4)
        assert_raises_rpc_error(-1, spender_error, node.getscripthistory, address, 0, with_mempool)
        # Omitting the four confirmed extras leaves room for the fifth mempool match.
        omitted = node.getscripthistory(address, 0, without_spenders)
        assert_equal(len(omitted["transactions"]), 54)
        assert "spending_transactions" not in omitted
        # Replace the fifth match with a higher-fee payment elsewhere. Now
        # it is only an extra mempool spender and must still consume the budget.
        replacement = query_wallet.create_self_transfer(
            utxo_to_spend=utxos[txids[8]],
            target_vsize=100_000, fee=Decimal("0.4"))["tx"]
        replacement.vout[0].scriptPubKey = getnewdestination()[1]
        query_wallet.sign_tx(replacement)
        query_wallet.sendrawtransaction(from_node=node, tx_hex=replacement.serialize().hex())
        assert_raises_rpc_error(-1, spender_error, node.getscripthistory, address, 0, with_mempool)
        assert_equal(len(node.getscripthistory(address, 0, confirmed_only)["spending_transactions"]), 4)
        for confirmed in (False, True):
            if confirmed:
                self.generate(self.wallet, 1)
                self.wait_for_indexes(node)
                assert_raises_rpc_error(-1, spender_error, node.getscripthistory, address, 0, confirmed_only)
            omitted = node.getscripthistory(address, 0, without_spenders)
            assert_equal([tx["txid"] for tx in omitted["transactions"]], txids[:53])
            assert "spending_transactions" not in omitted
            for tx in omitted["transactions"]:
                assert all("spending_txid" not in output for output in tx["vout"])
            by_id = txs_by_id(omitted)
            for txid in txids[:9]:
                assert_output_status(by_id[txid], 0, spent=True)
            assert_output_status(by_id[txids[9]], 0, spent=False)

    def run_test(self):
        node0 = self.nodes[0]
        self.wallet = MiniWallet(node0)

        self.log.info("Index is required")
        self.restart_node(0, extra_args=[])
        assert_raises_rpc_error(-1, "txoscriptindex is not enabled", node0.getscripthistory, "00" * 32)
        self.restart_node(0, extra_args=self.extra_args[0])
        self.wallet.rescan_utxos()
        self.wait_for_indexes(node0)

        self.log.info("Height range validation")
        dummy = "00" * 32
        assert_raises_rpc_error(-8, "start_height cannot be negative", node0.getscripthistory, dummy, 0, {"start_height": -1})
        assert_raises_rpc_error(-8, "end_height must be greater than or equal to start_height", node0.getscripthistory, dummy, 0, {"start_height": 5, "end_height": 4})
        self.log.info("Index coinbase and later payments to MiniWallet script")
        utxo = self.wallet.get_utxo()
        sh = scripthash(self.wallet.get_output_script().hex())

        history = node0.getscripthistory(sh)
        assert "spending_transactions" not in history
        assert "pruned_blocks" not in history
        assert_output_status(txs_by_id(history)[utxo["txid"]], utxo["vout"], spent=False)

        self.log.info("Address and descriptor queries match scripthash")
        addr = self.wallet.get_address()
        assert_equal(node0.getscripthistory(addr), history)
        assert_equal(node0.getscripthistory(self.wallet.get_descriptor()), history)
        assert_equal(node0.getscripthistory(f"addr({addr})"), history)
        for query in (addr, sh, self.wallet.get_descriptor(), f"addr({addr})"):
            for explicit_range in (0, 10, [0, 0]):
                assert_raises_rpc_error(-8, "range is only valid for ranged descriptors", node0.getscripthistory, query, 0, {"range": explicit_range})

        self.log.info("Payment with two outputs to the same script is one history entry")
        pay = self.wallet.send_self_transfer_multi(from_node=node0, utxos_to_spend=[utxo], num_outputs=2)
        blockhash = self.generate(self.wallet, 1)[0]
        self.wait_for_indexes(node0)

        for verbosity in (0, 1):
            history = node0.getscripthistory(sh, verbosity)
            pay_entry, = [tx for tx in history["transactions"] if tx["txid"] == pay["txid"]]
            assert_equal(pay_entry["blockhash"], blockhash)
            if verbosity == 0:
                assert_equal(pay_entry["hex"], pay["hex"])
            assert_equal([out["n"] for out in pay_entry["vout"]], [0, 1])
            for index in (0, 1):
                assert_output_status(pay_entry, index, spent=False)

        self.log.info("Height range skips earlier transactions")
        pay_height = node0.getblockheader(pay_entry["blockhash"])["height"]
        ranged = node0.getscripthistory(sh, 0, {"start_height": pay_height, "end_height": pay_height})
        ranged_txids = txs_by_id(ranged)
        assert pay["txid"] in ranged_txids
        assert utxo["txid"] not in ranged_txids
        before = node0.getscripthistory(sh, 0, {"start_height": 0, "end_height": pay_height - 1})
        before_by_id = txs_by_id(before)
        assert pay["txid"] not in before_by_id
        # pay spends the earlier matching output, so it is listed as a spender
        # even though its own matching outputs are outside the height range.
        assert pay["txid"] in spending_by_id(before)
        assert_output_status(before_by_id[utxo["txid"]], utxo["vout"], spent=True, spending_txid=pay["txid"])

        spend = self.wallet.send_self_transfer(from_node=node0, utxo_to_spend=pay["new_utxos"][0])

        self.log.info("Unconfirmed matching transactions are included from the mempool")
        history = node0.getscripthistory(sh)
        by_id = txs_by_id(history)
        assert_output_status(by_id[pay["txid"]], 0, spent=True, spending_txid=spend["txid"])
        assert spend["txid"] not in spending_by_id(history)
        mempool_entry = by_id[spend["txid"]]
        assert "blockhash" not in mempool_entry
        assert "height" not in mempool_entry
        # Mempool matching has separate scriptPubKey and scripthash paths.
        assert_equal(node0.getscripthistory(addr), history)
        assert_equal(node0.getscripthistory(self.wallet.get_descriptor()), history)

        no_mempool = node0.getscripthistory(sh, 0, {"include_mempool": False})
        assert spend["txid"] not in txs_by_id(no_mempool)
        assert spend["txid"] not in spending_by_id(no_mempool)
        assert_output_status(txs_by_id(no_mempool)[pay["txid"]], 0, spent=False)

        self.log.info("Chained mempool transaction is included without a blockhash")
        child = self.wallet.send_self_transfer(from_node=node0, utxo_to_spend=spend["new_utxo"])
        history = node0.getscripthistory(sh)
        by_id = txs_by_id(history)
        assert "blockhash" not in by_id[child["txid"]]
        assert_output_status(by_id[spend["txid"]], 0, spent=True, spending_txid=child["txid"])
        assert child["txid"] not in spending_by_id(history)
        self.check_omitted_spenders(node0, sh)
        assert_equal(node0.getscripthistory(addr, 0, {"include_spending_transactions": False}),
                     node0.getscripthistory(sh, 0, {"include_spending_transactions": False}))

        spend_block = self.generate(self.wallet, 1)[0]
        self.wait_for_indexes(node0)

        self.log.info("All verbosity levels retain spentness and spender annotations")
        for verbosity in (0, True, 2):
            history = node0.getscripthistory(sh, verbosity)
            by_id = txs_by_id(history)
            pay_entry = by_id[pay["txid"]]
            assert_output_status(pay_entry, 0, spent=True, spending_txid=spend["txid"])
            assert_output_status(pay_entry, 1, spent=False)
            assert spend["txid"] in by_id
            assert spend["txid"] not in spending_by_id(history)

        self.log.info("Reorg removes disconnected transactions from confirmed history")
        node0.invalidateblock(spend_block)
        self.wait_for_indexes(node0)
        history = node0.getscripthistory(sh)
        by_id = txs_by_id(history)
        assert_output_status(by_id[pay["txid"]], 0, spent=True, spending_txid=spend["txid"])
        for txid in (spend["txid"], child["txid"]):
            assert "blockhash" not in by_id[txid]

        confirmed_only = node0.getscripthistory(sh, 0, {"include_mempool": False})
        assert_output_status(txs_by_id(confirmed_only)[pay["txid"]], 0, spent=False)
        for txid in (spend["txid"], child["txid"]):
            assert txid not in txs_by_id(confirmed_only)
            assert txid not in spending_by_id(confirmed_only)

        node0.reconsiderblock(spend_block)
        self.wait_for_indexes(node0)
        history = node0.getscripthistory(sh)
        assert_output_status(txs_by_id(history)[pay["txid"]], 0, spent=True, spending_txid=spend["txid"])

        self.log.info("Spender that does not create matching outputs goes in spending_transactions")
        _, foreign_spk, _ = getnewdestination()
        foreign_txid = self.send_without_change(node0, self.wallet, pay["new_utxos"][1], foreign_spk)
        self.generate(self.wallet, 1)
        self.wait_for_indexes(node0)
        history = node0.getscripthistory(sh)
        by_id = txs_by_id(history)
        spenders = spending_by_id(history)
        assert_output_status(by_id[pay["txid"]], 1, spent=True, spending_txid=foreign_txid)
        assert foreign_txid not in by_id
        assert_equal(spenders[foreign_txid]["hex"], node0.getrawtransaction(foreign_txid))
        assert "vout" not in spenders[foreign_txid]
        assert_equal(node0.getscripthistory(sh, 0, {"include_spending_transactions": True}), history)
        if self.is_cli_compiled():
            assert_equal(node0.cli.getscripthistory(sh, include_spending_transactions=False),
                         node0.getscripthistory(sh, 0, {"include_spending_transactions": False}))
        self.check_omitted_spenders(node0, sh)

        self.test_confirmed_transaction_order(node0)
        self.test_unspendable_outputs(node0)

        self.log.info("Confirmed spenders need txospenderindex unless they are themselves scriptPubKey hits")
        self.restart_node(0, extra_args=["-txoscriptindex", "-txindex"])
        self.wait_for_indexes(node0)
        history = node0.getscripthistory(sh, 1)
        by_id = txs_by_id(history)
        pay_entry = by_id[pay["txid"]]
        assert_output_status(pay_entry, 0, spent=True, spending_txid=spend["txid"])
        assert_output_status(pay_entry, 1, spent=True)
        assert foreign_txid not in by_id
        assert "spending_transactions" not in history
        self.check_omitted_spenders(node0, sh)

        self.log.info("Mempool spenders are included without txospenderindex")
        self.wallet.rescan_utxos()
        new_pay = self.wallet.send_self_transfer(from_node=node0)
        self.generate(self.wallet, 1)
        self.wait_for_indexes(node0)
        _, mempool_foreign_spk, _ = getnewdestination()
        mempool_foreign_txid = self.send_without_change(node0, self.wallet, new_pay["new_utxo"], mempool_foreign_spk)
        history = node0.getscripthistory(sh)
        assert_output_status(txs_by_id(history)[new_pay["txid"]], 0, spent=True, spending_txid=mempool_foreign_txid)
        assert mempool_foreign_txid not in txs_by_id(history)
        mempool_spenders = spending_by_id(history)
        assert "blockhash" not in mempool_spenders[mempool_foreign_txid]
        assert_equal(mempool_spenders[mempool_foreign_txid]["hex"], node0.getrawtransaction(mempool_foreign_txid))
        self.check_omitted_spenders(node0, sh)
        self.test_serialized_size_limit(node0)
        self.test_shared_spenders(node0)

        self.log.info("Pruned script history works with only txoscriptindex enabled")
        self.restart_node(0, extra_args=["-txoscriptindex", "-txospenderindex=0", "-txindex=0", "-prune=1", "-fastprune"])
        self.wait_for_indexes(node0)
        assert_equal(set(node0.getindexinfo()), {"txoscriptindex"})
        old_range = {"start_height": pay_height, "end_height": pay_height, "include_mempool": False}
        assert pay["txid"] in txs_by_id(node0.getscripthistory(sh, 0, old_range))

        # Small fastprune block files avoid mining large blocks. Mine to the
        # regtest prune threshold, using an unrelated script to keep history small.
        _, _, unrelated_address = getnewdestination()
        self.generatetoaddress(node0, 1001 - node0.getblockcount(), unrelated_address)
        recent_block = self.generate(self.wallet, 1)[0]
        recent_txid = node0.getblock(recent_block)["tx"][0]
        self.wait_for_indexes(node0)
        assert node0.pruneblockchain(node0.getblockcount()) >= pay_height
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node0.getblock, blockhash)

        for query, verbosity in ((sh, 0), (addr, 0), (self.wallet.get_descriptor(), 0), (sh, 1), (sh, 2)):
            pruned = node0.getscripthistory(query, verbosity, old_range)
            assert_equal(pruned["transactions"], [])
            assert_equal(pruned["pruned_blocks"], [blockhash])
            assert "spending_transactions" not in pruned
        for verbosity in (0, 1, 2):
            recent = node0.getscripthistory(sh, verbosity, {
                "start_height": node0.getblockcount(), "include_mempool": False,
            })
            assert_equal([tx["txid"] for tx in recent["transactions"]], [recent_txid])
            assert "pruned_blocks" not in recent


if __name__ == "__main__":
    GetScriptHistoryTest(__file__).main()
