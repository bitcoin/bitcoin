#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) interface."""
import asyncio

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.ipc_util import (
    load_capnp_modules,
    make_capnp_init_ctx,
)

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass


class IPCInterfaceTest(BitcoinTestFramework):

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

    def run_echo_test(self):
        self.log.info("Running echo test")
        async def async_routine():
            ctx, init = await make_capnp_init_ctx(self)
            self.log.debug("Create Echo proxy object")
            echo = init.makeEcho(ctx).result
            self.log.debug("Test a few invocations of echo")
            for s in ["hallo", "", "haha"]:
                result_eval = (await echo.echo(ctx, s)).result
                assert_equal(s, result_eval)
            self.log.debug("Destroy the Echo object")
            echo.destroy(ctx)
        asyncio.run(capnp.run(async_routine()))

    def run_mining_test(self):
        self.log.info("Running mining test")
        block_hash_size = 32

        async def async_routine():
            ctx, init = await make_capnp_init_ctx(self)
            self.log.debug("Create Mining proxy object")
            mining = init.makeMining(ctx).result
            self.log.debug("Test simple inspectors")
            assert (await mining.isTestChain(ctx)).result
            assert not (await mining.isInitialBlockDownload(ctx)).result
            blockref = await mining.getTip(ctx)
            assert blockref.hasResult
            assert_equal(len(blockref.result.hash), block_hash_size)
            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            assert_equal(blockref.result.height, current_block_height)

        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.run_echo_test()
        self.run_mining_test()

if __name__ == '__main__':
    IPCInterfaceTest(__file__).main()
