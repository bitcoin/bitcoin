#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Mining TxCollection interface."""
import asyncio
from contextlib import AsyncExitStack
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

            self.log.debug("Create and destroy an empty collection")
            async with destroying((await mining0.collectTxs(ctx0, [])).result, ctx0):
                pass

            self.log.debug("Run the TxCollection workflow")
            remote_wallet.rescan_utxos()
            self.log.debug("Create a transaction that is shared by both mempools before disconnecting")
            shared_tx = remote_wallet.send_self_transfer(
                from_node=remote_node,
                fee_rate=10,
                confirmed_only=True,
            )
            self.sync_mempools()

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
                tx_collection = await mining_collect_txs(mining0, stack, ctx0, requested_wtxids)

                # The first transaction is already in node's mempool, but
                # the child transaction only exists on the disconnected
                # remote node.
                assert_equal(await tx_collection_unknown_pos(tx_collection, ctx0), [1])

        asyncio.run(capnp.run(async_routine()))


if __name__ == '__main__':
    IPCMiningTxCollectionTest(__file__).main()
