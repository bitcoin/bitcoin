#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) interface."""
import asyncio
import functools
from pathlib import Path
import tempfile
from test_framework.test_framework import BitcoinTestFramework

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ImportError:
    pass


class IPCInterfaceTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.use_multiprocess_node = True
        # The default socket path exceeds the maximum socket filename length in CI.
        self.socket_path = Path(tempfile.mkdtemp()) / "ipc.sock"
        self.extra_args = [[f"-ipcbind=unix:{self.socket_path}"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_py_capnp()
        self.skip_if_no_bitcoind_ipc()

    @functools.cache
    def capnp_modules(self):
        src_dir = Path(self.config['environment']['SRCDIR']) / "src"
        mp_dir = src_dir / "ipc" / "libmultiprocess" / "include"
        imports = ["/usr/include", str(src_dir), str(mp_dir)]
        return {
            "proxy": capnp.load(str(mp_dir / "mp" / "proxy.capnp"), imports=imports),
            "init": capnp.load(str(src_dir / "ipc" / "capnp" / "init.capnp"), imports=imports),
            "echo": capnp.load(str(src_dir / "ipc" / "capnp" / "echo.capnp"), imports=imports),
            "mining": capnp.load(str(src_dir / "ipc" / "capnp" / "mining.capnp"), imports=imports),
        }

    async def make_capnp_init_ctx(self):
        modules = self.capnp_modules()
        connection = await capnp.AsyncIoStream.create_unix_connection(self.socket_path)
        client = capnp.TwoPartyClient(connection)
        init = client.bootstrap().cast_as(modules['init'].Init)
        threadmap = init.construct().threadMap
        thread = threadmap.makeThread("pythread").result
        ctx = modules['proxy'].Context()
        ctx.thread = thread
        return init, ctx

    def run_echo_test(self):
        async def async_routine():
            init, ctx = await self.make_capnp_init_ctx()
            echo = init.makeEcho(ctx).result
            for s in ["hallo", "", "haha"]:
                result_eval = (await echo.echo(ctx, s)).result
                assert s == result_eval
            echo.destroy(ctx)
        asyncio.run(capnp.run(async_routine()))

    def run_mining_test(self):
        modules = self.capnp_modules()
        async def async_routine():
            init, ctx = await self.make_capnp_init_ctx()
            mining = init.makeMining(ctx)
            opts = modules['mining'].BlockCreateOptions()
            opts.useMempool = True
            opts.blockReservedWeight = 4000
            opts.coinbaseOutputMaxAdditionalSigops = 0
            template = mining.result.createNewBlock(opts)
            header = template.result.getBlockHeader(ctx)
            header_eval = (await header).result
            assert len(header_eval) == 80
            template.result.destroy(ctx)
        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.run_echo_test()
        self.run_mining_test()

if __name__ == '__main__':
    IPCInterfaceTest(__file__).main()
