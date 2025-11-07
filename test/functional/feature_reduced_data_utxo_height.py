#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Knots developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test REDUCED_DATA soft fork UTXO height checking.

This test verifies that the REDUCED_DATA deployment correctly exempts UTXOs
created before ReducedDataHeightBegin from reduced_data script validation rules,
as implemented in validation.cpp.

Test scenarios:
1. Old UTXO (created before activation) spent during active period with violation - should be ACCEPTED (EXEMPT)
2. New UTXO (created during active period) spent with violation - should be REJECTED
3. Mixed inputs (old + new UTXOs) in same transaction
4. Boundary test: UTXO created at exactly ReducedDataHeightBegin
"""

from io import BytesIO

from test_framework.blocktools import (
    COINBASE_MATURITY,
    create_block,
    create_coinbase,
    add_witness_commitment,
)
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    CScript,
    OP_TRUE,
    OP_DROP,
)
from test_framework.script_util import (
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet


# BIP9 constants for regtest
BIP9_PERIOD = 144  # blocks per period in regtest
BIP9_THRESHOLD = 108  # 75% of 144
VERSIONBITS_TOP_BITS = 0x20000000
REDUCED_DATA_BIT = 4

# REDUCED_DATA enforces MAX_SCRIPT_ELEMENT_SIZE_REDUCED (256) instead of MAX_SCRIPT_ELEMENT_SIZE (520)
MAX_ELEMENT_SIZE_STANDARD = 520
MAX_ELEMENT_SIZE_REDUCED = 256
VIOLATION_SIZE = 300  # Violates reduced (256) but OK for standard (520)


class ReducedDataUTXOHeightTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # Activate REDUCED_DATA using BIP9 with min_activation_height=288
        # Due to BIP9 design, period 0 is always DEFINED, so signaling happens in period 1
        # This activates at height 432 (start of period 3)
        # Format: deployment:start:timeout:min_activation_height:max_activation_height:active_duration
        # start_time=0, timeout=999999999999 (never), min_activation_height=288, max=2147483647 (INT_MAX, disabled), active_duration=2147483647 (permanent)
        self.extra_args = [[
            '-vbparams=reduced_data:0:999999999999:288:2147483647:2147483647',
        ]]

    def create_p2wsh_funding_and_spending_tx(self, wallet, node, witness_element_size):
        """Create a P2WSH output, then a transaction spending it with custom witness size.

        Returns:
            tuple: (funding_tx, spending_tx) where funding_tx creates P2WSH output,
                   spending_tx spends it with witness element of specified size
        """
        # Create a simple witness script: <data> OP_DROP OP_TRUE
        # This allows us to put arbitrary data in the witness
        witness_script = CScript([OP_DROP, OP_TRUE])
        script_pubkey = script_to_p2wsh_script(witness_script)

        # Use MiniWallet to create funding transaction to P2WSH output
        funding_txid = wallet.send_to(from_node=node, scriptPubKey=script_pubkey, amount=100000)['txid']
        funding_tx_hex = node.getrawtransaction(funding_txid)
        funding_tx = CTransaction()
        funding_tx.deserialize(BytesIO(bytes.fromhex(funding_tx_hex)))
        funding_tx.txid_hex  # Calculate sha256 hash after deserializing

        # Find the P2WSH output
        p2wsh_vout = None
        for i, vout in enumerate(funding_tx.vout):
            if vout.scriptPubKey == script_pubkey:
                p2wsh_vout = i
                break
        assert p2wsh_vout is not None, "P2WSH output not found"

        # Spending transaction: spend P2WSH output with custom witness
        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(funding_tx.txid_int, p2wsh_vout))]
        spending_tx.vout = [CTxOut(funding_tx.vout[p2wsh_vout].nValue - 1000, CScript([OP_TRUE]))]

        # Create witness with element of specified size
        spending_tx.wit.vtxinwit.append(CTxInWitness())
        spending_tx.wit.vtxinwit[0].scriptWitness.stack = [
            b'\x42' * witness_element_size,  # Data element of specified size
            witness_script  # Witness script
        ]
        spending_tx.txid_hex

        return funding_tx, spending_tx

    def create_test_block(self, txs, signal=False):
        """Create a block with the given transactions."""
        # Always get fresh tip and height to ensure blocks chain correctly
        tip = self.nodes[0].getbestblockhash()
        height = self.nodes[0].getblockcount() + 1
        tip_header = self.nodes[0].getblockheader(tip)
        block_time = tip_header['time'] + 1
        block = create_block(int(tip, 16), create_coinbase(height), ntime=block_time, txlist=txs)
        if signal:
            block.nVersion = VERSIONBITS_TOP_BITS | (1 << REDUCED_DATA_BIT)
        add_witness_commitment(block)
        block.solve()
        return block

    def mine_blocks(self, count, signal=False):
        """Mine blocks with optional BIP9 signaling for REDUCED_DATA."""
        for _ in range(count):
            block = self.create_test_block([], signal=signal)
            result = self.nodes[0].submitblock(block.serialize().hex())
            if result is not None:
                raise AssertionError(f"submitblock failed: {result}")
            # Verify block was accepted
            assert self.nodes[0].getbestblockhash() == block.hash_hex

    def run_test(self):
        node = self.nodes[0]
        self.peer = node.add_p2p_connection(P2PDataStore())

        # Use MiniWallet for easy UTXO management
        wallet = MiniWallet(node)

        self.log.info("Mining blocks to activate REDUCED_DATA via BIP9...")

        # BIP9 state timeline with start_time=0:
        # - Period 0 (blocks 0-143): DEFINED (cannot signal yet)
        # - Period 1 (blocks 144-287): STARTED (signal here with 108/144 threshold)
        # - Period 2 (blocks 288-431): LOCKED_IN (if threshold met in period 1)
        # - Period 3 (blocks 432-575): ACTIVE

        # Mine through period 0 (DEFINED state)
        self.log.info("Mining through period 0 (DEFINED)...")
        self.generate(wallet, 144)
        self.log.info(f"DEBUG: After period 0, height = {node.getblockcount()}")

        # Mine 108 signaling blocks in period 1 (STARTED state)
        self.log.info("Mining 108 signaling blocks in period 1 (blocks 144-251)...")
        self.mine_blocks(108, signal=True)
        self.log.info(f"DEBUG: After 108 signaling blocks, height = {node.getblockcount()}")

        # Mine to end of period 1 (block 287)
        self.log.info("Mining to end of period 1 (block 287)...")
        self.mine_blocks(287 - 144 - 108, signal=False)
        self.log.info(f"DEBUG: After period 1, height = {node.getblockcount()}")

        # Check that we're LOCKED_IN at start of period 2
        self.generate(wallet, 1)  # Mine block 288
        self.log.info(f"DEBUG: After mining block 288, height = {node.getblockcount()}")
        deployment_info = node.getdeploymentinfo()
        rd_info = deployment_info['deployments']['reduced_data']
        if 'bip9' in rd_info:
            status = rd_info['bip9']['status']
            self.log.info(f"At height {node.getblockcount()}, REDUCED_DATA status: {status}")
            assert status == 'locked_in', f"Expected LOCKED_IN at block 288, got {status}"
        else:
            raise AssertionError("REDUCED_DATA deployment not found")

        # Mine to block 432 (start of period 3) where activation occurs
        self.log.info("Mining to block 432 for activation...")
        self.generate(wallet, 432 - 288)

        current_height = node.getblockcount()

        # Check activation status
        deployment_info = node.getdeploymentinfo()
        rd_info = deployment_info['deployments']['reduced_data']
        if 'bip9' in rd_info:
            status = rd_info['bip9']['status']
            self.log.info(f"At height {current_height}, REDUCED_DATA status: {status}")
            if status == 'active':
                ACTIVATION_HEIGHT = rd_info['bip9']['since']
            else:
                raise AssertionError(f"REDUCED_DATA not active at height {current_height}, status: {status}")
        else:
            raise AssertionError("REDUCED_DATA deployment not found")

        self.log.info(f"✓ REDUCED_DATA activated at height {ACTIVATION_HEIGHT}")
        assert ACTIVATION_HEIGHT == 432, f"Expected activation at 432, got {ACTIVATION_HEIGHT}"

        # Initialize wallet with some coins
        self.generate(wallet, COINBASE_MATURITY + 10)
        current_height = node.getblockcount()

        # Now rewind to before activation to create test UTXOs
        # Rewind to 20 blocks before activation
        target_height = ACTIVATION_HEIGHT - 20
        blocks_to_invalidate = current_height - target_height
        self.log.info(f"Rewinding {blocks_to_invalidate} blocks to height {target_height}...")
        for _ in range(blocks_to_invalidate):
            node.invalidateblock(node.getbestblockhash())

        assert_equal(node.getblockcount(), target_height)

        # ======================================================================
        # Test 1: Create OLD UTXO before activation
        # ======================================================================
        self.log.info("Test 1: Creating P2WSH UTXO before activation height...")

        # Create P2WSH funding transaction for old UTXO
        old_funding_tx, old_spending_tx = self.create_p2wsh_funding_and_spending_tx(
            wallet, node, VIOLATION_SIZE
        )

        # Confirm the funding transaction in a block
        block = self.create_test_block([old_funding_tx], signal=False)
        node.submitblock(block.serialize().hex())
        old_utxo_height = node.getblockcount()

        self.log.info(f"Created old P2WSH UTXO at height {old_utxo_height} (< {ACTIVATION_HEIGHT})")

        # ======================================================================
        # Test 2: Mine to activation height
        # ======================================================================
        self.log.info("Test 2: Mining to activation height...")

        current_height = node.getblockcount()
        blocks_to_activation = ACTIVATION_HEIGHT - current_height
        if blocks_to_activation > 0:
            self.mine_blocks(blocks_to_activation, signal=False)

        current_height = node.getblockcount()
        assert_equal(current_height, ACTIVATION_HEIGHT)
        self.log.info(f"At activation height: {current_height}")

        # Verify REDUCED_DATA is active
        deployment_info = node.getdeploymentinfo()
        rd_info = deployment_info['deployments']['reduced_data']
        if 'bip9' in rd_info:
            status = rd_info['bip9']['status']
        else:
            status = 'active' if rd_info.get('active') else 'unknown'
        assert status == 'active', f"Expected 'active' at height {current_height}, got '{status}'"

        # ======================================================================
        # Test 3: Create NEW UTXO at/after activation
        # ======================================================================
        self.log.info("Test 3: Creating P2WSH UTXO at activation height...")

        # Create P2WSH funding transaction for new UTXO
        new_funding_tx, new_spending_tx = self.create_p2wsh_funding_and_spending_tx(
            wallet, node, VIOLATION_SIZE
        )

        # Confirm the funding transaction in a block
        block = self.create_test_block([new_funding_tx], signal=False)
        node.submitblock(block.serialize().hex())
        new_utxo_height = node.getblockcount()

        self.log.info(f"Created new P2WSH UTXO at height {new_utxo_height} (>= {ACTIVATION_HEIGHT})")

        # Mine a few more blocks
        self.mine_blocks(5, signal=False)
        current_height = node.getblockcount()
        self.log.info(f"Current height: {current_height}")

        # ======================================================================
        # Test 4: Spend OLD UTXO with oversized witness - should be ACCEPTED
        # ======================================================================
        self.log.info(f"Test 4: Spending old UTXO (height {old_utxo_height}) with {VIOLATION_SIZE}-byte witness element...")
        self.log.info(f"        This violates REDUCED_DATA ({MAX_ELEMENT_SIZE_REDUCED} limit) but old UTXOs should be EXEMPT")

        # Try to mine block with old_spending_tx (has 300-byte witness element)
        block = self.create_test_block([old_spending_tx], signal=False)
        result = node.submitblock(block.serialize().hex())
        assert result is None, f"Expected success, got: {result}"

        self.log.info(f"✓ SUCCESS: Old UTXO with {VIOLATION_SIZE}-byte witness element was ACCEPTED (correctly exempt)")

        # ======================================================================
        # Test 5: Spend NEW UTXO with oversized witness - should be REJECTED
        # ======================================================================
        self.log.info(f"Test 5: Spending new UTXO (height {new_utxo_height}) with {VIOLATION_SIZE}-byte witness element...")
        self.log.info(f"        This violates REDUCED_DATA ({MAX_ELEMENT_SIZE_REDUCED} limit) and should be REJECTED")

        # Try to mine block with new_spending_tx (has 300-byte witness element)
        block = self.create_test_block([new_spending_tx], signal=False)
        result = node.submitblock(block.serialize().hex())
        assert result is not None and 'script-verify-flag-failed' in result, f"Expected rejection, got: {result}"

        self.log.info(f"✓ SUCCESS: New UTXO with {VIOLATION_SIZE}-byte witness element was REJECTED (correctly enforced)")

        # ======================================================================
        # Test 6: Boundary test - UTXO at exactly ReducedDataHeightBegin
        # ======================================================================
        self.log.info(f"Test 6: Boundary test - verifying UTXO at activation height {ACTIVATION_HEIGHT}...")

        # The new_funding_tx was confirmed at height ACTIVATION_HEIGHT+1, but let's create one AT height ACTIVATION_HEIGHT
        # First, invalidate back to height ACTIVATION_HEIGHT-1
        current_tip = node.getbestblockhash()
        blocks_to_invalidate = node.getblockcount() - (ACTIVATION_HEIGHT - 1)
        for _ in range(blocks_to_invalidate):
            node.invalidateblock(node.getbestblockhash())

        assert_equal(node.getblockcount(), ACTIVATION_HEIGHT - 1)
        self.log.info(f"        Rewound to height {node.getblockcount()}")

        # Create UTXO exactly at activation height
        boundary_funding_tx, boundary_spending_tx = self.create_p2wsh_funding_and_spending_tx(
            wallet, node, VIOLATION_SIZE
        )
        block = self.create_test_block([boundary_funding_tx], signal=False)
        result = node.submitblock(block.serialize().hex())
        assert result is None, f"Expected success, got: {result}"
        boundary_height = node.getblockcount()
        assert_equal(boundary_height, ACTIVATION_HEIGHT)

        self.log.info(f"        Created boundary UTXO at height {boundary_height} (exactly at activation)")

        # Mine a few blocks past activation
        self.mine_blocks(5, signal=False)

        # Try to spend boundary UTXO - should be REJECTED (height ACTIVATION_HEIGHT >= ACTIVATION_HEIGHT)
        self.log.info(f"        Spending boundary UTXO with {VIOLATION_SIZE}-byte witness (should be REJECTED)")
        block = self.create_test_block([boundary_spending_tx], signal=False)
        result = node.submitblock(block.serialize().hex())
        assert result is not None and 'script-verify-flag-failed' in result, f"Expected rejection, got: {result}"

        self.log.info(f"✓ SUCCESS: UTXO at exactly activation height {ACTIVATION_HEIGHT} is SUBJECT to rules (not exempt)")

        # Restore chain to where we were
        node.reconsiderblock(current_tip)

        # ======================================================================
        # Test 7: Mixed inputs - one old (exempt) + one new (subject to rules)
        # ======================================================================
        self.log.info("Test 7: Creating transaction with mixed inputs (old + new UTXOs)...")

        # We need fresh old and new UTXOs. Rewind to before activation again
        current_tip2 = node.getbestblockhash()
        blocks_to_invalidate = node.getblockcount() - (ACTIVATION_HEIGHT - 20)
        for _ in range(blocks_to_invalidate):
            node.invalidateblock(node.getbestblockhash())

        # Create OLD UTXO at height before activation
        old_mixed_funding, old_mixed_spending = self.create_p2wsh_funding_and_spending_tx(
            wallet, node, VIOLATION_SIZE
        )
        block = self.create_test_block([old_mixed_funding], signal=False)
        node.submitblock(block.serialize().hex())
        old_mixed_height = node.getblockcount()
        self.log.info(f"        Created old UTXO at height {old_mixed_height}")

        # Mine to after activation
        blocks_to_mine = ACTIVATION_HEIGHT - node.getblockcount() + 5
        self.mine_blocks(blocks_to_mine, signal=False)

        # Create NEW UTXO at height after activation
        new_mixed_funding, new_mixed_spending = self.create_p2wsh_funding_and_spending_tx(
            wallet, node, VIOLATION_SIZE
        )
        block = self.create_test_block([new_mixed_funding], signal=False)
        node.submitblock(block.serialize().hex())
        new_mixed_height = node.getblockcount()
        self.log.info(f"        Created new UTXO at height {new_mixed_height}")

        # Find P2WSH outputs in funding transactions
        witness_script = CScript([OP_DROP, OP_TRUE])
        script_pubkey = script_to_p2wsh_script(witness_script)

        old_p2wsh_vout = None
        for i, vout in enumerate(old_mixed_funding.vout):
            if vout.scriptPubKey == script_pubkey:
                old_p2wsh_vout = i
                break

        new_p2wsh_vout = None
        for i, vout in enumerate(new_mixed_funding.vout):
            if vout.scriptPubKey == script_pubkey:
                new_p2wsh_vout = i
                break

        # Create transaction with BOTH inputs
        mixed_tx = CTransaction()
        mixed_tx.vin = [
            CTxIn(COutPoint(old_mixed_funding.txid_int, old_p2wsh_vout)),  # Old UTXO (exempt)
            CTxIn(COutPoint(new_mixed_funding.txid_int, new_p2wsh_vout)),  # New UTXO (subject to rules)
        ]
        total_value = (old_mixed_funding.vout[old_p2wsh_vout].nValue +
                      new_mixed_funding.vout[new_p2wsh_vout].nValue - 2000)
        mixed_tx.vout = [CTxOut(total_value, CScript([OP_TRUE]))]

        # Add witness for both inputs - both with 300-byte elements
        mixed_tx.wit.vtxinwit = []

        # Input 0: old UTXO (would pass alone)
        wit0 = CTxInWitness()
        wit0.scriptWitness.stack = [b'\x42' * VIOLATION_SIZE, witness_script]
        mixed_tx.wit.vtxinwit.append(wit0)

        # Input 1: new UTXO (would fail)
        wit1 = CTxInWitness()
        wit1.scriptWitness.stack = [b'\x42' * VIOLATION_SIZE, witness_script]
        mixed_tx.wit.vtxinwit.append(wit1)

        mixed_tx.txid_hex

        self.log.info(f"        Mixed tx: old UTXO (height {old_mixed_height}, exempt) + new UTXO (height {new_mixed_height}, subject)")
        self.log.info(f"        Both inputs have {VIOLATION_SIZE}-byte witness elements")

        # Try to mine block - should REJECT because new input violates
        self.mine_blocks(2, signal=False)
        block = self.create_test_block([mixed_tx], signal=False)
        result = node.submitblock(block.serialize().hex())
        assert result is not None and 'script-verify-flag-failed' in result, f"Expected rejection, got: {result}"

        self.log.info("✓ SUCCESS: Mixed transaction REJECTED (new input violated rules, even though old input was exempt)")

        # Restore chain
        node.reconsiderblock(current_tip2)

        # ======================================================================
        # Test 8: cache state must not survive activation-boundary reorg
        # ======================================================================
        self.log.info("Test 8: script-execution cache must not survive boundary-context flip")

        def rewind_to(height):
            # Height-based loop: invalidating one tip can switch to an alternate branch at same height.
            while node.getblockcount() > height:
                node.invalidateblock(node.getbestblockhash())
            assert_equal(node.getblockcount(), height)

        branch_point = ACTIVATION_HEIGHT - 2  # 430
        rewind_to(branch_point)

        # spend_tx has a 300-byte witness element: valid only with pre-activation exemption.
        funding_tx, spend_tx = self.create_p2wsh_funding_and_spending_tx(wallet, node, VIOLATION_SIZE)

        # Branch A: funding at 431 (exempt).
        block = self.create_test_block([funding_tx], signal=False)
        assert_equal(node.submitblock(block.serialize().hex()), None)
        assert_equal(node.getblockcount(), ACTIVATION_HEIGHT - 1)

        self.restart_node(0, extra_args=['-vbparams=reduced_data:0:999999999999:288:2147483647:2147483647', '-par=1'])  # Use single-threaded validation to maximize chance of hitting cache-related issues.

        # Validate-only block at height 432. This calls TestBlockValidity(fJustCheck=true),
        # which populates the tx-wide script-execution cache under STRICT flags, even though
        # the spend is only valid here due to the per-input "pre-activation UTXO" exemption.
        self.generateblock(node, output=wallet.get_address(), transactions=[spend_tx.serialize().hex()], submit=False, sync_fun=self.no_op)

        assert_equal(node.getblockcount(), ACTIVATION_HEIGHT - 1)

        # Reorg to branch point; cache state is intentionally retained across reorg.
        rewind_to(branch_point)

        # Branch B: funding at 432 (non-exempt).
        # Make this empty block unique to avoid duplicate-invalid when rebuilding branch B.
        block = self.create_test_block([], signal=False)
        block.nTime += 1
        block.solve()
        assert_equal(node.submitblock(block.serialize().hex()), None)  # 431
        block = self.create_test_block([funding_tx], signal=False)
        assert_equal(node.submitblock(block.serialize().hex()), None)  # 432

        # Same spend is now non-exempt and must be rejected.
        attack_block = self.create_test_block([spend_tx], signal=False)  # 433
        result = node.submitblock(attack_block.serialize().hex())
        assert result is not None and 'Push value size limit exceeded' in result, \
            f"Expected rejection after boundary-crossing reorg, got: {result}"

        self.log.info("✓ SUCCESS: Cache poisoning via activation-boundary reorg correctly prevented")

        # ======================================================================
        # Summary
        # ======================================================================
        self.log.info(f"""
        ============================================================
        TEST SUMMARY - UTXO Height-Based REDUCED_DATA Enforcement
        ============================================================

        ✓ Test 1-3: Setup old and new UTXOs at correct heights
        ✓ Test 4: Old UTXO (height < {ACTIVATION_HEIGHT}) is EXEMPT - 300-byte witness ACCEPTED
        ✓ Test 5: New UTXO (height >= {ACTIVATION_HEIGHT}) is SUBJECT - 300-byte witness REJECTED
        ✓ Test 6: Boundary condition - UTXO at exactly height {ACTIVATION_HEIGHT} is SUBJECT
        ✓ Test 7: Mixed inputs - transaction rejected if ANY input violates
        ✓ Test 8: Cache poisoning via activation-boundary reorg prevented

        Key validations:
        • REDUCED_DATA activated via BIP9 signaling at height {ACTIVATION_HEIGHT}
        • UTXOs created before activation height are EXEMPT from rules
        • UTXOs created at/after activation height are SUBJECT to rules
        • Per-input validation flags work correctly (validation.cpp)
        • Boundary at activation height uses >= operator (not >)

        This confirms the implementation of UTXO height exemption:
        "Exempt inputs spending UTXOs prior to ReducedDataHeightBegin from
        reduced_data script validation rules"

        All 8 tests passed!
        ============================================================
        """)


if __name__ == '__main__':
    ReducedDataUTXOHeightTest(__file__).main()
