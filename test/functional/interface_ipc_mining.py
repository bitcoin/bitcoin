#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Mining interface."""
import asyncio
from contextlib import AsyncExitStack
from io import BytesIO
from test_framework.blocktools import NULL_OUTPOINT
from test_framework.messages import (
    CTransaction,
    CTxIn,
    CTxOut,
    CTxInWitness,
    ser_uint256,
    COIN,
)
from test_framework.script import (
    CScript,
    CScriptNum,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal
)
from test_framework.wallet import MiniWallet
from test_framework.ipc_util import (
    destroying,
    mining_create_block_template,
    load_capnp_modules,
    make_capnp_init_ctx,
    mining_get_block,
    mining_get_coinbase_tx,
    mining_get_coinbase_raw_tx,
    mining_wait_next_template,
    wait_and_do,
)

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass


class IPCMiningTest(BitcoinTestFramework):

    def skip_test_if_missing_module(self):
        self.skip_if_no_ipc()
        self.skip_if_no_py_capnp()

    def set_test_params(self):
        self.num_nodes = 2

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}, {}]
        super().setup_nodes()
        # Use this function to also load the capnp modules (we cannot use set_test_params for this,
        # as it is being called before knowing whether capnp is available).
        self.capnp_modules = load_capnp_modules(self.config)

    async def build_coinbase_test(self, template, ctx, miniwallet):
        self.log.debug("Build coinbase transaction using getCoinbaseTx()")
        assert template is not None
        coinbase_res = await mining_get_coinbase_tx(template, ctx)
        coinbase_tx = CTransaction()
        coinbase_tx.version = coinbase_res.version
        coinbase_tx.vin = [CTxIn()]
        coinbase_tx.vin[0].prevout = NULL_OUTPOINT
        coinbase_tx.vin[0].nSequence = coinbase_res.sequence

        # Verify there's no dummy extraNonce in the coinbase scriptSig
        current_block_height = self.nodes[0].getchaintips()[0]["height"]
        expected_scriptsig = CScript([CScriptNum(current_block_height + 1)])
        assert_equal(coinbase_res.scriptSigPrefix.hex(), expected_scriptsig.hex())

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
        # Compare to dummy coinbase transaction provided by the deprecated
        # getCoinbaseRawTx()
        coinbase_legacy = await mining_get_coinbase_raw_tx(template, ctx)
        assert_equal(coinbase_legacy.vout[0].nValue, coinbase_res.blockRewardRemaining)
        # Swap dummy output for our own
        coinbase_legacy.vout[0].scriptPubKey = coinbase_tx.vout[0].scriptPubKey
        assert_equal(coinbase_tx.serialize().hex(), coinbase_legacy.serialize().hex())

        return coinbase_tx

    async def make_mining_ctx(self):
        """Create IPC context and Mining proxy object."""
        ctx, init = await make_capnp_init_ctx(self)
        self.log.debug("Create Mining proxy object")
        mining = init.makeMining(ctx).result
        return ctx, mining

    def run_mining_interface_test(self):
        """Test Mining interface methods."""
        self.log.info("Running Mining interface test")
        block_hash_size = 32
        timeout = 1000.0 # 1000 milliseconds

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()
            blockref = await mining.getTip(ctx)
            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            assert_equal(blockref.result.height, current_block_height)

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

        asyncio.run(capnp.run(async_routine()))

    def run_block_template_test(self):
        """Test BlockTemplate interface methods."""
        self.log.info("Running BlockTemplate interface test")
        block_header_size = 80
        timeout = 1000.0 # 1000 milliseconds

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()
            blockref = await mining.getTip(ctx)

            async with AsyncExitStack() as stack:
                self.log.debug("Create a template")
                template = await mining_create_block_template(mining, stack, ctx, self.default_block_create_options)

                self.log.debug("Test some inspectors of Template")
                header = (await template.getBlockHeader(ctx)).result
                assert_equal(len(header), block_header_size)
                block = await mining_get_block(template, ctx)
                assert_equal(ser_uint256(block.hashPrevBlock), blockref.result.hash)
                assert len(block.vtx) >= 1
                txfees = await template.getTxFees(ctx)
                assert_equal(len(txfees.result), 0)
                txsigops = await template.getTxSigops(ctx)
                assert_equal(len(txsigops.result), 0)

                self.log.debug("Wait for a new template")
                waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
                waitoptions.timeout = timeout
                waitoptions.feeThreshold = 1
                template2 = await wait_and_do(
                    mining_wait_next_template(template, stack, ctx, waitoptions),
                    lambda: self.generate(self.nodes[0], 1))
                block2 = await mining_get_block(template2, ctx)
                assert_equal(len(block2.vtx), 1)

                self.log.debug("Wait for another, but time out")
                template3 = await template2.waitNext(ctx, waitoptions)
                assert_equal(template3._has("result"), False)

                self.log.debug("Wait for another, get one after increase in fees in the mempool")
                template4 = await wait_and_do(
                    mining_wait_next_template(template2, stack, ctx, waitoptions),
                    lambda: self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0]))
                block3 = await mining_get_block(template4, ctx)
                assert_equal(len(block3.vtx), 2)

                self.log.debug("Wait again, this should return the same template, since the fee threshold is zero")
                waitoptions.feeThreshold = 0
                template5 = await mining_wait_next_template(template4, stack, ctx, waitoptions)
                block4 = await mining_get_block(template5, ctx)
                assert_equal(len(block4.vtx), 2)
                waitoptions.feeThreshold = 1

                self.log.debug("Wait for another, get one after increase in fees in the mempool")
                template6 = await wait_and_do(
                    mining_wait_next_template(template5, stack, ctx, waitoptions),
                    lambda: self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0]))
                block4 = await mining_get_block(template6, ctx)
                assert_equal(len(block4.vtx), 3)

                self.log.debug("Wait for another, but time out, since the fee threshold is set now")
                template7 = await template6.waitNext(ctx, waitoptions)
                assert_equal(template7._has("result"), False)

                self.log.debug("interruptWait should abort the current wait")
                async def wait_for_block():
                    new_waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
                    new_waitoptions.timeout = timeout * 60 # 1 minute wait
                    new_waitoptions.feeThreshold = 1
                    template7 = await template6.waitNext(ctx, new_waitoptions)
                    assert_equal(template7._has("result"), False)
                await wait_and_do(wait_for_block(), template6.interruptWait())

        asyncio.run(capnp.run(async_routine()))

    def run_coinbase_and_submission_test(self):
        """Test coinbase construction (getCoinbaseTx, getCoinbaseCommitment) and block submission (submitSolution)."""
        self.log.info("Running coinbase construction and submission test")

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            check_opts = self.capnp_modules['mining'].BlockCheckOptions()

            async with destroying((await mining.createNewBlock(self.default_block_create_options)).result, ctx) as template:
                block = await mining_get_block(template, ctx)
                balance = self.miniwallet.get_balance()
                coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet)
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
                remote_block_before = await mining_get_block(template, ctx)

                self.log.debug("Submitted coinbase must include witness")
                assert_not_equal(coinbase.serialize_without_witness().hex(), coinbase.serialize().hex())
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize_without_witness())).result
                assert_equal(submitted, False)

                self.log.debug("Even a rejected submitSolution() mutates the template's block")
                # Can be used by clients to download and inspect the (rejected)
                # reconstructed block.
                remote_block_after = await mining_get_block(template, ctx)
                assert_not_equal(remote_block_before.serialize().hex(), remote_block_after.serialize().hex())

                self.log.debug("Submit again, with the witness")
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())).result
                assert_equal(submitted, True)

            self.log.debug("Block should propagate")
            # Check that the IPC node actually updates its own chain
            assert_equal(self.nodes[0].getchaintips()[0]["height"], current_block_height + 1)
            # Stalls if a regression causes submitSolution() to accept an invalid block:
            self.sync_all()
            # Check that the other node accepts the block
            assert_equal(self.nodes[0].getchaintips()[0], self.nodes[1].getchaintips()[0])

            self.miniwallet.rescan_utxos()
            assert_equal(self.miniwallet.get_balance(), balance + 1)
            self.log.debug("Check block should fail now, since it is a duplicate")
            check = await mining.checkBlock(block.serialize(), check_opts)
            assert_equal(check.result, False)
            assert_equal(check.reason, "inconclusive-not-best-prevblk")

        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.miniwallet = MiniWallet(self.nodes[0])
        self.default_block_create_options = self.capnp_modules['mining'].BlockCreateOptions(
            useMempool=True,
            blockReservedWeight=4000,
            coinbaseOutputMaxAdditionalSigops=0
        )

        self.run_mining_interface_test()
        self.run_block_template_test()
        self.run_coinbase_and_submission_test()


if __name__ == '__main__':
    IPCMiningTest(__file__).main()
