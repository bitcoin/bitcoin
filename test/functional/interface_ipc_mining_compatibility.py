#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test IPC mining compatibility with previous releases.

Previous releases are required by this test, see test/README.md.
"""

import asyncio
from contextlib import AsyncExitStack
from pathlib import Path

from test_framework.ipc_util import (
    load_capnp_modules,
    make_mining_ctx,
    mining_create_block_template,
    mining_get_coinbase_tx,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_not_equal

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass


class IPCMiningCompatibilityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()
        self.skip_if_no_py_capnp()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        self.add_nodes(self.num_nodes, versions=[310000])

        # Previous-release test nodes default to running bitcoind directly, but
        # the IPC interface is served by the multiprocess node.
        previous_bin = Path(self.options.previous_releases_path) / "v31.0" / "bin"
        self.nodes[0].args = [
            str(previous_bin / "bitcoin"),
            "-m",
            "node",
            *self.nodes[0].args[1:],
        ]

        self.start_nodes()
        self.capnp_modules = load_capnp_modules(self.config)

    def run_test(self):
        self.log.info("Test current IPC mining client against a v31.0 node")

        async def async_routine():
            async with AsyncExitStack() as stack:
                ctx, mining = await make_mining_ctx(self)

                self.log.debug("Default block creation options work with v31.0")
                opts = self.capnp_modules["mining"].BlockCreateOptions()
                template = await mining_create_block_template(mining, stack, ctx, opts, cooldown=False)
                coinbase = await mining_get_coinbase_tx(template, ctx)
                assert_not_equal(coinbase.witness, None)
                assert_equal(len(coinbase.requiredOutputs), 1)

        asyncio.run(capnp.run(async_routine()))


if __name__ == "__main__":
    IPCMiningCompatibilityTest(__file__).main()
