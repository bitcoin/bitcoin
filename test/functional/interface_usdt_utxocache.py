#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

""" Tests the utxocache:* tracepoint API interface.
    See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#context-utxocache
"""

from test_framework.messages import COIN
from test_framework.test_framework import BitcoinTestFramework
from test_framework.usdt import USDT
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet

FLUSHMODE_NAME = {
    0: "NONE",
    1: "IF_NEEDED",
    2: "PERIODIC",
    3: "FORCE_FLUSH",
    4: "FORCE_SYNC",
}


class UTXOCacheTracepointTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.extra_args = [["-txindex"]]

    def start_nodes(self):
        change_fields = [("txid", "h"), ("index", "i"), ("height", "i"),
                         ("value", "i"), ("is_coinbase", "i")]
        self.trace = USDT(self.nodes[0], {
            "utxocache:add": change_fields,
            "utxocache:spent": change_fields,
            "utxocache:uncache": change_fields,
            "utxocache:flush": [("duration", "i"), ("mode", "i"), ("size", "i"),
                               ("memory", "i"), ("for_prune", "i")],
        })
        super().start_nodes()

    def skip_test_if_missing_module(self):
        self.skip_if_platform_not_linux()
        self.skip_if_no_bitcoind_tracepoints()
        self.skip_if_no_usdt_tools()
        self.skip_if_running_under_valgrind()

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        self.test_uncache()
        self.test_add_spent()
        self.test_flush()

    def test_uncache(self):
        """ Tests the utxocache:uncache tracepoint API.
        https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#tracepoint-utxocacheuncache
        """
        # To trigger an UTXO uncache from the cache, we create an invalid transaction
        # spending a not-cached, but existing UTXO. During transaction validation, this
        # the UTXO is added to the utxo cache, but as the transaction is invalid, it's
        # uncached again.
        self.log.info("testing the utxocache:uncache tracepoint API")

        # Retrieve the txid for the UTXO created in the first block. This UTXO is not
        # in our UTXO cache.
        EARLY_BLOCK_HEIGHT = 1
        block_1_hash = self.nodes[0].getblockhash(EARLY_BLOCK_HEIGHT)
        block_1 = self.nodes[0].getblock(block_1_hash)
        block_1_coinbase_txid = block_1["tx"][0]

        # Create a transaction and invalidate it by changing the txid of the previous
        # output to the coinbase txid of the block at height 1.
        invalid_tx = self.wallet.create_self_transfer()["tx"]
        invalid_tx.vin[0].prevout.hash = int(block_1_coinbase_txid, 16)

        self.trace.clear()

        self.log.info(
            "testmempoolaccept the invalid transaction to trigger an UTXO-cache uncache")
        result = self.nodes[0].testmempoolaccept(
            [invalid_tx.serialize().hex()])[0]
        assert_equal(result["allowed"], False)

        events = [event for event in self.trace.read() if event.probe == "utxocache:uncache"]
        assert_equal(1, len(events))
        event = events[0]
        assert_equal(block_1_coinbase_txid, event.txid)
        assert_equal(0, event.index)  # prevout index
        assert_equal(EARLY_BLOCK_HEIGHT, event.height)
        assert_equal(50 * COIN, event.value)
        assert_equal(True, event.is_coinbase)

    def test_add_spent(self):
        """ Tests the utxocache:add utxocache:spent tracepoint API
            See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#tracepoint-utxocacheadd
            and https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#tracepoint-utxocachespent
        """

        self.log.info(
            "test the utxocache:add and utxocache:spent tracepoint API")

        self.log.info("create an unconfirmed transaction")
        self.wallet.send_self_transfer(from_node=self.nodes[0])

        # We mine a block to trace changes (add/spent) to the active in-memory cache
        # of the UTXO set (see CoinsTip() of CCoinsViewCache). However, in some cases
        # temporary clones of the active cache are made. For example, during mining with
        # the generate RPC call, the block is first tested in TestBlockValidity(). There,
        # a clone of the active cache is modified during a test ConnectBlock() call.
        # These are implementation details we don't want to test here. Thus, after
        # mining, we invalidate the block, clear the events, and then trace the cache
        # changes to the active utxo cache.
        self.log.info("mine and invalidate a block that is later reconsidered")
        block_hash = self.generate(self.wallet, 1)[0]
        self.nodes[0].invalidateblock(block_hash)

        EXPECTED_ADDS = 2
        EXPECTED_SPENTS = 1
        expected_utxocache_adds = []
        expected_utxocache_spents = []
        self.trace.clear()

        # We trigger a block re-connection. This causes changes (add/spent)
        # to the UTXO-cache which in turn triggers the tracepoints.
        self.log.info("reconsider the previously invalidated block")
        self.nodes[0].reconsiderblock(block_hash)

        block = self.nodes[0].getblock(block_hash, 2)
        for (block_index, tx) in enumerate(block["tx"]):
            for vin in tx["vin"]:
                if "coinbase" not in vin:
                    prevout_tx = self.nodes[0].getrawtransaction(
                        vin["txid"], True)
                    prevout_tx_block = self.nodes[0].getblockheader(
                        prevout_tx["blockhash"])
                    spends_coinbase = "coinbase" in prevout_tx["vin"][0]
                    expected_utxocache_spents.append({
                        "txid": vin["txid"],
                        "index": vin["vout"],
                        "height": prevout_tx_block["height"],
                        "value": int(prevout_tx["vout"][vin["vout"]]["value"] * COIN),
                        "is_coinbase": spends_coinbase,
                    })
            for (i, vout) in enumerate(tx["vout"]):
                if vout["scriptPubKey"]["type"] != "nulldata":
                    expected_utxocache_adds.append({
                        "txid": tx["txid"],
                        "index": i,
                        "height": block["height"],
                        "value": int(vout["value"] * COIN),
                        "is_coinbase": block_index == 0,
                    })

        events = self.trace.read()
        actual_utxocache_adds = [event for event in events if event.probe == "utxocache:add"]
        actual_utxocache_spents = [event for event in events if event.probe == "utxocache:spent"]

        assert_equal(EXPECTED_ADDS, len(expected_utxocache_adds), len(actual_utxocache_adds))
        assert_equal(EXPECTED_SPENTS, len(expected_utxocache_spents), len(actual_utxocache_spents))

        self.log.info(
            f"check that we successfully traced {EXPECTED_ADDS} adds and {EXPECTED_SPENTS} spent")

        # Check that all expected tracepoints are received, but not the order they were received in.
        # Tracepoint ordering is not strictly guaranteed, so this comparison avoids intermittent failures in the test.
        def cache_event_to_key(event):
            return (
                event.txid,
                event.index,
                event.height,
                event.value,
                event.is_coinbase
            )

        expected_add_keys = {(e["txid"], e["index"], e["height"], e["value"], e["is_coinbase"])
                             for e in expected_utxocache_adds}
        expected_spent_keys = {(e["txid"], e["index"], e["height"], e["value"], e["is_coinbase"])
                              for e in expected_utxocache_spents}

        actual_add_keys = {cache_event_to_key(e) for e in actual_utxocache_adds}
        actual_spent_keys = {cache_event_to_key(e) for e in actual_utxocache_spents}

        assert_equal(expected_add_keys, actual_add_keys)
        assert_equal(expected_spent_keys, actual_spent_keys)

    def test_flush(self):
        """ Tests the utxocache:flush tracepoint API.
            See https://github.com/bitcoin/bitcoin/blob/master/doc/tracing.md#tracepoint-utxocacheflush"""

        self.log.info("test the utxocache:flush tracepoint API")
        flushes = []

        def check_flushes(expected):
            events = [event for event in self.trace.read() if event.probe == "utxocache:flush"]
            assert_equal(len(expected), len(events))
            for event in events:
                expected.remove({
                    "mode": FLUSHMODE_NAME[event.mode],
                    "for_prune": event.for_prune,
                    "size": event.size,
                })
                assert event.memory > 0
                assert event.duration > 0
            assert_equal([], expected)
            flushes.extend(events)

        self.trace.clear()
        self.log.info("stop the node to flush the UTXO cache")
        UTXOS_IN_CACHE = 3 # might need to be changed if the earlier tests are modified
        # A node shutdown causes two flushes. One that flushes UTXOS_IN_CACHE
        # UTXOs and one that flushes 0 UTXOs. Normally the 0-UTXO-flush is the
        # second flush, however it can happen that the order changes.
        self.stop_node(0)

        check_flushes([
            {"mode": "FORCE_FLUSH", "for_prune": False, "size": UTXOS_IN_CACHE},
            {"mode": "FORCE_FLUSH", "for_prune": False, "size": 0},
        ])

        self.log.info("restart the node with -prune")
        self.start_node(0, ["-fastprune=1", "-prune=1"])

        BLOCKS_TO_MINE = 350
        self.log.info(f"mine {BLOCKS_TO_MINE} blocks to be able to prune")
        self.generate(self.wallet, BLOCKS_TO_MINE)

        self.log.info("test the utxocache:flush tracepoint API with pruning")
        self.trace.clear()

        self.log.info("prune blockchain to trigger a flush for pruning")
        self.nodes[0].pruneblockchain(315)

        check_flushes([{"mode": "NONE", "for_prune": True, "size": BLOCKS_TO_MINE}])
        assert_equal(3, len(flushes))


if __name__ == '__main__':
    UTXOCacheTracepointTest(__file__).main()
