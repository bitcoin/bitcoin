#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Mining interface."""
import asyncio
import time
from contextlib import AsyncExitStack
from io import BytesIO
import re
from test_framework.blocktools import (
    NULL_OUTPOINT,
    WITNESS_COMMITMENT_HEADER,
)
from test_framework.messages import (
    MAX_BLOCK_WEIGHT,
    CBlockHeader,
    CTransaction,
    CTxIn,
    CTxOut,
    CTxInWitness,
    ser_uint256,
    COIN,
    from_hex,
    msg_headers,
)
from test_framework.script import (
    CScript,
    CScriptNum,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_not_equal
)
from test_framework.wallet import MiniWallet
from test_framework.p2p import P2PInterface
from test_framework.ipc_util import (
    destroying,
    mining_create_block_template,
    load_capnp_modules,
    make_capnp_init_ctx,
    mining_get_block,
    mining_get_coinbase_tx,
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
        for output_data in coinbase_res.requiredOutputs:
            output = CTxOut()
            output.deserialize(BytesIO(output_data))
            coinbase_tx.vout.append(output)

        coinbase_tx.nLockTime = coinbase_res.lockTime
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

            self.log.debug("interrupt() should abort waitTipChanged()")
            async def wait_for_tip():
                long_timeout = 60000.0  # 1 minute
                result = (await mining.waitTipChanged(ctx, newblockref.hash, long_timeout)).result
                # Unlike a timeout, interrupt() returns an empty BlockRef.
                assert_equal(len(result.hash), 0)
            await wait_and_do(wait_for_tip(), mining.interrupt())

        asyncio.run(capnp.run(async_routine()))

    def run_block_template_test(self):
        """Test BlockTemplate interface methods."""
        self.log.info("Running BlockTemplate interface test")
        block_header_size = 80
        timeout = 1000.0 # 1000 milliseconds

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            async with AsyncExitStack() as stack:
                self.log.debug("createNewBlock() should wait if tip is still updating")
                self.disconnect_nodes(0, 1)
                node1_block_hash = self.generate(self.nodes[1], 1, sync_fun=self.no_op)[0]
                header = from_hex(CBlockHeader(), self.nodes[1].getblockheader(node1_block_hash, False))
                header_only_peer = self.nodes[0].add_p2p_connection(P2PInterface())
                header_only_peer.send_and_ping(msg_headers([header]))
                start = time.time()
                async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx):
                    pass
                # Lower-bound only: a heavily loaded CI host might still exceed 0.9s
                # even without the cooldown, so this can miss regressions but avoids
                # spurious failures.
                assert_greater_than_or_equal(time.time() - start, 0.9)

                self.log.debug("interrupt() should abort createNewBlock() during cooldown")
                async def create_block():
                    result = await mining.createNewBlock(ctx, self.default_block_create_options)
                    # interrupt() causes createNewBlock to return nullptr
                    assert_equal(result._has("result"), False)

                await wait_and_do(create_block(), mining.interrupt())

                header_only_peer.peer_disconnect()
                self.connect_nodes(0, 1)
                self.sync_all()

                self.log.debug("Create a template")
                template = await mining_create_block_template(mining, stack, ctx, self.default_block_create_options)
                assert template is not None

                self.log.debug("Test some inspectors of Template")
                header = (await template.getBlockHeader(ctx)).result
                assert_equal(len(header), block_header_size)
                block = await mining_get_block(template, ctx)
                current_tip = self.nodes[0].getbestblockhash()
                assert_equal(ser_uint256(block.hashPrevBlock), ser_uint256(int(current_tip, 16)))
                assert_greater_than_or_equal(len(block.vtx), 1)
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
                assert template2 is not None
                block2 = await mining_get_block(template2, ctx)
                assert_equal(len(block2.vtx), 1)

                self.log.debug("Wait for another, but time out")
                template3 = await mining_wait_next_template(template2, stack, ctx, waitoptions)
                assert template3 is None

                self.log.debug("Wait for another, get one after increase in fees in the mempool")
                template4 = await wait_and_do(
                    mining_wait_next_template(template2, stack, ctx, waitoptions),
                    lambda: self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0]))
                assert template4 is not None
                block3 = await mining_get_block(template4, ctx)
                assert_equal(len(block3.vtx), 2)

                self.log.debug("Wait again, this should return the same template, since the fee threshold is zero")
                waitoptions.feeThreshold = 0
                template5 = await mining_wait_next_template(template4, stack, ctx, waitoptions)
                assert template5 is not None
                block4 = await mining_get_block(template5, ctx)
                assert_equal(len(block4.vtx), 2)
                waitoptions.feeThreshold = 1

                self.log.debug("Wait for another, get one after increase in fees in the mempool")
                template6 = await wait_and_do(
                    mining_wait_next_template(template5, stack, ctx, waitoptions),
                    lambda: self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0]))
                assert template6 is not None
                block4 = await mining_get_block(template6, ctx)
                assert_equal(len(block4.vtx), 3)

                self.log.debug("Wait for another, but time out, since the fee threshold is set now")
                template7 = await mining_wait_next_template(template6, stack, ctx, waitoptions)
                assert template7 is None

                self.log.debug("interruptWait should abort the current wait")
                async def wait_for_block():
                    new_waitoptions = self.capnp_modules['mining'].BlockWaitOptions()
                    new_waitoptions.timeout = timeout * 60 # 1 minute wait
                    new_waitoptions.feeThreshold = 1
                    template7 = await mining_wait_next_template(template6, stack, ctx, new_waitoptions)
                    assert template7 is None
                await wait_and_do(wait_for_block(), template6.interruptWait())

        asyncio.run(capnp.run(async_routine()))

    def run_ipc_option_override_test(self):
        self.log.info("Running IPC option override test")
        # Set an absurd reserved weight. `-blockreservedweight` is RPC-only, so
        # with this setting RPC templates would be empty. IPC clients set
        # blockReservedWeight per template request and are unaffected; later in
        # the test the IPC template includes a mempool transaction.
        self.restart_node(0, extra_args=[f"-blockreservedweight={MAX_BLOCK_WEIGHT}"])

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()
            self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0])

            async with AsyncExitStack() as stack:
                opts = self.capnp_modules['mining'].BlockCreateOptions()
                template = await mining_create_block_template(mining, stack, ctx, opts)
                assert template is not None
                block = await mining_get_block(template, ctx)
                assert_equal(len(block.vtx), 2)

                self.log.debug("Use absurdly large reserved weight to force an empty template")
                opts.blockReservedWeight = MAX_BLOCK_WEIGHT
                empty_template = await mining_create_block_template(mining, stack, ctx, opts)
                assert empty_template is not None
                empty_block = await mining_get_block(empty_template, ctx)
                assert_equal(len(empty_block.vtx), 1)

            self.log.debug("Enforce minimum reserved weight for IPC clients too")
            opts.blockReservedWeight = 0
            try:
                await mining.createNewBlock(ctx, opts)
                raise AssertionError("createNewBlock unexpectedly succeeded")
            except capnp.lib.capnp.KjException as e:
                if e.type == "DISCONNECTED":
                    # The remote exception isn't caught currently and leads to a
                    # std::terminate call. Just detect and restart in this case.
                    # This bug is fixed with
                    # https://github.com/bitcoin-core/libmultiprocess/pull/218
                    assert_equal(e.description, "Peer disconnected.")
                    self.nodes[0].wait_until_stopped(expected_ret_code=(-11, -6, 1, 66), expected_stderr=re.compile(""))
                    self.start_node(0)
                else:
                    assert_equal(e.description, "remote exception: std::exception: block_reserved_weight (0) must be at least 2000 weight units")
                    assert_equal(e.type, "FAILED")

        asyncio.run(capnp.run(async_routine()))

    def run_coinbase_and_submission_test(self):
        """Test coinbase construction (getCoinbaseTx) and block submission (submitSolution)."""
        self.log.info("Running coinbase construction and submission test")

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            check_opts = self.capnp_modules['mining'].BlockCheckOptions()

            async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx) as template:
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
                check = await mining.checkBlock(ctx, block.serialize(), check_opts)
                assert_equal(check.result, False)
                assert_equal(check.reason, "bad-version(0x00000000)")
                result = await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())
                assert_equal(result.result, False)
                assert_equal(result.reason, "bad-version(0x00000000)")
                self.log.debug("Submit a valid block")
                block.nVersion = original_version
                block.solve()

                self.log.debug("First call checkBlock()")
                block_valid = (await mining.checkBlock(ctx, block.serialize(), check_opts)).result
                assert_equal(block_valid, True)

                # The remote template block will be mutated, capture the original:
                remote_block_before = await mining_get_block(template, ctx)

                self.log.debug("Submitted coinbase must include witness")
                assert_not_equal(coinbase.serialize_without_witness().hex(), coinbase.serialize().hex())
                result = await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize_without_witness())
                assert_equal(result.result, False)
                assert_equal(result.reason, "bad-witness-nonce-size")

                self.log.debug("Even a rejected submitSolution() mutates the template's block")
                # Can be used by clients to download and inspect the (rejected)
                # reconstructed block.
                remote_block_after = await mining_get_block(template, ctx)
                assert_not_equal(remote_block_before.serialize().hex(), remote_block_after.serialize().hex())

                self.log.debug("Submit again, with the witness")
                result = await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())
                assert_equal(result.result, True)
                assert_equal(result.reason, "")

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
            check = await mining.checkBlock(ctx, block.serialize(), check_opts)
            assert_equal(check.result, False)
            assert_equal(check.reason, "inconclusive-not-best-prevblk")

        asyncio.run(capnp.run(async_routine()))

    def run_submit_block_test(self):
        """Test submitBlock() on the Mining interface (no BlockTemplate required for submission)."""
        self.log.info("Running submitBlock test")

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            current_block_height = self.nodes[0].getchaintips()[0]["height"]

            # Send a real transaction so the template includes it
            self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0])

            # Use a template to bootstrap a valid block, then submit the
            # serialized complete block via Mining.submitBlock instead of
            # BlockTemplate.submitSolution. This exercises the complete-block
            # submission path and approximates the Sv2 JDS flow.
            async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx) as template:
                block = await mining_get_block(template, ctx)
                assert len(block.vtx) >= 2, "Block should include at least the coinbase and the mempool tx"
                balance = self.miniwallet.get_balance()
                coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet)
                coinbase.vout[0].nValue = COIN
                block.vtx[0] = coinbase
                block.hashMerkleRoot = block.calc_merkle_root()

                original_version = block.nVersion

                self.log.debug("submitBlock with bad version should fail")
                block.nVersion = 0
                block.solve()
                result = await mining.submitBlock(ctx, block.serialize())
                assert_equal(result.result, False)
                assert_equal(result.reason, "bad-version(0x00000000)")

                self.log.debug("submitBlock with valid block should succeed")
                block.nVersion = original_version
                block.solve()
                result = await mining.submitBlock(ctx, block.serialize())
                assert_equal(result.result, True)
                assert_equal(result.reason, "")

            self.log.debug("Block should propagate")
            assert_equal(self.nodes[0].getchaintips()[0]["height"], current_block_height + 1)
            self.sync_all()
            assert_equal(self.nodes[0].getchaintips()[0], self.nodes[1].getchaintips()[0])

            self.miniwallet.rescan_utxos()
            assert_equal(self.miniwallet.get_balance(), balance + 1)

            self.log.debug("submitBlock duplicate should fail")
            result = await mining.submitBlock(ctx, block.serialize())
            assert_equal(result.result, False)
            assert_equal(result.reason, "duplicate")

        asyncio.run(capnp.run(async_routine()))

    def run_submit_block_witness_test(self):
        """Test that submitBlock does NOT auto-add coinbase witness nonce."""
        self.log.info("Running submitBlock witness encoding test")

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx) as template:
                block = await mining_get_block(template, ctx)
                coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet)
                coinbase.vout[0].nValue = COIN

                # Strip the coinbase witness (nonce) but keep the OP_RETURN
                # witness commitment output. This simulates an invalid encoding
                # that the submitblock RPC would auto-fix via
                # UpdateUncommittedBlockStructures, but submitBlock should not.
                coinbase.wit.vtxinwit = []
                has_witness_commitment = any(
                    len(o.scriptPubKey) >= 38 and o.scriptPubKey[2:6] == WITNESS_COMMITMENT_HEADER
                    for o in coinbase.vout
                )
                assert has_witness_commitment, "Coinbase should have a witness commitment output"

                block.vtx[0] = coinbase
                block.hashMerkleRoot = block.calc_merkle_root()
                block.solve()

                self.log.debug("submitBlock with witness commitment but missing coinbase witness nonce should fail")
                result = await mining.submitBlock(ctx, block.serialize())
                assert_equal(result.result, False)
                assert_equal(result.reason, "bad-witness-nonce-size")

        asyncio.run(capnp.run(async_routine()))

    def run_submit_block_then_solution_test(self):
        """Test that submitSolution returns duplicate after submitBlock succeeds."""
        self.log.info("Running submitBlock then submitSolution interaction test")
        # Mine a block to ensure the chain is synced and stable after previous tests
        self.generate(self.nodes[0], 1)

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            current_block_height = self.nodes[0].getchaintips()[0]["height"]

            async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx) as template:
                block = await mining_get_block(template, ctx)
                coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet)
                coinbase.vout[0].nValue = COIN
                block.vtx[0] = coinbase
                block.hashMerkleRoot = block.calc_merkle_root()
                block.solve()

                self.log.debug("Submit via submitBlock first")
                result = await mining.submitBlock(ctx, block.serialize())
                assert_equal(result.result, True)

                self.nodes[0].waitforblockheight(current_block_height + 1)

                self.log.debug("submitSolution on same template should return duplicate")
                result = await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())
                assert_equal(result.result, False)
                assert_equal(result.reason, "duplicate")

            self.sync_all()

        asyncio.run(capnp.run(async_routine()))

    def run_submit_solution_then_block_test(self):
        """Test that submitBlock returns duplicate after submitSolution succeeds."""
        self.log.info("Running submitSolution then submitBlock interaction test")
        # Mine a block to ensure the chain is synced and stable after previous tests
        self.generate(self.nodes[0], 1)

        async def async_routine():
            ctx, mining = await self.make_mining_ctx()

            current_block_height = self.nodes[0].getchaintips()[0]["height"]

            async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx) as template:
                block = await mining_get_block(template, ctx)
                coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet)
                coinbase.vout[0].nValue = COIN
                block.vtx[0] = coinbase
                block.hashMerkleRoot = block.calc_merkle_root()
                block.solve()

                self.log.debug("Submit via submitSolution first")
                result = await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())
                assert_equal(result.result, True)
                assert_equal(result.reason, "")

                self.nodes[0].waitforblockheight(current_block_height + 1)

                self.log.debug("submitBlock with same block should fail with duplicate")
                result = await mining.submitBlock(ctx, block.serialize())
                assert_equal(result.result, False)
                assert_equal(result.reason, "duplicate")

            self.sync_all()

        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.miniwallet = MiniWallet(self.nodes[0])
        self.default_block_create_options = self.capnp_modules['mining'].BlockCreateOptions()
        self.run_mining_interface_test()
        self.run_block_template_test()
        self.run_coinbase_and_submission_test()
        self.run_submit_block_test()
        self.run_submit_block_witness_test()
        self.run_submit_block_then_solution_test()
        self.run_submit_solution_then_block_test()
        self.run_ipc_option_override_test()


if __name__ == '__main__':
    IPCMiningTest(__file__).main()
