#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) Mining interface."""
import asyncio
import time
from contextlib import AsyncExitStack
from copy import deepcopy
from decimal import Decimal
from io import BytesIO
from test_framework.blocktools import (
    NULL_OUTPOINT,
    script_BIP34_coinbase_height,
    WITNESS_COMMITMENT_HEADER,
)
from test_framework.messages import (
    CBlockHeader,
    COIN,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    DEFAULT_BLOCK_RESERVED_WEIGHT,
    MAX_BLOCK_SIGOPS_COST,
    MAX_BLOCK_WEIGHT,
    from_hex,
    msg_headers,
    ser_uint256,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_not_equal,
)
from test_framework.wallet import MiniWallet
from test_framework.p2p import P2PInterface
from test_framework.ipc_util import (
    assert_capnp_failed,
    assert_create_new_block_fails,
    destroying,
    load_capnp_modules,
    make_mining_ctx,
    mining_create_block_template,
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
        self.num_nodes = 3

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}, {}, {"ipcbind": True}]
        super().setup_nodes()
        # Use this function to also load the capnp modules (we cannot use set_test_params for this,
        # as it is being called before knowing whether capnp is available).
        self.capnp_modules = load_capnp_modules(self.config)

    async def build_coinbase_test(self, template, ctx, miniwallet, extra_nonce=b""):
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
        bip34_prefix = script_BIP34_coinbase_height(current_block_height + 1, padding=False)
        assert_equal(coinbase_res.scriptSigPrefix, bip34_prefix)

        # Typically a mining pool appends its name and an extraNonce
        coinbase_tx.vin[0].scriptSig = coinbase_res.scriptSigPrefix + extra_nonce

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

    async def build_candidate_block(self, template, ctx):
        """Build a complete block from a remote BlockTemplate."""
        block = await mining_get_block(template, ctx)
        coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet)
        # Reduce payout for balance comparison simplicity.
        coinbase.vout[0].nValue = COIN
        block.vtx[0] = coinbase
        block.hashMerkleRoot = block.calc_merkle_root()
        return block

    async def assert_submit_block(self, mining, ctx, block, *, result, reason="", debug=""):
        submit = await mining.submitBlock(ctx, block.serialize())
        assert_equal(submit.result, result)
        assert_equal(submit.reason, reason)
        assert_equal(submit.debug, debug)

    def run_mining_interface_test(self):
        """Test Mining interface methods."""
        self.log.info("Running Mining interface test")
        block_hash_size = 32

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            blockref = await mining.getTip(ctx)
            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            assert_equal(blockref.result.height, current_block_height)

            self.log.debug("Mine a block")
            newblockref = (await wait_and_do(
                mining.waitTipChanged(ctx, blockref.result.hash, self.default_ipc_timeout),
                lambda: self.generate(self.nodes[0], 1))).result
            assert_equal(len(newblockref.hash), block_hash_size)
            assert_equal(newblockref.height, current_block_height + 1)
            self.log.debug("Wait for timeout")
            oldblockref = (await mining.waitTipChanged(ctx, newblockref.hash, self.default_ipc_timeout)).result
            assert_equal(len(newblockref.hash), block_hash_size)
            assert_equal(oldblockref.hash, newblockref.hash)
            assert_equal(oldblockref.height, newblockref.height)

            self.log.debug("interrupt() should abort waitTipChanged()")
            async def wait_for_tip():
                long_timeout = max(self.default_ipc_timeout, 60000.0)  # at least 1 minute
                result = (await mining.waitTipChanged(ctx, newblockref.hash, long_timeout)).result
                # Unlike a timeout, interrupt() returns an empty BlockRef.
                assert_equal(len(result.hash), 0)
            await wait_and_do(wait_for_tip(), mining.interrupt())

        asyncio.run(capnp.run(async_routine()))

    def run_early_startup_test(self):
        """Make sure mining.createNewBlock safely returns on early startup as
        soon as mining interface is available """
        self.log.info("Running Mining interface early startup test")

        node = self.nodes[0]
        self.stop_node(node.index)
        node.start()

        async def async_routine():
            while True:
                try:
                    ctx, mining = await make_mining_ctx(self)
                    break
                except (ConnectionRefusedError, FileNotFoundError):
                    # Poll quickly to connect as soon as socket becomes
                    # available but without using a lot of CPU
                    await asyncio.sleep(0.005)

            opts = self.capnp_modules['mining'].BlockCreateOptions()
            await mining.createNewBlock(ctx, opts)

        asyncio.run(capnp.run(async_routine()))

        # Reconnect nodes so next tests are happy
        node.wait_for_rpc_connection()
        self.connect_nodes(1, 0)
        # Restarting node 0 drops its P2P connection to the rest of the test
        # chain. Restore the synced 0-1-2 topology before later tests split
        # node 2 off for submitBlock checks.
        self.sync_all()

    def run_block_template_test(self):
        """Test BlockTemplate interface methods."""
        self.log.info("Running BlockTemplate interface test")
        block_header_size = 80

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)

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

                self.log.debug("createNewBlock() should wake up promptly after tip advances")
                success = False
                duration = 0.0
                async def wait_fn():
                    nonlocal success, duration
                    start = time.time()
                    res = await mining.createNewBlock(ctx, self.default_block_create_options)
                    duration = time.time() - start
                    success = res._has("result")
                def do_fn():
                    block_hex = self.nodes[1].getblock(node1_block_hash, False)
                    self.nodes[0].submitblock(block_hex)
                await wait_and_do(wait_fn(), do_fn)
                assert_equal(success, True)
                if self.options.timeout_factor <= 1:
                    assert duration < 3.0, f"createNewBlock took {duration:.2f}s, did not wake up promptly after tip advances"
                else:
                    self.log.debug("Skipping strict wake-up duration check because timeout_factor > 1")

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
                waitoptions.timeout = self.default_ipc_timeout
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
                    new_waitoptions.timeout = max(self.default_ipc_timeout, 60000.0)  # at least 1 minute
                    new_waitoptions.feeThreshold = 1
                    template7 = await mining_wait_next_template(template6, stack, ctx, new_waitoptions)
                    assert template7 is None
                await wait_and_do(wait_for_block(), template6.interruptWait())

        asyncio.run(capnp.run(async_routine()))

    def run_ipc_option_override_test(self):
        self.log.info("Running IPC option override test")
        # Confirm that BlockCreateOptions.blockReservedWeight takes precedence
        # over -blockreservedweight. Set an absurdly high -blockreservedweight
        # value that would result in empty blocks to verify this. IPC clients set
        # blockReservedWeight per template request and are unaffected; later in
        # the test the IPC template includes a mempool transaction.
        self.restart_node(0, extra_args=[f"-blockreservedweight={MAX_BLOCK_WEIGHT}"])
        self.miniwallet.rescan_utxos()

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
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
            await assert_create_new_block_fails(ctx, mining, opts,
                "block_reserved_weight (0) is lower than minimum safety value of (2000)")

        async def async_routine_check_max_reserved_weight():
            self.log.debug("Enforce maximum reserved weight for IPC clients too")
            ctx, mining = await make_mining_ctx(self)
            opts = self.capnp_modules['mining'].BlockCreateOptions()
            opts.blockReservedWeight = MAX_BLOCK_WEIGHT + 1
            await assert_create_new_block_fails(ctx, mining, opts,
                f"block_reserved_weight ({MAX_BLOCK_WEIGHT + 1}) exceeds consensus maximum block weight ({MAX_BLOCK_WEIGHT})")

        async def async_routine_check_sigops_limit():
            self.log.debug("Enforce sigops limit for IPC clients too")
            ctx, mining = await make_mining_ctx(self)
            opts = self.capnp_modules['mining'].BlockCreateOptions()
            opts.coinbaseOutputMaxAdditionalSigops = MAX_BLOCK_SIGOPS_COST + 1
            await assert_create_new_block_fails(ctx, mining, opts,
                f"coinbase_output_max_additional_sigops ({MAX_BLOCK_SIGOPS_COST + 1}) exceeds consensus maximum block sigops cost ({MAX_BLOCK_SIGOPS_COST})")

        asyncio.run(capnp.run(async_routine()))
        asyncio.run(capnp.run(async_routine_check_max_reserved_weight()))
        asyncio.run(capnp.run(async_routine_check_sigops_limit()))
        self.restart_node(0)
        self.connect_nodes(0, 1)
        self.miniwallet.rescan_utxos()

    def run_waitnext_mining_policy_test(self):
        """Verify that waitNext() preserves the mining policy from -blockmintxfee
        instead of falling back to defaults."""
        self.log.info("Running waitNext mining policy test")
        block_min_tx_fee = Decimal("0.00002000")
        below_block_min_tx_fee = Decimal("0.00001000")
        above_block_min_tx_fee = Decimal("0.00003000")

        self.restart_node(0, extra_args=[
            f"-blockmintxfee={block_min_tx_fee:.8f}",
            "-minrelaytxfee=0",
            "-persistmempool=0",
        ])

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)

            self.log.debug("Create a below -blockmintxfee transaction")
            low_fee_tx = self.miniwallet.send_self_transfer(
                fee_rate=below_block_min_tx_fee,
                from_node=self.nodes[0],
                confirmed_only=True,
            )
            assert low_fee_tx["txid"] in self.nodes[0].getrawmempool()

            async with AsyncExitStack() as stack:
                self.log.debug("createNewBlock should respect -blockmintxfee")
                template = await mining_create_block_template(mining, stack, ctx, self.default_block_create_options)
                assert template is not None
                block = await mining_get_block(template, ctx)
                assert low_fee_tx["txid"] not in {tx.txid_hex for tx in block.vtx[1:]}

                self.log.debug("waitNext should preserve the same mining policy")
                high_fee_tx = self.miniwallet.send_self_transfer(
                    fee_rate=above_block_min_tx_fee,
                    from_node=self.nodes[0],
                    confirmed_only=True,
                )
                mempool_txids = self.nodes[0].getrawmempool()
                assert high_fee_tx["txid"] in mempool_txids
                assert low_fee_tx["txid"] in mempool_txids
                template_next = await mining_wait_next_template(template, stack, ctx, self.default_block_wait_options)
                assert template_next is not None

                block_next = await mining_get_block(template_next, ctx)
                block_next_txids = {tx.txid_hex for tx in block_next.vtx[1:]}
                assert high_fee_tx["txid"] in block_next_txids
                assert low_fee_tx["txid"] not in block_next_txids

        asyncio.run(capnp.run(async_routine()))

    def run_block_max_weight_test(self):
        """Verify IPC createNewBlock() and waitNext() preserve the -blockmaxweight policy."""
        self.log.info("Running block_max_weight test")

        # Cap that leaves room for only a handful of mempool transactions
        # above DEFAULT_BLOCK_RESERVED_WEIGHT (8000). Well below MAX_BLOCK_WEIGHT
        # (4_000_000), so any truncation observed here is attributable to the
        # cap, not to consensus limits or wallet chain limits.
        small_cap = DEFAULT_BLOCK_RESERVED_WEIGHT + 4000
        NUM_TXS = 20

        self.restart_node(0, extra_args=[
            f"-blockmaxweight={small_cap}",
            "-minrelaytxfee=0",
            "-persistmempool=0",
        ])
        # Refresh miniwallet's UTXO view from the chain after restart.
        self.miniwallet.rescan_utxos()

        # Fill the mempool enough that the configured block weight cap forces
        # template truncation.
        for _ in range(NUM_TXS):
            self.miniwallet.send_self_transfer(from_node=self.nodes[0], confirmed_only=True)
        assert_equal(self.nodes[0].getmempoolinfo()["size"], NUM_TXS)

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            async with AsyncExitStack() as stack:
                template = await mining_create_block_template(mining, stack, ctx, self.default_block_create_options)
                assert template is not None
                block = await mining_get_block(template, ctx)
                assert_greater_than_or_equal(small_cap, block.get_weight())
                # Exclude the coinbase; the cap must have forced truncation.
                initial_included = len(block.vtx) - 1
                assert initial_included < NUM_TXS, (
                    f"Expected -blockmaxweight={small_cap} to truncate; "
                    f"included {initial_included}/{NUM_TXS} mempool txs"
                )

                self.log.debug("waitNext should preserve -blockmaxweight")
                high_fee_tx = self.miniwallet.send_self_transfer(
                    from_node=self.nodes[0],
                    confirmed_only=True,
                    fee_rate=10,
                )
                template_next = await mining_wait_next_template(template, stack, ctx, self.default_block_wait_options)
                assert template_next is not None

                block_next = await mining_get_block(template_next, ctx)
                assert_greater_than_or_equal(small_cap, block_next.get_weight())
                assert high_fee_tx["txid"] in {tx.txid_hex for tx in block_next.vtx[1:]}
                next_included = len(block_next.vtx) - 1
                assert next_included < NUM_TXS + 1, (
                    f"Expected -blockmaxweight={small_cap} to remain capped after waitNext; "
                    f"included {next_included}/{NUM_TXS + 1} mempool txs"
                )

        asyncio.run(capnp.run(async_routine()))

    def run_coinbase_and_submission_test(self):
        """Test coinbase construction (getCoinbaseTx) and block submission (submitSolution)."""
        self.log.info("Running coinbase construction and submission test")

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            # Node 0 drives the checkBlock() and submitSolution() checks. Node
            # 2 has a separate IPC interface and starts synced through node 1,
            # so it can be isolated below to test submitBlock() without
            # changing node 0's template and chain state first.
            ctx2, mining2 = await make_mining_ctx(self, node_index=2)

            current_block_height = self.nodes[0].getchaintips()[0]["height"]
            check_opts = self.capnp_modules['mining'].BlockCheckOptions()

            # Send a real transaction so the template includes it.
            self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0])

            async with destroying((await mining.createNewBlock(ctx, self.default_block_create_options)).result, ctx) as template:
                block = await self.build_candidate_block(template, ctx)
                coinbase = block.vtx[0]
                self.log.debug("Template should include a mempool transaction")
                assert len(block.vtx) >= 2, "Block should include at least the coinbase and the mempool tx"
                balance = self.miniwallet.get_balance()
                original_version = block.nVersion

                self.log.debug("Disconnect node 2 before block submission tests")
                # The default topology is 2 -> 1 -> 0. Splitting the 1-2 edge
                # lets node 2 accept/reject complete blocks independently.
                self.disconnect_nodes(1, 2)

                self.log.debug("submitSolution should reject an empty coinbase")
                submitted = (await template.submitSolution(ctx, 0, 0, 0, b"")).result
                assert_equal(submitted, False)

                self.log.debug("Submit solution that can't be deserialized")
                try:
                    await template.submitSolution(ctx, 0, 0, 0, b"\x00")
                    raise AssertionError("submitSolution unexpectedly succeeded")
                except capnp.lib.capnp.KjException as e:
                    assert_capnp_failed(e, "remote exception: std::exception: SpanReader::read(): end of data:")

                self.log.debug("Submit a block with a bad version")
                block.nVersion = 0
                block.solve()
                check = await mining.checkBlock(ctx, block.serialize(), check_opts)
                assert_equal(check.result, False)
                assert_equal(check.reason, "bad-version(0x00000000)")
                assert_equal(check.debug, "rejected nVersion=0x00000000 block")
                self.log.debug("submitSolution should reject a bad-version block")
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())).result
                assert_equal(submitted, False)
                self.log.debug("submitBlock should reject a bad-version block")
                await self.assert_submit_block(
                    mining2,
                    ctx2,
                    block,
                    result=False,
                    reason="bad-version(0x00000000)",
                    debug="rejected nVersion=0x00000000 block",
                )
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
                missing_witness_block = deepcopy(block)
                missing_witness_block.vtx[0].wit.vtxinwit = []
                has_witness_commitment = any(
                    len(o.scriptPubKey) >= 38 and o.scriptPubKey[2:6] == WITNESS_COMMITMENT_HEADER
                    for o in missing_witness_block.vtx[0].vout
                )
                assert has_witness_commitment, "Coinbase should have a witness commitment output"
                missing_witness_block.hashMerkleRoot = missing_witness_block.calc_merkle_root()
                missing_witness_block.solve()
                self.log.debug("submitSolution should reject a coinbase missing witness")
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize_without_witness())).result
                assert_equal(submitted, False)

                self.log.debug("Even a rejected submitSolution() mutates the template's block")
                # Can be used by clients to download and inspect the (rejected)
                # reconstructed block.
                remote_block_after = await mining_get_block(template, ctx)
                assert_not_equal(remote_block_before.serialize().hex(), remote_block_after.serialize().hex())

                self.log.debug("submitBlock should reject a block missing coinbase witness")
                await self.assert_submit_block(
                    mining2,
                    ctx2,
                    missing_witness_block,
                    result=False,
                    reason="bad-witness-nonce-size",
                    debug="CheckWitnessMalleation : invalid witness reserved value size",
                )

                self.log.debug("Submit again, with the witness")
                submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())).result
                assert_equal(submitted, True)

                self.log.debug("Submit a valid complete block through the disconnected node")
                await self.assert_submit_block(mining2, ctx2, block, result=True)
                assert_equal(self.nodes[2].getchaintips()[0]["height"], current_block_height + 1)
                self.log.debug("submitBlock should reject the duplicate complete block")
                await self.assert_submit_block(mining2, ctx2, block, result=False, reason="duplicate")

            self.log.debug("Block should propagate")
            # Check that the IPC node actually updates its own chain
            assert_equal(self.nodes[0].getchaintips()[0]["height"], current_block_height + 1)
            # Stalls if a regression causes submitSolution() to accept an invalid block:
            self.sync_blocks(self.nodes[:2])
            # Rejoin node 2 and verify the submitBlock result converges with
            # the submitSolution result from node 0.
            self.connect_nodes(1, 2)
            self.sync_all()
            # Check that the other node accepts the block
            assert_equal(self.nodes[0].getchaintips()[0], self.nodes[1].getchaintips()[0])

            self.miniwallet.rescan_utxos()
            assert_equal(self.miniwallet.get_balance(), balance + 1)
            self.log.debug("Check block should fail now, since it is a duplicate")
            check = await mining.checkBlock(ctx, block.serialize(), check_opts)
            assert_equal(check.result, False)
            assert_equal(check.reason, "inconclusive-not-best-prevblk")
            self.log.debug("submitBlock on the same node should fail with duplicate after submitSolution succeeds")
            await self.assert_submit_block(mining, ctx, block, result=False, reason="duplicate")

            self.log.debug("submitSolution should still return True for a duplicate after submitBlock succeeds")
            async with destroying((await mining2.createNewBlock(ctx2, self.default_block_create_options)).result, ctx2) as template2:
                duplicate_block = await self.build_candidate_block(template2, ctx2)
                duplicate_coinbase = duplicate_block.vtx[0]
                duplicate_block.solve()
                self.log.debug("Submit a valid complete block before duplicate submitSolution")
                await self.assert_submit_block(mining2, ctx2, duplicate_block, result=True)
                self.nodes[2].waitforblockheight(current_block_height + 2)
                self.log.debug("submitSolution should accept the duplicate block")
                submitted = (await template2.submitSolution(ctx2, duplicate_block.nVersion, duplicate_block.nTime, duplicate_block.nNonce, duplicate_coinbase.serialize())).result
                assert_equal(submitted, True)
            self.sync_all()

            self.log.debug("Submit the same invalid block twice")
            async with destroying((await mining2.createNewBlock(ctx2, self.default_block_create_options)).result, ctx2) as template2:
                invalid_block = await self.build_candidate_block(template2, ctx2)
                invalid_block.vtx[0].nLockTime = 2**32 - 1
                invalid_block.hashMerkleRoot = invalid_block.calc_merkle_root()
                invalid_block.solve()
                self.log.debug("submitBlock should reject the non-final block")
                await self.assert_submit_block(
                    mining2,
                    ctx2,
                    invalid_block,
                    result=False,
                    reason="bad-txns-nonfinal",
                    debug="non-final transaction",
                )
                self.log.debug("submitBlock should report duplicate-invalid for the same block")
                await self.assert_submit_block(
                    mining2,
                    ctx2,
                    invalid_block,
                    result=False,
                    reason="duplicate-invalid",
                    debug=f"block {invalid_block.hash_hex} was previously marked invalid",
                )

            self.log.debug("Submit a malformed complete block")
            try:
                await mining2.submitBlock(ctx2, block.serialize()[:-15])
                raise AssertionError("submitBlock unexpectedly succeeded")
            except capnp.lib.capnp.KjException as e:
                assert_capnp_failed(e, "remote exception: std::exception: SpanReader::read(): end of data:")

            self.log.debug("Submit empty block data")
            try:
                await mining2.submitBlock(ctx2, b"")
                raise AssertionError("submitBlock unexpectedly succeeded")
            except capnp.lib.capnp.KjException as e:
                assert_capnp_failed(e, "remote exception: std::exception: SpanReader::read(): end of data:")
            assert_equal(self.nodes[2].is_node_stopped(), False)

        asyncio.run(capnp.run(async_routine()))

    def run_transaction_lookup_test(self):
        """Test getTransactionsByTxID() and getTransactionsByWitnessID()."""
        self.log.info("Running transaction lookup test")

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            tx1 = self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0])
            tx2 = self.miniwallet.send_self_transfer(fee_rate=10, from_node=self.nodes[0], utxo_to_spend=tx1["new_utxo"])

            self.log.debug("getTransactionsByTxID() returns mempool txs and nulls")
            raw_txs_txid = await mining.getTransactionsByTxID(ctx, [tx1["tx"].txid, tx2["tx"].txid, bytes(32)])
            assert_equal(len(raw_txs_txid.result), 3)
            assert_equal(raw_txs_txid.result[0].hex(), tx1["hex"])
            assert_equal(raw_txs_txid.result[1].hex(), tx2["hex"])
            assert_equal(raw_txs_txid.result[2], b'')

            self.log.debug("getTransactionsByWitnessID() returns mempool txs and nulls")
            raw_txs_wtxid = await mining.getTransactionsByWitnessID(ctx, [tx1["tx"].wtxid, tx2["tx"].wtxid, bytes(32)])
            assert_equal(len(raw_txs_wtxid.result), 3)
            assert_equal(raw_txs_wtxid.result[0].hex(), tx1["hex"])
            assert_equal(raw_txs_wtxid.result[1].hex(), tx2["hex"])
            assert_equal(raw_txs_wtxid.result[2], b'')

            self.log.debug("Mined transactions are not returned")
            self.generate(self.nodes[0], 1)
            self.sync_all()
            raw_txs = await mining.getTransactionsByTxID(ctx, [tx1["tx"].txid])
            assert_equal(raw_txs.result[0], b'')
            raw_txs = await mining.getTransactionsByWitnessID(ctx, [tx1["tx"].wtxid])
            assert_equal(raw_txs.result[0], b'')

        asyncio.run(capnp.run(async_routine()))

    def run_low_height_test(self):
        """Test that IPC createNewBlock() works at low block heights on a
        clean chain, in particular with regard to bad-cb-length.

        createNewBlock pads the scriptSig with a dummy extranonce to pass
        its internal CheckBlock(). This dummy is omitted from the
        getCoinbaseTx() script_sig_prefix field. The client provides its
        own extraNonce via submitSolution()."""
        self.log.info("Running low block height test")

        node = self.nodes[0]
        self.stop_node(0)
        # Clear chain data to start from genesis
        self.cleanup_folder(node.chain_path)
        node.start()
        node.wait_for_rpc_connection()
        assert_equal(node.getblockcount(), 0)

        async def async_routine():
            ctx, mining = await make_mining_ctx(self)
            opts = self.capnp_modules['mining'].BlockCreateOptions()

            # Mine blocks 1-17 to exercise the boundary at height 16, where the
            # internal scriptSig padding is no longer needed.
            for height in range(1, 18):
                async with AsyncExitStack() as stack:
                    # Disable cooldown to avoid hanging in the IBD loop on a fresh chain
                    template = await mining_create_block_template(mining, stack, ctx, opts, cooldown=False)
                    assert template is not None
                    block = await mining_get_block(template, ctx)
                    # Heights <= 16 need extra nonce padding.
                    extra_nonce = b'\xaa\xbb\xcc\xdd' if height <= 16 else b""
                    coinbase = await self.build_coinbase_test(template, ctx, self.miniwallet, extra_nonce=extra_nonce)
                    block.vtx[0] = coinbase
                    block.hashMerkleRoot = block.calc_merkle_root()
                    block.solve()
                    submitted = (await template.submitSolution(ctx, block.nVersion, block.nTime, block.nNonce, coinbase.serialize())).result
                    assert_equal(submitted, True)
                    assert_equal(node.getblockcount(), height)

        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.miniwallet = MiniWallet(self.nodes[0])
        # Amount of time in milliseconds the test is allowed to wait or be idle before it should fail.
        self.default_ipc_timeout = 1000.0 * self.options.timeout_factor
        self.default_block_create_options = self.capnp_modules['mining'].BlockCreateOptions()
        self.default_block_wait_options = self.capnp_modules['mining'].BlockWaitOptions()
        self.default_block_wait_options.timeout = self.default_ipc_timeout
        self.default_block_wait_options.feeThreshold = 1
        self.run_mining_interface_test()
        self.run_early_startup_test()
        self.run_block_template_test()
        self.run_coinbase_and_submission_test()
        self.run_waitnext_mining_policy_test()
        self.run_block_max_weight_test()
        self.run_ipc_option_override_test()
        self.run_transaction_lookup_test()

        # Needs to run last because it resets the chain.
        self.run_low_height_test()


if __name__ == '__main__':
    IPCMiningTest(__file__).main()
