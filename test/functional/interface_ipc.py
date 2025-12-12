#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) interface."""
import asyncio
import inspect
from contextlib import asynccontextmanager, AsyncExitStack
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import shutil
from test_framework.blocktools import NULL_OUTPOINT
from test_framework.messages import (
    CBlock,
    CTransaction,
    CTxIn,
    CTxOut,
    CTxInWitness,
    ser_uint256,
    COIN,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal
)
from test_framework.wallet import MiniWallet
from typing import Optional

# Stores the result of getCoinbaseTx()
@dataclass
class CoinbaseTxData:
    version: int
    sequence: int
    scriptSigPrefix: bytes
    witness: Optional[bytes]
    blockRewardRemaining: int
    requiredOutputs: list[bytes]
    lockTime: int

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass

@asynccontextmanager
async def destroying(obj, ctx):
    """Call obj.destroy(ctx) at end of with: block. Similar to contextlib.closing."""
    try:
        yield obj
    finally:
        await obj.destroy(ctx)

async def create_block_template(mining, stack, ctx, opts):
    """Call mining.createNewBlock() and return template, then call template.destroy() when stack exits."""
    return await stack.enter_async_context(destroying((await mining.createNewBlock(opts)).result, ctx))

async def wait_next_template(template, stack, ctx, opts):
    """Call template.waitNext() and return template, then call template.destroy() when stack exits."""
    return await stack.enter_async_context(destroying((await template.waitNext(ctx, opts)).result, ctx))

async def wait_and_do(wait_fn, do_fn):
    """Call wait_fn, then sleep, then call do_fn in a parallel task. Wait for
    both tasks to complete."""
    wait_started = asyncio.Event()
    result = None

    async def wait():
        nonlocal result
        wait_started.set()
        result = await wait_fn

    async def do():
        await wait_started.wait()
        await asyncio.sleep(0.1)
        # Let do_fn be either a callable or an awaitable object
        if inspect.isawaitable(do_fn):
            await do_fn
        else:
            do_fn()

    await asyncio.gather(wait(), do())
    return result

class IPCInterfaceTest(BitcoinTestFramework):

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()
        self.skip_if_no_py_capnp()

    def load_capnp_modules(self):
        if capnp_bin := shutil.which("capnp"):
            # Add the system cap'nproto path so include/capnp/c++.capnp can be found.
            capnp_dir = Path(capnp_bin).resolve().parent.parent / "include"
        else:
            # If there is no system cap'nproto, the pycapnp module should have its own "bundled"
            # includes at this location. If pycapnp was installed with bundled capnp,
            # capnp/c++.capnp can be found here.
            capnp_dir = Path(capnp.__path__[0]).parent
        src_dir = Path(self.config['environment']['SRCDIR']) / "src"
        mp_dir = src_dir / "ipc" / "libmultiprocess" / "include"
        # List of import directories. Note: it is important for mp_dir to be
        # listed first, in case there are other libmultiprocess installations on
        # the system, to ensure that `import "/mp/proxy.capnp"` lines load the
        # same file as capnp.load() loads directly below, and there are not
        # "failed: Duplicate ID @0xcc316e3f71a040fb" errors.
        imports = [str(mp_dir), str(capnp_dir), str(src_dir)]
        return {
            "proxy": capnp.load(str(mp_dir / "mp" / "proxy.capnp"), imports=imports),
            "init": capnp.load(str(src_dir / "ipc" / "capnp" / "init.capnp"), imports=imports),
            "echo": capnp.load(str(src_dir / "ipc" / "capnp" / "echo.capnp"), imports=imports),
            "mining": capnp.load(str(src_dir / "ipc" / "capnp" / "mining.capnp"), imports=imports),
        }

    def set_test_params(self):
        self.num_nodes = 2

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}, {}]
        super().setup_nodes()
        # Use this function to also load the capnp modules (we cannot use set_test_params for this,
        # as it is being called before knowing whether capnp is available).
        self.capnp_modules = self.load_capnp_modules()

    async def make_capnp_init_ctx(self):
        node = self.nodes[0]
        # Establish a connection, and create Init proxy object.
        connection = await capnp.AsyncIoStream.create_unix_connection(node.ipc_socket_path)
        client = capnp.TwoPartyClient(connection)
        init = client.bootstrap().cast_as(self.capnp_modules['init'].Init)
        # Create a remote thread on the server for the IPC calls to be executed in.
        threadmap = init.construct().threadMap
        thread = threadmap.makeThread("pythread").result
        ctx = self.capnp_modules['proxy'].Context()
        ctx.thread = thread
        # Return both.
        return ctx, init

    async def parse_and_deserialize_block(self, block_template, ctx):
        block_data = BytesIO((await block_template.getBlock(ctx)).result)
        block = CBlock()
        block.deserialize(block_data)
        return block

    async def parse_and_deserialize_coinbase_tx(self, block_template, ctx):
        assert block_template is not None
        coinbase_data = BytesIO((await block_template.getCoinbaseRawTx(ctx)).result)
        tx = CTransaction()
        tx.deserialize(coinbase_data)
        return tx

    async def parse_and_deserialize_coinbase(self, block_template, ctx) -> CoinbaseTxData:
        assert block_template is not None
        # Note: the template_capnp struct will be garbage-collected when this
        # method returns, so it is important to copy any Data fields from it
        # which need to be accessed later using the bytes() cast. Starting with
        # pycapnp v2.2.0, Data fields have type `memoryview` and are ephemeral.
        template_capnp = (await block_template.getCoinbaseTx(ctx)).result
        witness: Optional[bytes] = None
        if template_capnp._has("witness"):
            witness = bytes(template_capnp.witness)
        return CoinbaseTxData(
            version=int(template_capnp.version),
            sequence=int(template_capnp.sequence),
            scriptSigPrefix=bytes(template_capnp.scriptSigPrefix),
            witness=witness,
            blockRewardRemaining=int(template_capnp.blockRewardRemaining),
            requiredOutputs=[bytes(output) for output in template_capnp.requiredOutputs],
            lockTime=int(template_capnp.lockTime),
        )

    def run_echo_test(self):
        self.log.info("Running echo test")
        async def async_routine():
            ctx, init = await self.make_capnp_init_ctx()
            self.log.debug("Create Echo proxy object")
            echo = init.makeEcho(ctx).result
            self.log.debug("Test a few invocations of echo")
            for s in ["hallo", "", "haha"]:
                result_eval = (await echo.echo(ctx, s)).result
                assert_equal(s, result_eval)
            self.log.debug("Destroy the Echo object")
            echo.destroy(ctx)
        asyncio.run(capnp.run(async_routine()))

    async def build_coinbase_test(self, template, ctx, miniwallet):
        self.log.debug("Build coinbase transaction using getCoinbaseTx()")
        assert template is not None
        coinbase_res = await self.parse_and_deserialize_coinbase(template, ctx)
        coinbase_tx = CTransaction()
        coinbase_tx.version = coinbase_res.version
        coinbase_tx.vin = [CTxIn()]
        coinbase_tx.vin[0].prevout = NULL_OUTPOINT
        coinbase_tx.vin[0].nSequence = coinbase_res.sequence
        # Typically a mining pool appends its name and an extraNonce
        coinbase_tx.vin[0].scriptSig = coinbase_res.scriptSigPrefix

        # We currently always provide a coinbase witness, even for empty
        # blocks, but this may change, so always check:
        has_witness = coinbase_res.witness is not None
        if has_witness:
            coinbase_tx.wit.vtxinwit = [CTxInWitness()]
            coinbase_tx.wit.vtxinwit[0].scriptWitness.stack = [coinbase_res.witness]

        # First output is our payout
        coinbase_tx.vout = [CTxOut()]
        coinbase_tx.vout[0].scriptPubKey = miniwallet.get_output_script()
        coinbase_tx.vout[0].nValue = coinbase_res.blockRewardRemaining
        # Add SegWit OP_RETURN. This is currently always present even for
        # empty blocks, but this may change.
        found_witness_op_return = False
        # Compare SegWit OP_RETURN to getCoinbaseCommitment()
        coinbase_commitment = (await template.getCoinbaseCommitment(ctx)).result
        for output_data in coinbase_res.requiredOutputs:
            output = CTxOut()
            output.deserialize(BytesIO(output_data))
            coinbase_tx.vout.append(output)
            if output.scriptPubKey == coinbase_commitment:
                found_witness_op_return = True

        assert_equal(has_witness, found_witness_op_return)

        coinbase_tx.nLockTime = coinbase_res.lockTime

        # Compare to dummy coinbase provided by the deprecated getCoinbaseTx()
        coinbase_legacy = await self.parse_and_deserialize_coinbase_tx(template, ctx)
        assert_equal(coinbase_legacy.vout[0].nValue, coinbase_res.blockRewardRemaining)
        # Swap dummy output for our own
        coinbase_legacy.vout[0].scriptPubKey = coinbase_tx.vout[0].scriptPubKey
        assert_equal(coinbase_tx.serialize().hex(), coinbase_legacy.serialize().hex())

        return coinbase_tx

    def run_mining_test(self):
        self.log.info("Running mining test")
        block_hash_size = 32
        block_header_size = 80
        timeout = 1000.0 # 1000 milliseconds
        miniwallet = MiniWallet(self.nodes[0])

        async def async_routine():
            ctx, init = await self.make_capnp_init_ctx()
            self.log.debug("Create Mining proxy object")
            mining = init.makeMining(ctx).result
            self.log.debug("Test simple inspectors")
            assert (await mining.isTestChain(ctx)).result
            assert not (await mining.isInitialBlockDownload(ctx)).result
            blockref = await mining.getTip(ctx)
            assert blockref.hasResult
            assert_equal(len(blockref.result.hash), block_hash_size)
            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            assert blockref.result.height == current_block_height
            self.log.debug("Mine a block")
            newblockref = (await wait_and_do(
                mining.waitTipChanged(ctx, blockref.result.hash, timeout),
                lambda: self.generate(self.nodes[0], 1))).result
            assert_equal(len(newblockref.hash), block_hash_size)
            assert_equal(newblockref.height, current_block_height + 1)
            self.log.debug("Wait for timeout")
            oldblockref = (await mining.waitTipChanged(ctx, newblockref.hash, timeout)).result
            assert_equal(len(newblockref.hash), block_hash_size)
            assert_equal(oldblockref.hash, newblockref.hash)
            assert_equal(oldblockref.height, newblockref.height)

            async with AsyncExitStack() as stack:
                self.log.debug("Create a template")
                opts = self.capnp_modules['mining'].BlockCreateOptions()
                opts.useMempool = True
                opts.blockReservedWeight = 4000
                opts.coinbaseOutputMaxAdditionalSigops = 0
                template = await create_block_template(mining, stack, ctx, opts)

                self.log.debug("Test some inspectors of Template")
                header = (await template.getBlockHeader(ctx)).result
                assert_equal(len(header), block_header_size)
                block = await self.parse_and_deserialize_block(template, ctx)
                assert_equal(ser_uint256(block.hashPrevBlock), newblockref.hash)
                assert len(block.vtx) >= 1
                txfees = await template.getTxFees(ctx)
                assert_equal(len(txfees.result), 0)
                txsigops = await template.getTxSigops(ctx)
                assert_equal(len(txsigops.result), 0)
                coinbase_data = BytesIO((await template.getCoinbaseRawTx(ctx)).result)
                coinbase = CTransaction()
                coinbase.deserialize(coinbase_data)
                assert_equal(coinbase.vin[0].prevout.hash, 0)

                self.log.debug("Wait for a new template")
                waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
                waitoptions.timeout = timeout
                waitoptions.feeThreshold = 1
                template2 = await wait_and_do(
                    wait_next_template(template, stack, ctx, waitoptions),
                    lambda: self.generate(self.nodes[0], 1))
                block2 = await self.parse_and_deserialize_block(template2, ctx)
                assert_equal(len(block2.vtx), 1)

                self.log.debug("Wait for another, but time out")
                template3 = await template2.waitNext(ctx, waitoptions)
                assert_equal(template3._has("result"), False)

                self.log.debug("Wait for another, get one after increase in fees in the mempool")
                template4 = await wait_and_do(
                    wait_next_template(template2, stack, ctx, waitoptions),
                    lambda: miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0]))
                block3 = await self.parse_and_deserialize_block(template4, ctx)
                assert_equal(len(block3.vtx), 2)

                self.log.debug("Wait again, this should return the same template, since the fee threshold is zero")
                waitoptions.feeThreshold = 0
                template5 = await wait_next_template(template4, stack, ctx, waitoptions)
                block4 = await self.parse_and_deserialize_block(template5, ctx)
                assert_equal(len(block4.vtx), 2)
                waitoptions.feeThreshold = 1

                self.log.debug("Wait for another, get one after increase in fees in the mempool")
                template6 = await wait_and_do(
                    wait_next_template(template5, stack, ctx, waitoptions),
                    lambda: miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0]))
                block4 = await self.parse_and_deserialize_block(template6, ctx)
                assert_equal(len(block4.vtx), 3)

                self.log.debug("Wait for another, but time out, since the fee threshold is set now")
                template7 = await template6.waitNext(ctx, waitoptions)
                assert_equal(template7._has("result"), False)

                self.log.debug("interruptWait should abort the current wait")
                async def wait_for_block():
                    new_waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
                    new_waitoptions.timeout = waitoptions.timeout * 60 # 1 minute wait
                    new_waitoptions.feeThreshold = 1
                    template7 = await template6.waitNext(ctx, new_waitoptions)
                    assert_equal(template7._has("result"), False)
                await wait_and_do(wait_for_block(), template6.interruptWait())

            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            check_opts = self.capnp_modules['mining'].BlockCheckOptions()
            async with destroying((await mining.createNewBlock(opts)).result, ctx) as template:
                block = await self.parse_and_deserialize_block(template, ctx)
                balance = miniwallet.get_balance()
                coinbase = await self.build_coinbase_test(template, ctx, miniwallet)
                # Reduce payout for balance comparison simplicity
                coinbase.vout[0].nValue = COIN
                block.vtx[0] = coinbase
                block.hashMerkleRoot = block.calc_merkle_root()
                original_version = block.nVersion
                self.log.debug("Submit a block with a bad version")
                block.nVersion = 0
                block.solve()
                check = await mining.checkBlock(block.serialize(), check_opts)
                assert_equal(check.result, False)
                assert_equal(check.reason, "bad-version(0x00000000)")
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())).result
                assert_equal(submitted, False)
                self.log.debug("Submit a valid block")
                block.nVersion = original_version
                block.solve()

                self.log.debug("First call checkBlock()")
                block_valid = (await mining.checkBlock(block.serialize(), check_opts)).result
                assert_equal(block_valid, True)

                # The remote template block will be mutated, capture the original:
                remote_block_before = await self.parse_and_deserialize_block(template, ctx)

                self.log.debug("Submitted coinbase must include witness")
                assert_not_equal(coinbase.serialize_without_witness().hex(), coinbase.serialize().hex())
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize_without_witness())).result
                assert_equal(submitted, False)

                self.log.debug("Even a rejected submitBlock() mutates the template's block")
                # Can be used by clients to download and inspect the (rejected)
                # reconstructed block.
                remote_block_after = await self.parse_and_deserialize_block(template, ctx)
                assert_not_equal(remote_block_before.serialize().hex(), remote_block_after.serialize().hex())

                self.log.debug("Submit again, with the witness")
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())).result
                assert_equal(submitted, True)

            self.log.debug("Block should propagate")
            # Check that the IPC node actually updates its own chain
            assert_equal(self.nodes[0].getchaintips()[0]["height"], current_block_height + 1)
            # Stalls if a regression causes submitBlock() to accept an invalid block:
            self.sync_all()
            # Check that the other node accepts the block
            assert_equal(self.nodes[0].getchaintips()[0], self.nodes[1].getchaintips()[0])

            miniwallet.rescan_utxos()
            assert_equal(miniwallet.get_balance(), balance + 1)
            self.log.debug("Check block should fail now, since it is a duplicate")
            check = await mining.checkBlock(block.serialize(), check_opts)
            assert_equal(check.result, False)
            assert_equal(check.reason, "inconclusive-not-best-prevblk")

        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.run_echo_test()
        self.run_mining_test()

if __name__ == '__main__':
    IPCInterfaceTest(__file__).main()
