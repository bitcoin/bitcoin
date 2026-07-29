#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Mining TxCollection interface."""
import asyncio
from contextlib import AsyncExitStack
from copy import deepcopy
from test_framework.messages import (
    MAX_BLOCK_WEIGHT,
    MIN_TRANSACTION_WEIGHT,
    ser_uint256,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet
from test_framework.ipc_util import (
    destroying,
    load_capnp_modules,
    make_mining_ctx,
    mining_collect_txs,
    mining_create_block_template,
    mining_get_block,
    tx_collection_make_template,
    tx_collection_unknown_pos,
)

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass


class IPCMiningTxCollectionTest(BitcoinTestFramework):

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()
        self.skip_if_no_py_capnp()

    def set_test_params(self):
        self.num_nodes = 2

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}, {"ipcbind": True}]
        super().setup_nodes()
        # Use this function to also load the capnp modules (we cannot use set_test_params for this,
        # as it is being called before knowing whether capnp is available).
        self.capnp_modules = load_capnp_modules(self.config)

    def run_test(self):
        async def async_routine():
            node = self.nodes[0]
            remote_node = self.nodes[1]
            ctx0, mining0 = await make_mining_ctx(self)
            ctx1, mining1 = await make_mining_ctx(self, node_index=1)
            remote_wallet = MiniWallet(remote_node)
            block_create_options = self.capnp_modules['mining'].BlockCreateOptions()

            self.log.debug("collectTxs() should reject duplicate wtxids")
            try:
                await mining0.collectTxs(ctx0, [ser_uint256(1), ser_uint256(1)])
                raise AssertionError("collectTxs unexpectedly accepted duplicate wtxids")
            except capnp.lib.capnp.KjException as e:
                assert_equal(e.description, f"remote exception: std::exception: duplicate wtxid {ser_uint256(1)[::-1].hex()}")
                assert_equal(e.type, "FAILED")

            self.log.debug("collectTxs() should reject an excessive number of wtxids")
            max_txs = MAX_BLOCK_WEIGHT // MIN_TRANSACTION_WEIGHT
            try:
                await mining0.collectTxs(ctx0, [ser_uint256(i) for i in range(max_txs + 1)])
                raise AssertionError("collectTxs unexpectedly accepted too many wtxids")
            except capnp.lib.capnp.KjException as e:
                assert_equal(e.description, f"remote exception: std::exception: too many wtxids ({max_txs + 1} > {max_txs})")
                assert_equal(e.type, "FAILED")

            self.log.debug("An empty collection can immediately build a coinbase-only template")
            async with AsyncExitStack() as stack:
                tx_collection = await stack.enter_async_context(destroying((await mining0.collectTxs(ctx0, [])).result, ctx0))
                tip = bytes((await mining0.getTip(ctx0)).result.hash)
                response = await tx_collection.makeTemplate(ctx0, tip)
                assert_equal(response.reason, "")
                assert_equal(response.debug, "")
                template = await stack.enter_async_context(destroying(response.result, ctx0))
                block = await mining_get_block(template, ctx0)
                assert_equal(len(block.vtx), 1)

                self.log.debug("Externally generated templates should reject coinbase, fee, sigop, and waitNext accessors")
                for method_name, method in (
                    ("getCoinbaseTx", template.getCoinbaseTx),
                    ("getTxFees", template.getTxFees),
                    ("getTxSigops", template.getTxSigops),
                    ("waitNext", template.waitNext),
                ):
                    try:
                        await method(ctx0)
                        raise AssertionError(f"{method_name} unexpectedly succeeded on external template")
                    except capnp.lib.capnp.KjException as e:
                        assert_equal(e.description, f"remote exception: std::exception: {method_name} is unavailable for externally generated templates")
                        assert_equal(e.type, "FAILED")

            self.log.debug("Run the TxCollection workflow")
            remote_wallet.rescan_utxos()
            self.log.debug("Create a transaction that is shared by both mempools before disconnecting")
            shared_tx = remote_wallet.send_self_transfer(
                from_node=remote_node,
                fee_rate=10,
                confirmed_only=True,
            )
            self.sync_mempools()
            current_tip_info = await mining0.getTip(ctx0)
            current_tip = bytes(current_tip_info.result.hash)

            # Keep the mempools separate for the rest of the test. Remote
            # blocks will be relayed explicitly later instead of reconnecting.
            self.disconnect_nodes(0, 1)

            async with AsyncExitStack() as stack:
                self.log.debug("Create a second transaction that stays only in the remote mempool")
                missing_tx = remote_wallet.send_self_transfer(
                    from_node=remote_node,
                    utxo_to_spend=shared_tx["new_utxo"],
                    fee_rate=10,
                )

                self.log.debug("Remote node builds the reference template that node will reconstruct")
                remote_template = await mining_create_block_template(mining1, stack, ctx1, block_create_options)
                assert remote_template is not None
                remote_block = await mining_get_block(remote_template, ctx1)
                assert_equal([tx.wtxid_hex for tx in remote_block.vtx[1:]], [shared_tx["wtxid"], missing_tx["wtxid"]])

                requested_wtxids = [tx.wtxid for tx in remote_block.vtx[1:]]
                raw_txs = [tx.serialize() for tx in remote_block.vtx[1:]]
                tx_collection = await mining_collect_txs(mining0, stack, ctx0, requested_wtxids)

                # The first transaction is already in node's mempool, but
                # the child transaction only exists on the disconnected
                # remote node.
                assert_equal(await tx_collection_unknown_pos(tx_collection, ctx0), [1])

                self.log.debug("makeTemplate() should fail while transactions are still missing")
                await tx_collection_make_template(
                    tx_collection, stack, ctx0, current_tip, reject_reason="missing-txs"
                )

                self.log.debug("Reject unexpected transactions in addMissingTxs(), leaving the collection unchanged")
                unexpected_tx = remote_wallet.create_self_transfer(fee_rate=10, confirmed_only=True)
                try:
                    await tx_collection.addMissingTxs(ctx0, [raw_txs[1], unexpected_tx["tx"].serialize()])
                    raise AssertionError("addMissingTxs unexpectedly accepted an unknown wtxid")
                except capnp.lib.capnp.KjException as e:
                    assert_equal(e.description, f"remote exception: std::exception: unexpected wtxid {unexpected_tx['wtxid']}")
                    assert_equal(e.type, "FAILED")
                # The unexpected transaction causes the whole call to fail, so
                # the missing transaction it was submitted with is not added.
                assert_equal(await tx_collection_unknown_pos(tx_collection, ctx0), [1])

                self.log.debug("Reject null transactions in addMissingTxs(), leaving the collection unchanged")
                # An empty Data field deserializes to a null CTransactionRef.
                try:
                    await tx_collection.addMissingTxs(ctx0, [raw_txs[1], b""])
                    raise AssertionError("addMissingTxs unexpectedly accepted a null transaction")
                except capnp.lib.capnp.KjException as e:
                    assert_equal(e.description, "remote exception: std::exception: unexpected null transaction")
                    assert_equal(e.type, "FAILED")
                assert_equal(await tx_collection_unknown_pos(tx_collection, ctx0), [1])

                self.log.debug("Reject a list with more transactions than the whole collection, leaving it unchanged")
                try:
                    await tx_collection.addMissingTxs(ctx0, [raw_txs[1]] * 3)
                    raise AssertionError("addMissingTxs unexpectedly accepted an oversized list")
                except capnp.lib.capnp.KjException as e:
                    assert_equal(e.description, "remote exception: std::exception: too many transactions (3 > 2)")
                    assert_equal(e.type, "FAILED")
                assert_equal(await tx_collection_unknown_pos(tx_collection, ctx0), [1])

                self.log.debug("Add the missing transaction")
                await tx_collection.addMissingTxs(ctx0, [raw_txs[1]])
                assert_equal(await tx_collection_unknown_pos(tx_collection, ctx0), [])

                # Mine an empty block so the reference transactions stay in
                # both mempools while the tip-handling checks run.
                future_block = self.generateblock(
                    remote_node,
                    output="raw(52)",
                    transactions=[],
                    submit=False,
                    sync_fun=self.no_op,
                )["hex"]
                assert_equal(remote_node.submitblock(future_block), None)
                future_tip_info = await mining1.getTip(ctx1)
                future_tip = bytes(future_tip_info.result.hash)

                self.log.debug("makeTemplate() should reject a prevhash that does not match the current tip")
                await tx_collection_make_template(
                    tx_collection, stack, ctx0, future_tip, reject_reason="inconclusive-not-best-prevblk"
                )

                self.log.debug("Relay the remote block and wait for the local tip to catch up")
                assert_equal(node.submitblock(future_block), None)
                self.wait_until(lambda: node.getbestblockhash() == remote_node.getbestblockhash())

                self.log.debug("makeTemplate() should report a prevhash the tip has moved past as stale")
                await tx_collection_make_template(
                    tx_collection, stack, ctx0, current_tip, reject_reason="stale-prevblk"
                )

                self.log.debug("Remote node rebuilds the reference template on the new tip")
                remote_template = await mining_create_block_template(mining1, stack, ctx1, block_create_options)
                assert remote_template is not None
                remote_tip_info = await mining1.getTip(ctx1)
                remote_tip = bytes(remote_tip_info.result.hash)
                remote_block = await mining_get_block(remote_template, ctx1)
                assert_equal([tx.wtxid_hex for tx in remote_block.vtx[1:]], [shared_tx["wtxid"], missing_tx["wtxid"]])

                template = await tx_collection_make_template(tx_collection, stack, ctx0, remote_tip)
                local_block = await mining_get_block(template, ctx0)

                assert_equal([tx.wtxid_hex for tx in local_block.vtx[1:]], [tx.wtxid_hex for tx in remote_block.vtx[1:]])

                self.log.debug("makeTemplate() validates a client-provided coinbase")
                remote_coinbase = remote_block.vtx[0]
                template_cb = await tx_collection_make_template(
                    tx_collection, stack, ctx0, remote_tip, coinbase=remote_coinbase.serialize()
                )
                block_cb = await mining_get_block(template_cb, ctx0)
                assert_equal(block_cb.vtx[0].serialize(), remote_coinbase.serialize())

                self.log.debug("makeTemplate() rejects an overpaying client-provided coinbase")
                overpaying_coinbase = deepcopy(remote_coinbase)
                overpaying_coinbase.vout[0].nValue += 1
                await tx_collection_make_template(
                    tx_collection, stack, ctx0, remote_tip,
                    coinbase=overpaying_coinbase.serialize(),
                    reject_reason="bad-cb-amount",
                )

                self.log.debug("Solve the reconstructed block and submit the same solution to both templates")
                # makeTemplate() leaves the merkle root unset (it validates with
                # check_merkle_root=false); submitSolution() fills it in. Set it
                # here too so the solved proof-of-work matches the submitted block.
                local_block.hashMerkleRoot = local_block.calc_merkle_root()
                local_block.solve()
                version = local_block.nVersion
                time = local_block.nTime
                nonce = local_block.nNonce
                coinbase = local_block.vtx[0].serialize()

                submitted_local = (await template.submitSolution(ctx0, version, time, nonce, coinbase)).result
                assert_equal(submitted_local, True)

                submitted_remote = (await remote_template.submitSolution(ctx1, version, time, nonce, coinbase)).result
                assert_equal(submitted_remote, True)
                assert_equal(node.getbestblockhash(), remote_node.getbestblockhash())

                self.log.debug("makeTemplate() should reject collected transactions already in the chain")
                # The collection still references the transactions just mined
                # above, so reassembling them on the new tip is a BIP30
                # duplicate and fails validation.
                new_tip = bytes((await mining0.getTip(ctx0)).result.hash)
                await tx_collection_make_template(
                    tx_collection, stack, ctx0, new_tip, reject_reason="bad-txns-BIP30"
                )

        asyncio.run(capnp.run(async_routine()))


if __name__ == '__main__':
    IPCMiningTxCollectionTest(__file__).main()
