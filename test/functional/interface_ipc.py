#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) interface."""
import asyncio

from contextlib import ExitStack
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.ipc_util import (
    load_capnp_modules,
    make_capnp_init_ctx,
    make_mining_ctx,
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
            ctx, mining = await make_mining_ctx(self)
            self.log.debug("Test simple inspectors")
            assert (await mining.isTestChain(ctx)).result
            assert not (await mining.isInitialBlockDownload(ctx)).result
            blockref = await mining.getTip(ctx)
            assert blockref.hasResult
            assert_equal(len(blockref.result.hash), block_hash_size)
            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            assert_equal(blockref.result.height, current_block_height)

        asyncio.run(capnp.run(async_routine()))

    def run_deprecated_mining_test(self):
        self.log.info("Running deprecated mining interface test")
        async def async_routine():
            node = self.nodes[0]
            connection = await capnp.AsyncIoStream.create_unix_connection(node.ipc_socket_path)
            init = capnp.TwoPartyClient(connection).bootstrap().cast_as(self.capnp_modules['init'].Init)
            self.log.debug("Calling deprecated makeMiningOld2 should raise an error")
            try:
                await init.makeMiningOld2()
                raise AssertionError("makeMiningOld2 unexpectedly succeeded")
            except capnp.KjException as e:
                assert_equal(e.description, "remote exception: std::exception: Old mining interface (@2) not supported. Please update your client!")
                assert_equal(e.type, "FAILED")
        asyncio.run(capnp.run(async_routine()))

    def run_unclean_disconnect_test(self):
        """Test behavior when disconnecting during an IPC call that later
        returns a non-null interface pointer. This used to cause a crash as
        reported https://github.com/bitcoin/bitcoin/issues/34250, but now just
        results in a cancellation log message"""
        node = self.nodes[0]
        self.log.info("Running disconnect during BlockTemplate.waitNext")
        timeout = self.rpc_timeout * 1000.0
        disconnected_log_check = ExitStack()

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            self.log.debug("Create a template")
            opts = self.capnp_modules['mining'].BlockCreateOptions()
            template = (await mining.createNewBlock(ctx, opts)).result

            self.log.debug("Wait for a new template")
            waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
            waitoptions.timeout = timeout
            waitoptions.feeThreshold = 1
            with node.assert_debug_log(expected_msgs=["BlockTemplate.waitNext", "IPC server post request"], timeout=2):
                promise = template.waitNext(ctx, waitoptions)
                await asyncio.sleep(0.1)
            disconnected_log_check.enter_context(node.assert_debug_log(expected_msgs=["IPC server: socket disconnected", "canceled while executing"], timeout=2))
            del promise

        asyncio.run(capnp.run(async_routine()))

        # Wait for socket disconnected log message, then generate a block to
        # cause the waitNext() call to return a new template. Look for a
        # canceled IPC log message after waitNext returns.
        with node.assert_debug_log(expected_msgs=["interrupted (canceled)"], timeout=2):
            disconnected_log_check.close()
            self.generate(node, 1)

    def run_thread_busy_test(self):
        """Test behavior when sending multiple calls to the same server thread
        which used to cause a crash as reported
        https://github.com/bitcoin/bitcoin/issues/33923."""
        node = self.nodes[0]
        self.log.info("Running thread busy test")
        timeout = self.rpc_timeout * 1000.0

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            self.log.debug("Create a template")
            opts = self.capnp_modules['mining'].BlockCreateOptions()
            template = (await mining.createNewBlock(ctx, opts)).result

            self.log.debug("Wait for a new template")
            waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
            waitoptions.timeout = timeout
            waitoptions.feeThreshold = 1

            # Make multiple waitNext calls where the first will start to
            # execute, and the second and third will be posted waiting to
            # execute. Previously, the third call would fail calling
            # mp::Waiter::post() because the waiting function slot is occupied,
            # but now posts are queued.
            with node.assert_debug_log(expected_msgs=["BlockTemplate.waitNext", "IPC server post request"], timeout=2):
                promise1 = template.waitNext(ctx, waitoptions)
                await asyncio.sleep(0.1)
            with node.assert_debug_log(expected_msgs=["BlockTemplate.waitNext", "IPC server post request"], timeout=2):
                promise2 = template.waitNext(ctx, waitoptions)
                await asyncio.sleep(0.1)
            with node.assert_debug_log(expected_msgs=["BlockTemplate.waitNext", "IPC server post request"], timeout=2):
                promise3 = template.waitNext(ctx, waitoptions)
                await asyncio.sleep(0.1)

            # Generate a new block to make the active waitNext calls return, then clean up.
            with node.assert_debug_log(expected_msgs=["IPC server send response"], timeout=2):
                self.generate(node, 1, sync_fun=self.no_op)
            await ((await promise1).result).destroy(ctx)
            await ((await promise2).result).destroy(ctx)
            await ((await promise3).result).destroy(ctx)
            await template.destroy(ctx)

        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.run_echo_test()
        self.run_mining_test()
        self.run_deprecated_mining_test()
        self.run_unclean_disconnect_test()
        self.run_thread_busy_test()

if __name__ == '__main__':
    IPCInterfaceTest(__file__).main()
