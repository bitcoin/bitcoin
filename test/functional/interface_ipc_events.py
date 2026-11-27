#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Events interface."""
import asyncio
from decimal import Decimal
from io import BytesIO

from test_framework.messages import (
    CBlock,
    CTransaction,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_not_equal,
)
from test_framework.wallet import MiniWallet
from test_framework.ipc_util import (
    assert_capnp_raises,
    load_capnp_modules,
    make_capnp_init_ctx,
    wait_and_do,
)

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass

REASON_REPLACED = "replaced"
REASON_BLOCK = "block"
REASON_CONFLICT = "conflict"

WAIT_TIMEOUT_MS = 30_000

# Must match interfaces::MAX_BATCH_BYTES.
MAX_BATCH_BYTES = 16 << 20
# Enough ~90 kB transactions to exceed MAX_BATCH_BYTES.
N_BIG_TXS = 200
BIG_TX_VSIZE = 90_000
# Enough ~90 kB transactions to exceed -ipceventsbuffer=1.
N_OVERFLOW_TXS = 20


def payload(event, kind):
    """Assert which union member is set and return it."""
    assert_equal(event.which(), kind)
    return getattr(event, kind)


def hash_hex(data):
    """Hex of a uint256 sent over IPC, which is in internal byte order."""
    return bytes(data)[::-1].hex()


def deser_block(data):
    block = CBlock()
    block.deserialize(BytesIO(bytes(data)))
    return block


def deser_tx(data):
    tx = CTransaction()
    tx.deserialize(BytesIO(bytes(data)))
    return tx


class IPCEventsTest(BitcoinTestFramework):

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()
        self.skip_if_no_py_capnp()

    def set_test_params(self):
        self.num_nodes = 1

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()
        self.capnp_modules = load_capnp_modules(self.config)

    def mempool_sequence(self):
        return self.nodes[0].getrawmempool(verbose=False, mempool_sequence=True)["mempool_sequence"]

    def run_test(self):
        node = self.nodes[0]
        miniwallet = MiniWallet(node)
        self.generate(miniwallet, 1)

        async def async_routine():
            ctx, init = await make_capnp_init_ctx(self)

            async def make_events(**include_data):
                options = self.capnp_modules["events"].EventsOptions()
                for name, value in include_data.items():
                    options.init(name).includeData = value
                return (await init.makeEvents(ctx, options)).result

            all_events = dict(blockConnected=True, blockDisconnected=True, txAddedToMempool=True, txRemovedFromMempool=True)

            self.log.info("Options default to no events, which is rejected")
            await assert_capnp_raises(lambda: init.makeEvents(ctx),
                                      "remote exception: std::exception: at least one event type must be enabled")
            await assert_capnp_raises(lambda: make_events(),
                                      "remote exception: std::exception: at least one event type must be enabled")

            # Await so the subscription exists before generating events.
            events = await make_events(**all_events)
            last = 0

            async def next_events(expected_count):
                nonlocal last
                result = (await events.waitNext(ctx, last, timeout=WAIT_TIMEOUT_MS)).result
                assert_equal(len(result), expected_count)
                for i, event in enumerate(result):
                    assert_equal(event.sequence, last + 1 + i)
                last = result[-1].sequence
                return result

            self.log.info("Block connected")
            block_hash = self.generate(node, 1)[0]
            [event] = await next_events(1)
            block = payload(event, "blockConnected")
            assert_equal(block.height, node.getblockcount())
            assert_equal(hash_hex(block.hash), block_hash)
            assert_equal(deser_block(block.data).hash_hex, block_hash)

            self.log.info("Transaction added, with fee, vsize and mempool sequence")
            utxo = miniwallet.get_utxo()
            seq = self.mempool_sequence()
            tx1 = miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=10)
            [event] = await next_events(1)
            added = payload(event, "txAddedToMempool")
            assert_equal(deser_tx(added.data).txid_hex, tx1["txid"])
            assert_equal(hash_hex(added.txid), tx1["txid"])
            assert_equal(hash_hex(added.wtxid), tx1["wtxid"])
            assert_not_equal(tx1["txid"], tx1["wtxid"])
            assert_equal(added.mempoolSequence, seq)
            entry = node.getmempoolentry(tx1["txid"])
            assert_equal(added.fee, int(entry["fees"]["base"] * 100_000_000))
            assert_equal(added.vsize, entry["vsize"])

            self.log.info("Replacement reports removal reason")
            tx2 = miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=50)
            result = await next_events(2)
            removed = payload(result[0], "txRemovedFromMempool")
            added = payload(result[1], "txAddedToMempool")
            assert_equal(deser_tx(removed.data).txid_hex, tx1["txid"])
            assert_equal(hash_hex(removed.txid), tx1["txid"])
            assert_equal(hash_hex(removed.wtxid), tx1["wtxid"])
            assert_equal(removed.reason, REASON_REPLACED)
            assert_equal(deser_tx(added.data).txid_hex, tx2["txid"])
            assert_equal(removed.mempoolSequence + 1, added.mempoolSequence)

            self.log.info("Confirmed transactions are reported as removed with reason block")
            seq = self.mempool_sequence()
            block_hash = self.generate(node, 1)[0]
            result = await next_events(2)
            removed = payload(result[0], "txRemovedFromMempool")
            assert_equal(hash_hex(removed.txid), tx2["txid"])
            assert_equal(removed.reason, REASON_BLOCK)
            assert_equal(removed.mempoolSequence, seq)
            block = payload(result[1], "blockConnected")
            assert_equal([tx.txid_hex for tx in deser_block(block.data).vtx[1:]], [tx2["txid"]])

            self.log.info("Block disconnected")
            height = node.getblockcount()
            node.invalidateblock(block_hash)
            node.syncwithvalidationinterfacequeue()
            result = await next_events(2)
            block = payload(result[0], "blockDisconnected")
            assert_equal(block.height, height)
            assert_equal(hash_hex(block.hash), block_hash)
            assert_equal(deser_block(block.data).hash_hex, block_hash)
            assert_equal(deser_tx(payload(result[1], "txAddedToMempool").data).txid_hex, tx2["txid"])
            node.reconsiderblock(block_hash)
            node.syncwithvalidationinterfacequeue()
            await next_events(2)

            self.log.info("In a reorg connecting several blocks, block removals come before their blockConnected events")
            tx_a = miniwallet.send_self_transfer(from_node=node)
            block_a = self.generate(node, 1)[0]
            tx_b = miniwallet.send_self_transfer(from_node=node)
            block_b = self.generate(node, 1)[0]
            await next_events(6)
            node.invalidateblock(block_a)
            node.syncwithvalidationinterfacequeue()
            await next_events(4)
            # A competing empty block, so reconsidering block_a is a reorg that connects block_a and block_b in one step.
            block_c = self.generateblock(node, output=miniwallet.get_address(), transactions=[])["hash"]
            await next_events(1)
            node.reconsiderblock(block_a)
            node.syncwithvalidationinterfacequeue()
            result = await next_events(5)
            assert_equal([e.which() for e in result], ["blockDisconnected", "txRemovedFromMempool", "txRemovedFromMempool", "blockConnected", "blockConnected"])
            assert_equal(hash_hex(payload(result[0], "blockDisconnected").hash), block_c)
            for i, tx in ((1, tx_a), (2, tx_b)):
                removed = payload(result[i], "txRemovedFromMempool")
                assert_equal(hash_hex(removed.txid), tx["txid"])
                assert_equal(removed.reason, REASON_BLOCK)
            assert_equal(hash_hex(payload(result[3], "blockConnected").hash), block_a)
            assert_equal(hash_hex(payload(result[4], "blockConnected").hash), block_b)

            self.log.info("Mempool transactions conflicting with a block are reported with reason conflict")
            utxo = miniwallet.get_utxo()
            in_mempool = miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=10)
            miniwallet.get_utxo(txid=in_mempool["txid"])  # Drop its output, it will not confirm.
            conflict = miniwallet.create_self_transfer(utxo_to_spend=utxo, fee_rate=20)
            await next_events(1)
            block_hash = self.generateblock(node, output=miniwallet.get_address(), transactions=[conflict["hex"]])["hash"]
            result = await next_events(2)
            removed = payload(result[0], "txRemovedFromMempool")
            assert_equal(hash_hex(removed.txid), in_mempool["txid"])
            assert_equal(removed.reason, REASON_CONFLICT)
            assert_equal(hash_hex(payload(result[1], "blockConnected").hash), block_hash)

            self.log.info("Unacknowledged events are delivered again")
            self.generate(node, 2)
            first = (await events.waitNext(ctx, last, maxEvents=1, timeout=WAIT_TIMEOUT_MS)).result
            assert_equal(len(first), 1)
            again = (await events.waitNext(ctx, last, timeout=WAIT_TIMEOUT_MS)).result
            assert_equal([e.sequence for e in again], [first[0].sequence, first[0].sequence + 1])
            last = again[-1].sequence

            self.log.info("maxEvents = 0 is rejected")
            await assert_capnp_raises(lambda: events.waitNext(ctx, last, maxEvents=0),
                                      "remote exception: std::exception: max_events must be greater than 0")

            self.log.info("after beyond the last sequence is rejected")
            await assert_capnp_raises(lambda: events.waitNext(ctx, last + 1),
                                      "remote exception: std::exception: after must not be greater than the last event sequence")

            self.log.info("Timeout returns no events")
            assert_equal(len((await events.waitNext(ctx, last, timeout=100)).result), 0)

            self.log.info("interrupt() aborts a pending waitNext")
            result = await wait_and_do(events.waitNext(ctx, last), events.interrupt())
            assert_equal(len(result.result), 0)

            self.log.info("Events arrive while waitNext is pending")
            result = await wait_and_do(events.waitNext(ctx, last, timeout=WAIT_TIMEOUT_MS), lambda: self.generate(node, 1))
            payload(result.result[0], "blockConnected")
            assert_equal(result.result[0].sequence, last + 1)

            await events.destroy(ctx)

            self.log.info("Options filter event types, and sequence counts only subscribed events")
            blocks_only = await make_events(blockConnected=True, blockDisconnected=True)
            tx_added_only = await make_events(txAddedToMempool=True)
            tx = miniwallet.send_self_transfer(from_node=node)
            block_hash = self.generate(node, 1)[0]
            [event] = (await blocks_only.waitNext(ctx, after=0, timeout=WAIT_TIMEOUT_MS)).result
            assert_equal(event.sequence, 1)
            assert_equal(deser_block(payload(event, "blockConnected").data).hash_hex, block_hash)
            [event] = (await tx_added_only.waitNext(ctx, after=0, timeout=WAIT_TIMEOUT_MS)).result
            assert_equal(event.sequence, 1)
            assert_equal(deser_tx(payload(event, "txAddedToMempool").data).txid_hex, tx["txid"])
            await blocks_only.destroy(ctx)
            await tx_added_only.destroy(ctx)

            self.log.info("Without includeData, data is unset and metadata is still reported")
            no_data = await make_events(blockConnected=False, blockDisconnected=False, txAddedToMempool=False, txRemovedFromMempool=False)
            utxo = miniwallet.get_utxo()
            seq = self.mempool_sequence()
            tx1 = miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=10)
            entry = node.getmempoolentry(tx1["txid"])
            tx2 = miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, fee_rate=50)
            block_hash = self.generate(node, 1)[0]
            height = node.getblockcount()
            node.invalidateblock(block_hash)
            node.syncwithvalidationinterfacequeue()
            result = (await no_data.waitNext(ctx, after=0, timeout=WAIT_TIMEOUT_MS)).result
            assert_equal([e.which() for e in result], ["txAddedToMempool", "txRemovedFromMempool", "txAddedToMempool", "txRemovedFromMempool", "blockConnected", "blockDisconnected", "txAddedToMempool"])
            assert_equal([getattr(e, e.which())._has("data") for e in result], [False] * len(result))
            added = payload(result[0], "txAddedToMempool")
            assert_equal(hash_hex(added.txid), tx1["txid"])
            assert_equal(hash_hex(added.wtxid), tx1["wtxid"])
            assert_equal(added.mempoolSequence, seq)
            assert_equal(added.fee, int(entry["fees"]["base"] * 100_000_000))
            assert_equal(added.vsize, entry["vsize"])
            removed = payload(result[1], "txRemovedFromMempool")
            assert_equal(hash_hex(removed.txid), tx1["txid"])
            assert_equal(hash_hex(removed.wtxid), tx1["wtxid"])
            assert_equal(removed.mempoolSequence, seq + 1)
            assert_equal(removed.reason, REASON_REPLACED)
            assert_equal(hash_hex(payload(result[2], "txAddedToMempool").txid), tx2["txid"])
            removed = payload(result[3], "txRemovedFromMempool")
            assert_equal(hash_hex(removed.txid), tx2["txid"])
            assert_equal(removed.reason, REASON_BLOCK)
            for e, kind in ((result[4], "blockConnected"), (result[5], "blockDisconnected")):
                block = payload(e, kind)
                assert_equal(hash_hex(block.hash), block_hash)
                assert_equal(block.height, height)
            assert_equal(hash_hex(payload(result[6], "txAddedToMempool").txid), tx2["txid"])
            node.reconsiderblock(block_hash)
            node.syncwithvalidationinterfacequeue()
            await no_data.destroy(ctx)

            self.log.info("Batches are limited to MAX_BATCH_BYTES")
            fanout = miniwallet.send_self_transfer_multi(from_node=node, num_outputs=N_BIG_TXS)
            self.generate(node, 1)
            tx_events = await make_events(txAddedToMempool=True)
            for utxo in fanout["new_utxos"]:
                miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, target_vsize=BIG_TX_VSIZE, fee_rate=Decimal("0.0001"))
            received = 0
            batches = 0
            last = 0
            while received < N_BIG_TXS:
                result = (await tx_events.waitNext(ctx, last, timeout=WAIT_TIMEOUT_MS)).result
                assert_greater_than_or_equal(MAX_BATCH_BYTES, sum(len(bytes(payload(e, "txAddedToMempool").data)) for e in result))
                received += len(result)
                batches += 1
                last = result[-1].sequence
            assert_equal(received, N_BIG_TXS)
            assert_greater_than(batches, 1)

            await tx_events.destroy(ctx)
            self.generate(node, 1)

            self.log.info("Oldest events are dropped past -ipceventsbuffer")
            self.restart_node(0, extra_args=["-ipceventsbuffer=1"])
            ctx, init = await make_capnp_init_ctx(self)
            fanout = miniwallet.send_self_transfer_multi(from_node=node, num_outputs=N_OVERFLOW_TXS)
            self.generate(node, 1)
            tx_events = await make_events(txAddedToMempool=True)
            for utxo in fanout["new_utxos"]:
                miniwallet.send_self_transfer(from_node=node, utxo_to_spend=utxo, target_vsize=BIG_TX_VSIZE, fee_rate=Decimal("0.0001"))
            node.syncwithvalidationinterfacequeue()
            result = (await tx_events.waitNext(ctx, after=0, timeout=WAIT_TIMEOUT_MS)).result
            assert_greater_than(result[0].sequence, 1)
            assert_equal([e.sequence for e in result], list(range(result[0].sequence, N_OVERFLOW_TXS + 1)))
            assert_greater_than_or_equal(1 << 20, sum(len(bytes(payload(e, "txAddedToMempool").data)) for e in result))
            await tx_events.destroy(ctx)
            self.generate(node, 1)

            self.log.info("Shutdown is not blocked by a pending waitNext")
            events = await make_events(**all_events)
            pending = events.waitNext(ctx, after=0)
            await asyncio.sleep(0.1)
            self.stop_node(0)
            try:
                assert_equal(len((await pending).result), 0)
            except capnp.lib.capnp.KjException:
                pass  # The connection may close before the reply is sent.

        asyncio.run(capnp.run(async_routine()))

        self.log.info("-ipceventsbuffer below 1 is rejected")
        node.assert_start_raises_init_error(["-ipceventsbuffer=0"], "Error: -ipceventsbuffer must be at least 1")


if __name__ == '__main__':
    IPCEventsTest(__file__).main()
