#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getscripthistory RPC and txoscriptindex."""

import hashlib
from decimal import Decimal

from test_framework.messages import COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.script import CScript, OP_DROP, OP_RETURN, OP_TRUE
from test_framework.script_util import key_to_p2pkh_script, key_to_p2wpkh_script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet, getnewdestination


def scripthash(script_hex):
    """SHA256(scriptPubKey), hex-encoded with reversed byte order."""
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


class GetScriptHistoryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-txoscriptindex", "-txospenderindex", "-txindex"]]

    def wait_for_indexes(self, node):
        self.wait_until(lambda: all(i["synced"] for i in node.getindexinfo().values()))

    def send_without_change(self, node, wallet, utxo, script):
        tx = wallet.create_self_transfer(utxo_to_spend=utxo, fee=Decimal("0.00001"))["tx"]
        tx.vout[0].scriptPubKey = script
        raw = tx.serialize().hex()
        return {"txid": wallet.sendrawtransaction(from_node=node, tx_hex=raw), "hex": raw}

    def test_asof(self, node):
        self.log.info("asof pins history, spentness, and confirmations to one block")
        query_wallet = MiniWallet(node, tag_name="scripthistory-asof")
        address = query_wallet.get_address()
        tx = self.wallet.create_self_transfer()["tx"]
        amount = tx.vout[0].nValue // 3
        tx.vout = [CTxOut(amount, query_wallet.get_output_script()) for _ in range(3)]
        funding_txid = self.wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex())
        funding_block = self.generate(self.wallet, 1)[0]
        funding_height = node.getblockcount()
        self.wait_for_indexes(node)
        query_wallet.rescan_utxos()
        utxos = sorted(query_wallet.get_utxos(), key=lambda utxo: utxo["vout"])

        before = [node.getscripthistory(address, verbosity, {"asof": funding_block}) for verbosity in (0, 1, 2)]
        assert_equal(node.getscripthistory(address, 0, {"asof": funding_height}), before[0])
        for history in before:
            assert_equal(history["asof"], funding_block)
            funding, = history["transactions"]
            assert_equal(funding["txid"], funding_txid)
            for index in range(3):
                assert_output_status(funding, index, spent=False)
            assert "spending_transactions" not in history

        matching_spend = query_wallet.send_self_transfer(from_node=node, utxo_to_spend=utxos[0])
        matching_block = self.generate(self.wallet, 1)[0]
        self.wait_for_indexes(node)
        at_matching = [node.getscripthistory(address, verbosity, {"asof": matching_block}) for verbosity in (0, 1, 2)]
        for verbosity, history in enumerate(at_matching):
            by_id = txs_by_id(history)
            assert_equal([tx["txid"] for tx in history["transactions"]], [funding_txid, matching_spend["txid"]])
            assert_output_status(by_id[funding_txid], 0, spent=True, spending_txid=matching_spend["txid"])
            assert_output_status(by_id[funding_txid], 1, spent=False)
            if verbosity:
                assert_equal(by_id[funding_txid]["confirmations"], 2)
                assert_equal(by_id[matching_spend["txid"]]["confirmations"], 1)

        foreign_txid = self.send_without_change(node, query_wallet, utxos[1], getnewdestination()[1])["txid"]
        foreign_block = self.generate(self.wallet, 1)[0]
        self.wait_for_indexes(node)
        for verbosity in (0, 1, 2):
            assert_equal(node.getscripthistory(address, verbosity, {"asof": funding_block}), before[verbosity])
            assert_equal(node.getscripthistory(address, verbosity, {"asof": matching_block}), at_matching[verbosity])
            for include_spenders in (False, True):
                history = node.getscripthistory(address, verbosity, {"asof": foreign_block, "include_spending_transactions": include_spenders})
                funding = txs_by_id(history)[funding_txid]
                assert_output_status(funding, 0, spent=True, spending_txid=matching_spend["txid"])
                assert_output_status(funding, 1, spent=True, spending_txid=foreign_txid if include_spenders else None)
                assert_output_status(funding, 2, spent=False)
                if include_spenders:
                    assert_equal([tx["txid"] for tx in history["spending_transactions"]], [foreign_txid])
                else:
                    assert "spending_transactions" not in history

        self.log.info("Page bounds select outputs while asof fixes spentness across pages")
        page_options = {"start_height": funding_height, "end_height": funding_height, "asof": foreign_block}
        first_pages = [node.getscripthistory(address, verbosity, page_options) for verbosity in (0, 1, 2)]
        for verbosity, page in enumerate(first_pages):
            assert_equal(page["asof"], foreign_block)
            funding, = page["transactions"]
            assert_equal(funding["txid"], funding_txid)
            assert_equal([tx["txid"] for tx in page["spending_transactions"]], [matching_spend["txid"], foreign_txid])
            assert_output_status(funding, 0, spent=True, spending_txid=matching_spend["txid"])
            assert_output_status(funding, 1, spent=True, spending_txid=foreign_txid)
            assert_output_status(funding, 2, spent=False)
            if verbosity:
                assert_equal(funding["confirmations"], 3)
                assert_equal([tx["confirmations"] for tx in page["spending_transactions"]], [2, 1])

            omitted = node.getscripthistory(address, verbosity, dict(page_options, include_spending_transactions=False))
            assert "spending_transactions" not in omitted
            funding = txs_by_id(omitted)[funding_txid]
            for index in range(3):
                assert_output_status(funding, index, spent=index < 2)

            next_page = node.getscripthistory(address, verbosity, {
                "start_height": funding_height + 1, "end_height": funding_height + 2, "asof": page["asof"],
            })
            assert_equal([tx["txid"] for tx in next_page["transactions"]], [matching_spend["txid"]])
            assert "spending_transactions" not in next_page
            earlier_page = node.getscripthistory(address, verbosity, dict(page_options, asof=matching_block))
            assert_equal([tx["txid"] for tx in earlier_page["spending_transactions"]], [matching_spend["txid"]])
            assert_output_status(txs_by_id(earlier_page)[funding_txid], 1, spent=False)

        self.log.info("end_height excludes mempool outputs but allows mempool spends; asof excludes both")
        mempool_spend = query_wallet.send_self_transfer(from_node=node, utxo_to_spend=utxos[2])
        mempool_match = query_wallet.send_self_transfer(from_node=node, utxo_to_spend=matching_spend["new_utxo"])
        live = node.getscripthistory(address)
        assert_equal(live["asof"], foreign_block)
        assert mempool_match["txid"] in txs_by_id(live)
        assert_equal(node.getscripthistory(address, 0, {"asof": funding_block, "include_mempool": True}), before[0])
        for include_mempool in (False, True):
            spenders = [matching_spend["txid"], foreign_txid, mempool_spend["txid"] if include_mempool else None]
            for include_spenders in (False, True):
                bounded = node.getscripthistory(address, 0, {
                    "end_height": funding_height, "include_mempool": include_mempool, "include_spending_transactions": include_spenders,
                })
                funding, = bounded["transactions"]
                assert_equal(funding["txid"], funding_txid)
                for index, spender in enumerate(spenders):
                    assert_output_status(funding, index, spent=spender is not None, spending_txid=spender if include_spenders else None)
                if include_spenders:
                    assert_equal([tx["txid"] for tx in bounded["spending_transactions"]], [spender for spender in spenders if spender])
                else:
                    assert "spending_transactions" not in bounded

        assert_equal(node.getscripthistory(address, 0, {"asof": funding_block, "end_height": funding_height}), before[0])
        assert_equal(node.getscripthistory(address, 0, {"asof": funding_block, "start_height": funding_height}), before[0])
        if self.is_cli_compiled():
            assert_equal(node.cli.getscripthistory(address, asof=funding_block), before[0])
            assert_equal(node.cli.getscripthistory(address, asof=funding_height, end_height=funding_height), before[0])
        assert_raises_rpc_error(-8, "end_height must not exceed", node.getscripthistory, address, 0,
                                {"asof": funding_block, "end_height": funding_height + 1})
        assert_raises_rpc_error(-8, "start_height must not exceed", node.getscripthistory, address, 0,
                                {"asof": funding_block, "start_height": funding_height + 1})
        assert_raises_rpc_error(-5, "Block not found", node.getscripthistory, address, 0, {"asof": "00" * 32})
        assert_raises_rpc_error(-8, "is negative", node.getscripthistory, address, 0, {"asof": -1})
        assert_raises_rpc_error(-8, "after current tip", node.getscripthistory, address, 0, {"asof": node.getblockcount() + 1})
        assert_raises_rpc_error(-3, "asof must be a block height or hash", node.getscripthistory, address, 0, {"asof": True})

        self.generate(self.wallet, 1)
        self.wait_for_indexes(node)
        for verbosity, page in enumerate(first_pages):
            assert_equal(node.getscripthistory(address, verbosity, page_options), page)

        self.log.info("Reorgs above asof preserve the view; replacing its block invalidates it")
        node.invalidateblock(foreign_block)
        self.wait_for_indexes(node)
        assert_equal(node.getscripthistory(address, 2, {"asof": matching_block}), at_matching[2])
        node.invalidateblock(matching_block)
        replacement = self.generateblock(node, self.wallet.get_address(), [])["hash"]
        self.wait_for_indexes(node)
        assert_raises_rpc_error(-8, "asof block is not on the active chain", node.getscripthistory, address, 0, {"asof": matching_block})
        assert_equal(node.getscripthistory(address, 0, {"asof": funding_block}), before[0])
        node.invalidateblock(replacement)
        node.reconsiderblock(foreign_block)
        self.generate(self.wallet, 1)
        self.wait_for_indexes(node)

    def test_unspendable_outputs(self, node):
        self.log.info("Unspendable and genesis outputs are not spent")
        script = CScript([OP_RETURN, b"scripthistory"])
        tx = self.wallet.create_self_transfer()["tx"]
        tx.vout.append(CTxOut(0, script))
        txid = self.wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex())
        for confirmed in (False, True):
            if confirmed:
                self.generate(self.wallet, 1)
                self.wait_for_indexes(node)
            history = node.getscripthistory(scripthash(script.hex()))
            assert_output_status(txs_by_id(history)[txid], 1, spent=False)
            assert "spending_transactions" not in history
            if confirmed:
                assert_equal(node.getscripthistory(scripthash(script.hex()), 0, {"asof": node.getbestblockhash()}), history)
        genesis = node.getblock(node.getblockhash(0), 2)["tx"][0]
        history = node.getscripthistory(scripthash(genesis["vout"][0]["scriptPubKey"]["hex"]), 0, {"end_height": 0})
        assert_output_status(txs_by_id(history)[genesis["txid"]], 0, spent=False)
        assert_equal(history["asof"], node.getbestblockhash())
        anchored = node.getscripthistory(scripthash(genesis["vout"][0]["scriptPubKey"]["hex"]), 0, {"asof": 0})
        assert_equal(anchored["asof"], node.getblockhash(0))
        assert_output_status(txs_by_id(anchored)[genesis["txid"]], 0, spent=False)

    def test_transaction_order_and_shared_spenders(self, node):
        self.log.info("Order matches and shared spenders across blocks, with mempool spenders last")
        query_wallet = MiniWallet(node, tag_name="scripthistory-order")
        funding_txids = []
        for _ in range(5):
            tx = self.wallet.create_self_transfer(confirmed_only=True)["tx"]
            tx.vout[0].nValue -= 2 * 100_000
            tx.vout.extend(CTxOut(100_000, query_wallet.get_output_script()) for _ in range(2))
            funding_txids.append(self.wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex()))
        # Reverse txid order distinguishes block order from txid order.
        expected_matches = sorted(funding_txids[:3], reverse=True) + sorted(funding_txids[3:], reverse=True)
        self.generateblock(node, self.wallet.get_address(), expected_matches[:3])
        self.generateblock(node, self.wallet.get_address(), expected_matches[3:])

        query_wallet.rescan_utxos()
        utxos = [query_wallet.get_utxo(txid=txid, vout=n) for txid in expected_matches for n in (1, 2)]
        # Spend multiple outputs from one transaction and across funding blocks.
        # The fifth spender stays in the mempool; utxos[5] stays unspent.
        input_groups = [
            (utxos[0], utxos[1], utxos[6]),
            (utxos[2], utxos[7]),
            (utxos[3], utxos[8]),
            (utxos[9],),
            (utxos[4],),
        ]
        destination_script = getnewdestination()[1]
        spending_txids = []
        spender_by_outpoint = {}
        for inputs in input_groups:
            tx = query_wallet.create_self_transfer_multi(utxos_to_spend=inputs)["tx"]
            tx.vout[0].scriptPubKey = destination_script
            spending_txids.append(query_wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex()))
            for utxo in inputs:
                spender_by_outpoint[utxo["txid"], utxo["vout"]] = tx.txid_hex

        expected_spenders = sorted(spending_txids[:3], reverse=True) + spending_txids[3:]
        self.generateblock(node, self.wallet.get_address(), expected_spenders[:3])
        self.generateblock(node, self.wallet.get_address(), [expected_spenders[3]])
        self.wait_for_indexes(node)
        history = node.getscripthistory(query_wallet.get_address())
        assert_equal([tx["txid"] for tx in history["transactions"]], expected_matches)
        assert_equal([tx["txid"] for tx in history["spending_transactions"]], expected_spenders)
        for tx in history["transactions"]:
            assert_equal([output["n"] for output in tx["vout"]], [1, 2])
            for output in tx["vout"]:
                spender = spender_by_outpoint.get((tx["txid"], output["n"]))
                assert_output_status(tx, output["n"], spent=spender is not None, spending_txid=spender)
        self.generateblock(node, self.wallet.get_address(), [expected_spenders[4]])
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
        for i in range(54):
            tx = self.wallet.create_self_transfer(confirmed_only=True, target_vsize=916_000)["tx"]
            tx.vout[0].scriptPubKey = query_wallet.get_output_script()
            # Ten transactions match both scripts, charging two index reads each.
            for script in scripts if 11 <= i < 21 else [scripts[i // 27]]:
                tx.vout.append(CTxOut(0, script))
            # A consensus-valid Taproot annex makes witness bytes decisive:
            # all 54 stripped transactions fit, but their full serialization does not.
            tx.wit.vtxinwit[0].scriptWitness.stack.append(b"\x50" + bytes(63_999))
            txids.append(tx.txid_hex)
            self.generateblock(node, self.wallet.get_address(), [tx.serialize().hex()])
        self.wait_for_indexes(node)
        confirmed_only = {"start_height": start_height, "end_height": start_height + 52, "include_mempool": False}
        with_mempool = {"start_height": start_height + 1}
        without_spenders = dict(confirmed_only, include_spending_transactions=False)
        error = f"Script history exceeds the {50 * 1024 * 1024} byte budget. "
        lookup_error = "Script history index lookup limit exceeded while scanning block "
        mempool_error = error + "Set include_mempool=false."
        spender_error = error + "Set include_spending_transactions=false."
        # Combining the scripts exceeds the read budget even though unique bodies fit.
        for script, expected in zip(scripts, (txids[:27], txids[11:21] + txids[27:])):
            history = node.getscripthistory(scripthash(script.hex()), 0)
            assert_equal([tx["txid"] for tx in history["transactions"]], expected)
        assert_raises_rpc_error(-1, lookup_error, node.getscripthistory, descriptor, 0, confirmed_only)
        narrowed = dict(confirmed_only, end_height=start_height + 39)
        assert_equal([tx["txid"] for tx in node.getscripthistory(descriptor, 0, narrowed)["transactions"]], txids[:40])
        assert_raises_rpc_error(-1, f"{lookup_error}{start_height + 53}.", node.getscripthistory, address, 0,
                                {"include_spending_transactions": False})
        assert_raises_rpc_error(-1, error + "Reduce the block range or retry with verbosity 0 or 1.", node.getscripthistory, address, 2, without_spenders)

        # Repeated index reads leave too little room for mempool matches, even
        # though all unique returned transaction bodies fit within the budget.
        combined_range = {"start_height": start_height + 11}
        assert_equal([tx["txid"] for tx in node.getscripthistory(descriptor, 0, combined_range)["transactions"]], txids[11:])
        mempool_txids = []
        for _ in range(5):
            tx = self.wallet.create_self_transfer(confirmed_only=True, target_vsize=99_000)["tx"]
            tx.vout[0].nValue -= 1000
            tx.vout.append(CTxOut(1000, scripts[0]))
            mempool_txids.append(self.wallet.sendrawtransaction(from_node=node, tx_hex=tx.serialize().hex()))
        assert_raises_rpc_error(-1, mempool_error, node.getscripthistory, descriptor, 0, combined_range)
        self.generateblock(node, self.wallet.get_address(), mempool_txids)
        self.wait_for_indexes(node)

        query_wallet.rescan_utxos()
        utxos = {utxo["txid"]: utxo for utxo in query_wallet.get_utxos()}
        # Four 100 kB transactions fit, but five do not. Each spends two
        # matching outputs and is itself a match: none may be counted twice.
        first_four = []
        for i in range(4):
            tx = query_wallet.send_self_transfer_multi(
                from_node=node,
                utxos_to_spend=[utxos[txid] for txid in txids[2 * i + 1:2 * i + 3]],
                target_vsize=100_000, fee_per_output=100_000)
            first_four.append(tx["txid"])
        mempool_history = node.getscripthistory(address, 0, with_mempool)
        assert_equal(sorted(tx["txid"] for tx in mempool_history["transactions"]), sorted(txids[1:] + first_four))
        assert "spending_transactions" not in mempool_history
        query_wallet.send_self_transfer(from_node=node, utxo_to_spend=utxos[txids[9]], target_vsize=100_000)
        assert_raises_rpc_error(-1, mempool_error, node.getscripthistory, address, 0, dict(with_mempool, include_spending_transactions=False))
        self.generateblock(node, self.wallet.get_address(), first_four)
        self.wait_for_indexes(node)
        # Outside the height range they are extra spenders, still counted once
        # despite spending two matching outputs apiece.
        history = node.getscripthistory(address, 0, confirmed_only)
        assert_equal([tx["txid"] for tx in history["transactions"]], txids[:53])
        assert_equal([tx["txid"] for tx in history["spending_transactions"]], first_four)
        assert_raises_rpc_error(-1, mempool_error, node.getscripthistory, address, 0, with_mempool)
        # With an end height, the fifth mempool match is still an extra spender.
        assert_raises_rpc_error(-1, spender_error, node.getscripthistory, address, 0, dict(confirmed_only, include_mempool=True))
        # Replace the fifth match with a higher-fee payment elsewhere. Now
        # it is only an extra mempool spender and must still consume the budget.
        replacement = query_wallet.create_self_transfer(
            utxo_to_spend=utxos[txids[9]],
            target_vsize=100_000, fee=Decimal("0.4"))["tx"]
        replacement.vout[0].scriptPubKey = getnewdestination()[1]
        query_wallet.sendrawtransaction(from_node=node, tx_hex=replacement.serialize().hex())
        assert_raises_rpc_error(-1, spender_error, node.getscripthistory, address, 0, with_mempool)
        for confirmed in (False, True):
            if confirmed:
                self.generate(self.wallet, 1)
                self.wait_for_indexes(node)
                assert_raises_rpc_error(-1, spender_error, node.getscripthistory, address, 0, confirmed_only)
                assert_raises_rpc_error(-1, error + "Narrow the range with start_height/end_height", node.getscripthistory, address, 0,
                                        {"start_height": start_height, "asof": node.getblockhash(start_height + 52), "include_spending_transactions": False})
            omitted = node.getscripthistory(address, 0, without_spenders)
            assert_equal([tx["txid"] for tx in omitted["transactions"]], txids[:53])
            assert "spending_transactions" not in omitted
            by_id = txs_by_id(omitted)
            for i, txid in enumerate(txids[:53]):
                assert_output_status(by_id[txid], 0, spent=1 <= i <= 8 or (i == 9 and confirmed))

    def test_undo_size_limit(self, node):
        self.log.info("A single block's undo can exceed the budget; lower verbosity permits retrieval without txindex")
        self.restart_node(0, extra_args=["-txoscriptindex"])
        self.wallet.rescan_utxos()
        _, query_script, address = getnewdestination()
        # Spending small inputs can require undo with large previous output scripts.
        large_script = CScript([bytes(500), OP_DROP] * 19 + [OP_TRUE])
        spends = []
        for i in range(56):
            funding = self.wallet.create_self_transfer(confirmed_only=True)["tx"]
            funding.vout[0].nValue -= 100 * 10_000
            funding.vout.extend(CTxOut(10_000, large_script) for _ in range(100))
            self.generateblock(node, self.wallet.get_address(), [funding.serialize().hex()])
            spend = CTransaction()
            spend.vin = [CTxIn(COutPoint(funding.txid_int, n)) for n in range(1, 101)]
            spend.vout = [CTxOut(900_000, query_script if i < 2 else self.wallet.get_output_script())]
            spends.append(spend)

        # Two matches share the same ~26 MiB undo read; charging it twice would fail.
        small_block = self.generateblock(node, self.wallet.get_address(), [tx.serialize().hex() for tx in spends[:28]])["hash"]
        height = node.getblockcount()
        options = {"start_height": height, "end_height": height, "include_spending_transactions": False}
        self.wait_for_indexes(node)
        history = node.getscripthistory(address, 2, options)
        assert_equal([tx["txid"] for tx in history["transactions"]], [tx.txid_hex for tx in spends[:2]])
        expected_inputs = history["transactions"][0]["vin"]
        assert_equal(expected_inputs[0]["prevout"]["scriptPubKey"]["hex"], large_script.hex())

        node.invalidateblock(small_block)
        self.generateblock(node, self.wallet.get_address(), [tx.serialize().hex() for tx in spends])
        self.wait_for_indexes(node)
        assert_raises_rpc_error(-1, f"Script history exceeds the {50 * 1024 * 1024} byte budget. Reduce the block range or retry with verbosity 0 or 1.",
                                node.getscripthistory, address, 2, options)
        for verbosity in (0, 1):
            history = node.getscripthistory(address, verbosity, options)
            assert_equal([tx["txid"] for tx in history["transactions"]], [tx.txid_hex for tx in spends[:2]])
        tx = history["transactions"][0]
        assert_equal(node.getrawtransaction(tx["txid"], 2, tx["blockhash"])["vin"], expected_inputs)

    def run_test(self):
        node0 = self.nodes[0]
        self.wallet = MiniWallet(node0)

        self.log.info("Index is required")
        self.restart_node(0, extra_args=[])
        assert_raises_rpc_error(-1, "txoscriptindex is not enabled", node0.getscripthistory, "00" * 32)
        self.restart_node(0, extra_args=self.extra_args[0])
        self.wallet.rescan_utxos()
        self.wait_for_indexes(node0)

        self.log.info("Verbosity and height range validation")
        dummy = "00" * 32
        for verbosity in (False, True):
            assert_raises_rpc_error(-3, "Verbosity was boolean but only integer allowed", node0.getscripthistory, dummy, verbosity)
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
        for query in (addr, self.wallet.get_descriptor(), f"addr({addr})"):
            assert_equal(node0.getscripthistory(query), history)
        for query in (addr, sh, self.wallet.get_descriptor()):
            assert_raises_rpc_error(-8, "range is only valid for ranged descriptors", node0.getscripthistory, query, 0, {"range": 0})

        self.log.info("Payment with two outputs to the same script is one history entry")
        pay = self.wallet.send_self_transfer_multi(from_node=node0, utxos_to_spend=[utxo], num_outputs=2)
        blockhash = self.generate(self.wallet, 1)[0]
        pay_height = node0.getblockcount()
        pay_block_txids = node0.getblock(blockhash)["tx"]
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
        ranged = node0.getscripthistory(sh, 0, {"start_height": pay_height, "end_height": pay_height})
        assert_equal([tx["txid"] for tx in ranged["transactions"]], pay_block_txids)
        before = node0.getscripthistory(sh, 0, {"start_height": 0, "end_height": pay_height - 1})
        before_by_id = txs_by_id(before)
        assert pay["txid"] not in before_by_id
        # pay spends the earlier matching output, so it is listed as a spender
        # even though its own matching outputs are outside the height range.
        assert_equal([tx["txid"] for tx in before["spending_transactions"]], [pay["txid"]])
        assert_output_status(before_by_id[utxo["txid"]], utxo["vout"], spent=True, spending_txid=pay["txid"])

        self.log.info("Mempool matches include chained spends without block metadata")
        spend = self.wallet.send_self_transfer(from_node=node0, utxo_to_spend=pay["new_utxos"][0])
        child = self.wallet.send_self_transfer(from_node=node0, utxo_to_spend=spend["new_utxo"])
        for verbosity in (0, 1, 2):
            history = node0.getscripthistory(sh, verbosity, {"include_spending_transactions": False})
            by_id = txs_by_id(history)
            assert_output_status(by_id[pay["txid"]], 0, spent=True, spending_txid=spend["txid"])
            assert_output_status(by_id[spend["txid"]], 0, spent=True, spending_txid=child["txid"])
            for txid in (spend["txid"], child["txid"]):
                assert "blockhash" not in by_id[txid]
                assert "height" not in by_id[txid]
            assert "spending_transactions" not in history
            if verbosity == 0:
                # Mempool matching has separate scriptPubKey and scripthash paths.
                for query in (sh, addr, self.wallet.get_descriptor()):
                    assert_equal(node0.getscripthistory(query), history)

        no_mempool = node0.getscripthistory(sh, 0, {"include_mempool": False})
        for txid in (spend["txid"], child["txid"]):
            assert txid not in txs_by_id(no_mempool)
        assert "spending_transactions" not in no_mempool
        assert_output_status(txs_by_id(no_mempool)[pay["txid"]], 0, spent=False)

        spend_block = self.generate(self.wallet, 1)[0]
        self.wait_for_indexes(node0)

        self.log.info("Reorg removes disconnected transactions from confirmed history")
        node0.invalidateblock(spend_block)
        self.wait_for_indexes(node0)
        history = node0.getscripthistory(sh)
        by_id = txs_by_id(history)
        assert_output_status(by_id[pay["txid"]], 0, spent=True, spending_txid=spend["txid"])
        for txid in (spend["txid"], child["txid"]):
            assert "blockhash" not in by_id[txid]

        assert_equal(node0.getscripthistory(sh, 0, {"include_mempool": False}), no_mempool)

        node0.reconsiderblock(spend_block)

        self.log.info("Spender that does not create matching outputs goes in spending_transactions")
        foreign = self.send_without_change(node0, self.wallet, pay["new_utxos"][1], getnewdestination()[1])
        foreign_txid = foreign["txid"]
        self.generate(self.wallet, 1)
        self.wait_for_indexes(node0)
        for verbosity in (0, 1, 2):
            for include_spenders in (False, True):
                history = node0.getscripthistory(sh, verbosity, {"include_spending_transactions": include_spenders})
                by_id = txs_by_id(history)
                assert_output_status(by_id[pay["txid"]], 0, spent=True, spending_txid=spend["txid"])
                assert_output_status(by_id[pay["txid"]], 1, spent=True, spending_txid=foreign_txid if include_spenders else None)
                assert spend["txid"] in by_id
                assert foreign_txid not in by_id
                if include_spenders:
                    spender, = history["spending_transactions"]
                    assert_equal(spender["txid"], foreign_txid)
                    assert_equal(spender["hex"], foreign["hex"])
                    if verbosity == 0:
                        assert "vout" not in spender
                else:
                    assert "spending_transactions" not in history
        if self.is_cli_compiled():
            assert_equal(node0.cli.getscripthistory(sh, include_spending_transactions=False),
                         node0.getscripthistory(sh, 0, {"include_spending_transactions": False}))

        self.test_transaction_order_and_shared_spenders(node0)
        self.test_unspendable_outputs(node0)
        self.test_asof(node0)

        self.log.info("Confirmed spenders need txospenderindex unless they are themselves scriptPubKey hits")
        self.restart_node(0, extra_args=["-txoscriptindex", "-txindex"])
        self.wait_for_indexes(node0)
        for include_spenders in (False, True):
            assert_raises_rpc_error(-1, "asof requires txospenderindex", node0.getscripthistory, sh, 0,
                                    {"asof": node0.getbestblockhash(), "include_spending_transactions": include_spenders})
        history = node0.getscripthistory(sh, 1)
        by_id = txs_by_id(history)
        pay_entry = by_id[pay["txid"]]
        assert_output_status(pay_entry, 0, spent=True, spending_txid=spend["txid"])
        assert_output_status(pay_entry, 1, spent=True)
        assert foreign_txid not in by_id
        assert "spending_transactions" not in history

        self.log.info("Mempool spenders are included without txospenderindex")
        self.wallet.rescan_utxos()
        new_pay = self.wallet.send_self_transfer(from_node=node0)
        self.generate(self.wallet, 1)
        self.wait_for_indexes(node0)
        mempool_foreign = self.send_without_change(node0, self.wallet, new_pay["new_utxo"], getnewdestination()[1])
        for include_spenders in (False, True):
            history = node0.getscripthistory(sh, 0, {"end_height": node0.getblockcount(), "include_spending_transactions": include_spenders})
            assert_output_status(txs_by_id(history)[new_pay["txid"]], 0, spent=True,
                                 spending_txid=mempool_foreign["txid"] if include_spenders else None)
            assert mempool_foreign["txid"] not in txs_by_id(history)
            if include_spenders:
                spender, = history["spending_transactions"]
                assert_equal(spender["txid"], mempool_foreign["txid"])
                assert_equal(spender["hex"], mempool_foreign["hex"])
                assert "blockhash" not in spender
            else:
                assert "spending_transactions" not in history
        self.test_serialized_size_limit(node0)
        self.test_undo_size_limit(node0)

        self.log.info("Pruned script history works with only txoscriptindex enabled")
        self.restart_node(0, extra_args=["-txoscriptindex", "-txospenderindex=0", "-txindex=0", "-prune=1", "-fastprune"])
        self.wait_for_indexes(node0)
        old_range = {"start_height": pay_height, "end_height": pay_height + 1, "include_mempool": False}
        saved_block = node0.getblock(blockhash, 0)

        # Small fastprune block files avoid mining large blocks. Mine to the
        # regtest prune threshold, using an unrelated script to keep history small.
        _, _, unrelated_address = getnewdestination()
        self.generatetoaddress(node0, 1001 - node0.getblockcount(), unrelated_address)
        recent_block = self.generate(self.wallet, 1)[0]
        recent_txid = node0.getblock(recent_block)["tx"][0]
        self.wait_for_indexes(node0)
        node0.pruneblockchain(node0.getblockcount())

        for verbosity in (0, 1, 2):
            pruned = node0.getscripthistory(sh, verbosity, old_range)
            assert_equal(pruned["transactions"], [])
            assert_equal(sorted(pruned["pruned_blocks"]), sorted([blockhash, spend_block]))
            assert "spending_transactions" not in pruned
            recent = node0.getscripthistory(sh, verbosity, {
                "start_height": pay_height, "include_mempool": False,
            })
            assert_equal([tx["txid"] for tx in recent["transactions"]], [recent_txid])
            assert {blockhash, spend_block}.issubset(recent["pruned_blocks"])

        self.log.info("Restored pruned blocks provide transaction details without undo data")
        node0.submitblock(saved_block)
        restored = node0.getscripthistory(sh, 2, dict(old_range, end_height=pay_height))
        assert "pruned_blocks" not in restored
        assert_equal([tx["txid"] for tx in restored["transactions"]], pay_block_txids)
        restored_pay = txs_by_id(restored)[pay["txid"]]
        assert_equal(restored_pay["hex"], pay["hex"])
        assert "fee" not in restored_pay
        assert all("prevout" not in txin for txin in restored_pay["vin"])


if __name__ == "__main__":
    GetScriptHistoryTest(__file__).main()
