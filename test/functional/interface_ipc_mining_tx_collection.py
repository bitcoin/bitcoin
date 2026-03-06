#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Mining TxCollection interface."""
import asyncio
from test_framework.messages import (
    MAX_BLOCK_WEIGHT,
    MIN_TRANSACTION_WEIGHT,
    ser_uint256,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.ipc_util import (
    destroying,
    load_capnp_modules,
    make_mining_ctx,
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
        self.num_nodes = 1

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()
        # Use this function to also load the capnp modules (we cannot use set_test_params for this,
        # as it is being called before knowing whether capnp is available).
        self.capnp_modules = load_capnp_modules(self.config)

    def run_test(self):
        async def async_routine():
            ctx0, mining0 = await make_mining_ctx(self)

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

        asyncio.run(capnp.run(async_routine()))


if __name__ == '__main__':
    IPCMiningTxCollectionTest(__file__).main()
