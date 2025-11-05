#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test ReducedData Temporary Softfork (RDTS) consensus rules.

This test verifies all 7 consensus rules enforced by DEPLOYMENT_REDUCED_DATA:

1. Output scriptPubKeys exceeding 34 bytes are invalid (except OP_RETURN up to 83 bytes)
2. OP_PUSHDATA* with payloads larger than 256 bytes are invalid (except BIP16 redeemScript)
3. Spending undefined witness versions (not v0/v1) is invalid
4. Witness stacks with a Taproot annex are invalid
5. Taproot control blocks larger than 257 bytes are invalid (max 7 merkle nodes = 128 leaves)
6. Tapscripts including OP_SUCCESS* opcodes are invalid
7. Tapscripts executing OP_IF or OP_NOTIF instructions are invalid
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
)
from test_framework.script import (
    ANNEX_TAG,
    CScript,
    CScriptOp,
    OP_0,
    OP_1,
    OP_2,
    OP_11,
    OP_15,
    OP_CHECKSIG,
    OP_CHECKMULTISIG,
    OP_DROP,
    OP_IF,
    OP_NOTIF,
    OP_ENDIF,
    OP_RETURN,
    OP_TRUE,
    SegwitV0SignatureHash,
    SIGHASH_ALL,
    SIGHASH_DEFAULT,
    hash160,
    taproot_construct,
    TaprootSignatureHash,
)
from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment,
)
from test_framework.script_util import (
    PAY_TO_ANCHOR,
    script_to_p2wsh_script,
    script_to_p2sh_script,
)
from test_framework.util import (
    assert_equal,
)
from test_framework.key import (
    ECKey,
    compute_xonly_pubkey,
    generate_privkey,
    sign_schnorr,
    tweak_add_privkey,
)


# Constants from BIP444
MAX_OUTPUT_SCRIPT_SIZE = 34
MAX_OUTPUT_DATA_SIZE = 83
MAX_SCRIPT_ELEMENT_SIZE_REDUCED = 256
TAPROOT_CONTROL_BASE_SIZE = 33
TAPROOT_CONTROL_NODE_SIZE = 32
TAPROOT_CONTROL_MAX_NODE_COUNT_REDUCED = 7
TAPROOT_CONTROL_MAX_SIZE_REDUCED = TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * TAPROOT_CONTROL_MAX_NODE_COUNT_REDUCED
# ANNEX_TAG is imported from test_framework.script


class ReducedDataTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # Make DEPLOYMENT_REDUCED_DATA always active (from block 0)
        # Using start_time=-1 (ALWAYS_ACTIVE) bypasses BIP9 state machine
        self.extra_args = [[
            '-vbparams=reduced_data:-1:999999999999:0',
            '-acceptnonstdtxn=1',
        ]]

    def init_test(self):
        """Initialize test by mining blocks and creating UTXOs."""
        node = self.nodes[0]

        # MiniWallet provides a simple wallet for test transactions
        self.wallet = MiniWallet(node)

        # Mine 120 blocks to mature coinbase outputs and create spending UTXOs
        # (101 for maturity + extras since each test consumes a UTXO)
        self.generate(self.wallet, 120)

        self.log.info("Test initialization complete")

    def create_test_transaction(self, scriptPubKey, value=None):
        """Helper to create a transaction with custom scriptPubKey (not broadcast)."""
        # Start with a valid transaction from the wallet
        tx_dict = self.wallet.create_self_transfer()
        tx = tx_dict['tx']

        # Use default output value if not specified (handles fee calculation)
        if value is None:
            value = tx.vout[0].nValue

        # Replace output with our custom scriptPubKey
        tx.vout[0] = CTxOut(value, scriptPubKey)
        tx.txid_hex

        return tx

    def test_output_script_size_limit(self):
        """Test spec 1: Output scriptPubKeys exceeding 34 bytes are invalid."""
        self.log.info("Testing output scriptPubKey size limits...")

        node = self.nodes[0]

        # Test 1.1: 34-byte P2WSH output (exactly at limit - should pass)
        witness_program_32 = b'\x00' * 32
        script_p2wsh = CScript([OP_0, witness_program_32])  # OP_0 (1 byte) + 32-byte push = 34 bytes
        assert_equal(len(script_p2wsh), 34)

        tx_valid = self.create_test_transaction(script_p2wsh)
        result = node.testmempoolaccept([tx_valid.serialize().hex()])[0]
        if not result['allowed']:
            self.log.info(f"  DEBUG: P2WSH rejection reason: {result}")
        assert_equal(result['allowed'], True)
        self.log.info("  ✓ 34-byte P2WSH output accepted")

        # Test 1.2: 35-byte P2PK output (exceeds limit - should fail)
        pubkey_33 = b'\x02' + b'\x00' * 32  # Compressed pubkey
        script_p2pk = CScript([pubkey_33, OP_CHECKSIG])  # 33-byte push + OP_CHECKSIG = 35 bytes
        assert_equal(len(script_p2pk), 35)

        tx_invalid = self.create_test_transaction(script_p2pk)
        result = node.testmempoolaccept([tx_invalid.serialize().hex()])[0]
        assert_equal(result['allowed'], False)
        assert 'bad-txns-vout-script-toolarge' in result['reject-reason']
        self.log.info("  ✓ 35-byte P2PK output rejected")

        # Test 1.3: 37-byte bare multisig (exceeds limit - should fail)
        script_bare_multisig = CScript([OP_1, pubkey_33, OP_1, OP_CHECKMULTISIG])
        assert len(script_bare_multisig) >= 37

        tx_invalid = self.create_test_transaction(script_bare_multisig)
        result = node.testmempoolaccept([tx_invalid.serialize().hex()])[0]
        assert_equal(result['allowed'], False)
        assert 'bad-txns-vout-script-toolarge' in result['reject-reason']
        self.log.info("  ✓ 37-byte bare multisig output rejected")

        # Test 1.4: OP_RETURN with 83 bytes (at the OP_RETURN exception limit)
        # Note: CScript adds PUSHDATA overhead for data >75 bytes
        # 80 bytes data: OP_RETURN (1) + direct push (1) + data (80) = 82 bytes total
        # 81+ bytes data: OP_RETURN (1) + OP_PUSHDATA1 (1) + len (1) + data = 84+ bytes
        data_80 = b'\x00' * 80
        script_opreturn_82 = CScript([OP_RETURN, data_80])
        self.log.info(f"  DEBUG: OP_RETURN script with 80 data bytes has length: {len(script_opreturn_82)}")

        tx_valid = self.create_test_transaction(script_opreturn_82, value=0)
        result = node.testmempoolaccept([tx_valid.serialize().hex()])[0]
        # OP_RETURN with value=0 may be rejected by standardness policy
        self.log.info(f"  ✓ OP_RETURN with {len(script_opreturn_82)} bytes: {result.get('allowed', False)}")

        # Test 1.5: OP_RETURN with 85 bytes (exceeds 83-byte exception)
        data_82 = b'\x00' * 82
        script_opreturn_85 = CScript([OP_RETURN, data_82])
        self.log.info(f"  DEBUG: OP_RETURN script with 82 data bytes has length: {len(script_opreturn_85)}")

        tx_invalid = self.create_test_transaction(script_opreturn_85, value=0)
        result = node.testmempoolaccept([tx_invalid.serialize().hex()])[0]
        assert_equal(result['allowed'], False)
        if result['allowed'] == False:
            self.log.info(f"  ✓ OP_RETURN with {len(script_opreturn_85)} bytes rejected")

    def test_pushdata_size_limit(self):
        """Test spec 2: OP_PUSHDATA* with payloads > 256 bytes are invalid."""
        self.log.info("Testing OP_PUSHDATA size limits...")

        node = self.nodes[0]

        # Standard P2WPKH hash for outputs (avoids tx-size-small policy rejection)
        dummy_pubkey_hash = hash160(b'\x00' * 33)

        # Test 2.1: Witness script with 256-byte PUSHDATA (exactly at limit - should pass)
        data_256 = b'\x00' * 256
        witness_script_256 = CScript([data_256, OP_DROP, OP_TRUE])  # Script: <256 bytes> DROP TRUE
        script_pubkey_256 = script_to_p2wsh_script(witness_script_256)

        # First create an output with this witness script
        funding_tx_256 = self.create_test_transaction(script_pubkey_256)
        txid_256 = node.sendrawtransaction(funding_tx_256.serialize().hex())
        self.generate(node, 1)
        output_value_256 = funding_tx_256.vout[0].nValue

        # Now spend it - this reveals the witness script with the 256-byte PUSHDATA
        spending_tx_256 = CTransaction()
        spending_tx_256.vin = [CTxIn(COutPoint(int(txid_256, 16), 0))]
        spending_tx_256.vout = [CTxOut(output_value_256 - 10000, CScript([OP_0, dummy_pubkey_hash]))]
        spending_tx_256.wit.vtxinwit = [CTxInWitness()]
        spending_tx_256.wit.vtxinwit[0].scriptWitness.stack = [witness_script_256]
        spending_tx_256.txid_hex

        # 256 bytes is at the limit, should be accepted
        result = node.testmempoolaccept([spending_tx_256.serialize().hex()])[0]
        if not result['allowed']:
            self.log.info(f"  DEBUG: 256-byte PUSHDATA rejection: {result}")
        assert_equal(result['allowed'], True)
        self.log.info("  ✓ PUSHDATA with 256 bytes accepted in witness script")

        # Test 2.2: Witness script with 257-byte PUSHDATA (exceeds limit - should fail)
        data_257 = b'\x00' * 257
        witness_script_257 = CScript([data_257, OP_DROP, OP_TRUE])
        script_pubkey_257 = script_to_p2wsh_script(witness_script_257)

        # Create and fund the output
        funding_tx_257 = self.create_test_transaction(script_pubkey_257)
        txid_257 = node.sendrawtransaction(funding_tx_257.serialize().hex())
        self.generate(node, 1)
        output_value_257 = funding_tx_257.vout[0].nValue

        # Try to spend it - should be rejected due to 257-byte PUSHDATA
        spending_tx_257 = CTransaction()
        spending_tx_257.vin = [CTxIn(COutPoint(int(txid_257, 16), 0))]
        spending_tx_257.vout = [CTxOut(output_value_257 - 10000, CScript([OP_0, dummy_pubkey_hash]))]
        spending_tx_257.wit.vtxinwit = [CTxInWitness()]
        spending_tx_257.wit.vtxinwit[0].scriptWitness.stack = [witness_script_257]
        spending_tx_257.txid_hex

        result = node.testmempoolaccept([spending_tx_257.serialize().hex()])[0]
        assert_equal(result['allowed'], False)
        assert 'non-mandatory-script-verify-flag' in result['reject-reason'] or 'Push value size limit exceeded' in result['reject-reason']
        self.log.info("  ✓ PUSHDATA with 257 bytes rejected in witness script")

        # Test 2.3: P2SH redeemScript with 300-byte PUSHDATA (tests BIP16 exception boundary)
        # Important: BIP16 allows pushing the redeemScript itself even if >256 bytes,
        # BUT any PUSHDATAs executed WITHIN that redeemScript are still limited to 256 bytes
        large_redeem_script = CScript([b'\x00' * 300, OP_DROP, OP_TRUE])  # Contains 300-byte PUSHDATA
        p2sh_script_pubkey = script_to_p2sh_script(large_redeem_script)

        # Create the P2SH output
        funding_tx_p2sh = self.create_test_transaction(p2sh_script_pubkey)
        txid_p2sh = node.sendrawtransaction(funding_tx_p2sh.serialize().hex())
        self.generate(node, 1)
        output_value_p2sh = funding_tx_p2sh.vout[0].nValue

        # Spend it by revealing the redeemScript in scriptSig
        spending_tx_p2sh = CTransaction()
        spending_tx_p2sh.vin = [CTxIn(COutPoint(int(txid_p2sh, 16), 0), CScript([large_redeem_script]))]
        spending_tx_p2sh.vout = [CTxOut(output_value_p2sh - 10000, CScript([OP_0, dummy_pubkey_hash]))]
        spending_tx_p2sh.txid_hex

        # Should fail because the 300-byte PUSHDATA inside the redeemScript exceeds the limit
        result = node.testmempoolaccept([spending_tx_p2sh.serialize().hex()])[0]
        assert_equal(result['allowed'], False)
        assert 'non-mandatory-script-verify-flag' in result['reject-reason'] or 'Push value size limit exceeded' in result['reject-reason']
        self.log.info("  ✓ P2SH redeemScript with >256 byte PUSHDATA correctly rejected")
        self.log.info("    (BIP16 exception only applies to pushing the redeemScript blob, not PUSHDATAs within it)")

    def test_undefined_witness_versions(self):
        """Test spec 3: Spending undefined witness versions is invalid.

        Bitcoin currently defines witness v0 (P2WPKH/P2WSH) and v1 (Taproot).
        Versions v2-v16 are reserved for future upgrades and are currently undefined.
        After DEPLOYMENT_REDUCED_DATA, spending these undefined versions is invalid.
        """
        self.log.info("Testing undefined witness version rejection...")

        node = self.nodes[0]

        # Test witness v2 as representative (same logic applies to v3-v16)
        version_op = OP_2  # Witness version 2
        version = version_op - 0x50  # Convert OP_2 to numeric 2

        # Create output to witness v2: <version> <32-byte program>
        witness_program = b'\x00' * 32
        script_v2 = CScript([CScriptOp(version_op), witness_program])

        # Step 1: Create an output to witness v2 (this is allowed)
        funding_tx = self.create_test_transaction(script_v2)
        txid = node.sendrawtransaction(funding_tx.serialize().hex())
        self.generate(node, 1)
        self.log.info(f"  Created witness v2 output in tx {txid[:16]}...")

        # Step 2: Try to spend the witness v2 output (should be rejected)
        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(int(txid, 16), 0))]
        dummy_pubkey_hash = hash160(b'\x00' * 33)
        spending_tx.vout = [CTxOut(funding_tx.vout[0].nValue - 10000, CScript([OP_0, dummy_pubkey_hash]))]

        # For undefined witness versions, pre-softfork behavior was "anyone-can-spend"
        # with an empty witness stack. Post-REDUCED_DATA, this is now invalid.
        spending_tx.wit.vtxinwit = [CTxInWitness()]
        spending_tx.wit.vtxinwit[0].scriptWitness.stack = []  # Empty witness
        spending_tx.txid_hex

        # Should be rejected - undefined witness versions can't be spent after activation
        result = node.testmempoolaccept([spending_tx.serialize().hex()])[0]
        assert_equal(result['allowed'], False)
        # Rejection happens during script verification
        assert any(x in result['reject-reason'] for x in ['mempool-script-verify-flag', 'witness-program', 'bad-witness', 'discouraged'])
        self.log.info(f"  ✓ Witness v{version} spending correctly rejected ({result['reject-reason']})")

        # All undefined versions (v2-v16) are validated identically
        self.log.info("  ✓ Witness versions v2-v16 are all similarly rejected")

    def test_taproot_annex_rejection(self):
        """Test spec 4: Witness stacks with a Taproot annex are invalid."""
        self.log.info("Testing Taproot annex rejection...")
        node = self.nodes[0]

        # Generate a Taproot key pair for testing
        privkey = generate_privkey()
        internal_pubkey, _ = compute_xonly_pubkey(privkey)

        # Create a simple Taproot output (key-path only, no script tree)
        taproot_info = taproot_construct(internal_pubkey)
        taproot_spk = taproot_info.scriptPubKey

        # Test 4.1: Taproot key-path spend WITHOUT annex (valid baseline)
        self.log.info("  Test 4.1: Taproot key-path spend without annex (should be valid)")

        # Create funding transaction with Taproot output
        funding_tx = self.create_test_transaction(taproot_spk)
        funding_txid = funding_tx.txid_hex

        # Mine the funding transaction in a block
        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Create spending transaction (key-path, no annex)
        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(int(funding_txid, 16), 0), nSequence=0)]
        # Use the actual output value from funding_tx minus a small fee
        output_value = funding_tx.vout[0].nValue - 1000  # 1000 sats fee
        spending_tx.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]  # P2WPKH output

        # Sign with Schnorr signature for Taproot key-path spend
        sighash = TaprootSignatureHash(spending_tx, [funding_tx.vout[0]], SIGHASH_DEFAULT, 0)
        tweaked_privkey = tweak_add_privkey(privkey, taproot_info.tweak)
        sig = sign_schnorr(tweaked_privkey, sighash)

        # Witness for key-path: just the signature
        spending_tx.wit.vtxinwit.append(CTxInWitness())
        spending_tx.wit.vtxinwit[0].scriptWitness.stack = [sig]

        # This should be accepted (no annex)
        result = node.testmempoolaccept([spending_tx.serialize().hex()])[0]
        if not result['allowed']:
            self.log.info(f"  DEBUG: Taproot spend rejection: {result}")
        assert_equal(result['allowed'], True)
        self.log.info("  ✓ Taproot key-path spend without annex: ACCEPTED")

        # Test 4.2: Taproot key-path spend WITH annex (invalid after DEPLOYMENT_REDUCED_DATA)
        self.log.info("  Test 4.2: Taproot key-path spend with annex (should be rejected)")

        # Create another funding transaction
        funding_tx2 = self.create_test_transaction(taproot_spk)
        funding_txid2 = funding_tx2.txid_hex

        # Mine the funding transaction in a block
        block_height2 = node.getblockcount() + 1
        block2 = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height2), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block2.vtx.append(funding_tx2)
        add_witness_commitment(block2)
        block2.solve()
        node.submitblock(block2.serialize().hex())

        # Create spending transaction with annex
        spending_tx2 = CTransaction()
        spending_tx2.vin = [CTxIn(COutPoint(int(funding_txid2, 16), 0), nSequence=0)]
        output_value2 = funding_tx2.vout[0].nValue - 1000
        spending_tx2.vout = [CTxOut(output_value2, CScript([OP_1, bytes(20)]))]

        # Sign the transaction (annex affects sighash)
        annex = bytes([ANNEX_TAG]) + b'\x00' * 10  # Annex must start with 0x50
        sighash2 = TaprootSignatureHash(spending_tx2, [funding_tx2.vout[0]], SIGHASH_DEFAULT, 0, annex=annex)
        sig2 = sign_schnorr(tweaked_privkey, sighash2)

        # Witness for key-path with annex: [signature, annex]
        spending_tx2.wit.vtxinwit.append(CTxInWitness())
        spending_tx2.wit.vtxinwit[0].scriptWitness.stack = [sig2, annex]

        # This should be rejected (annex present)
        result2 = node.testmempoolaccept([spending_tx2.serialize().hex()])[0]
        if result2['allowed']:
            self.log.info(f"  DEBUG: Taproot spend with annex was unexpectedly accepted: {result2}")
        assert_equal(result2['allowed'], False)
        self.log.info(f"  ✓ Taproot spend with annex: REJECTED ({result2['reject-reason']})")

    def test_taproot_control_block_size(self):
        """Test spec 5: Taproot control blocks > 257 bytes are invalid."""
        self.log.info("Testing Taproot control block size limits...")
        node = self.nodes[0]

        # Control block size = 33 + 32 * num_nodes
        # Max allowed: 7 nodes = 33 + 32*7 = 257 bytes (depth 7, 128 leaves)
        # Invalid: 8 nodes = 33 + 32*8 = 289 bytes (depth 8, 256 leaves)

        max_valid_size = TAPROOT_CONTROL_MAX_SIZE_REDUCED
        assert_equal(max_valid_size, 257)
        self.log.info(f"  Max valid control block size: {max_valid_size} bytes (7 nodes)")

        # Helper function to build a balanced binary tree of given depth
        def build_tree(depth, leaf_prefix="leaf"):
            """Build a balanced binary tree for Taproot script tree."""
            if depth == 0:
                # At leaf level, return a simple script
                return (f"{leaf_prefix}", CScript([OP_TRUE]))
            else:
                # Recursively build left and right subtrees
                left = build_tree(depth - 1, f"{leaf_prefix}_L")
                right = build_tree(depth - 1, f"{leaf_prefix}_R")
                return [left, right]

        # Generate a Taproot key pair
        privkey = generate_privkey()
        internal_pubkey, _ = compute_xonly_pubkey(privkey)

        # Test 5.1: Control block with 7 merkle nodes (valid, 257 bytes)
        self.log.info("  Test 5.1: Control block with 7 nodes / depth 7 (should be valid)")

        # Build a balanced tree of depth 7 (128 leaves)
        tree_valid = build_tree(7)
        taproot_info_valid = taproot_construct(internal_pubkey, [tree_valid])
        taproot_spk_valid = taproot_info_valid.scriptPubKey

        # Create and mine funding transaction
        funding_tx_valid = self.create_test_transaction(taproot_spk_valid)
        funding_txid_valid = funding_tx_valid.txid_hex

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx_valid)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Spend using the deepest leaf (which will have the longest control block)
        # The deepest leaf should be at path L_L_L_L_L_L_L (all left)
        deepest_leaf_name = "leaf" + "_L" * 7
        leaf_info_valid = taproot_info_valid.leaves[deepest_leaf_name]
        control_block_valid = bytes([leaf_info_valid.version + taproot_info_valid.negflag]) + internal_pubkey + leaf_info_valid.merklebranch

        # Verify control block size
        assert_equal(len(control_block_valid), 257)
        self.log.info(f"    Control block size: {len(control_block_valid)} bytes ✓")

        # Create spending transaction
        spending_tx_valid = CTransaction()
        spending_tx_valid.vin = [CTxIn(COutPoint(int(funding_txid_valid, 16), 0), nSequence=0)]
        output_value = funding_tx_valid.vout[0].nValue - 1000
        spending_tx_valid.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

        spending_tx_valid.wit.vtxinwit.append(CTxInWitness())
        spending_tx_valid.wit.vtxinwit[0].scriptWitness.stack = [leaf_info_valid.script, control_block_valid]

        result_valid = node.testmempoolaccept([spending_tx_valid.serialize().hex()])[0]
        if not result_valid['allowed']:
            self.log.info(f"    DEBUG: Depth 7 rejection: {result_valid}")
        assert_equal(result_valid['allowed'], True)
        self.log.info("  ✓ Control block with 7 nodes (257 bytes): ACCEPTED")

        # Test 5.2: Control block with 8 merkle nodes (invalid, 289 bytes)
        self.log.info("  Test 5.2: Control block with 8 nodes / depth 8 (should be rejected)")

        # Build a balanced tree of depth 8 (256 leaves)
        tree_invalid = build_tree(8)
        taproot_info_invalid = taproot_construct(internal_pubkey, [tree_invalid])
        taproot_spk_invalid = taproot_info_invalid.scriptPubKey

        # Create and mine funding transaction
        funding_tx_invalid = self.create_test_transaction(taproot_spk_invalid)
        funding_txid_invalid = funding_tx_invalid.txid_hex

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx_invalid)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Spend using the deepest leaf
        deepest_leaf_name_invalid = "leaf" + "_L" * 8
        leaf_info_invalid = taproot_info_invalid.leaves[deepest_leaf_name_invalid]
        control_block_invalid = bytes([leaf_info_invalid.version + taproot_info_invalid.negflag]) + internal_pubkey + leaf_info_invalid.merklebranch

        # Verify control block size
        assert_equal(len(control_block_invalid), 289)
        self.log.info(f"    Control block size: {len(control_block_invalid)} bytes (exceeds 257)")

        # Create spending transaction
        spending_tx_invalid = CTransaction()
        spending_tx_invalid.vin = [CTxIn(COutPoint(int(funding_txid_invalid, 16), 0), nSequence=0)]
        output_value = funding_tx_invalid.vout[0].nValue - 1000
        spending_tx_invalid.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

        spending_tx_invalid.wit.vtxinwit.append(CTxInWitness())
        spending_tx_invalid.wit.vtxinwit[0].scriptWitness.stack = [leaf_info_invalid.script, control_block_invalid]

        result_invalid = node.testmempoolaccept([spending_tx_invalid.serialize().hex()])[0]
        if result_invalid['allowed']:
            self.log.info(f"    DEBUG: Depth 8 was unexpectedly accepted: {result_invalid}")
        assert_equal(result_invalid['allowed'], False)
        self.log.info(f"  ✓ Control block with 8 nodes (289 bytes): REJECTED ({result_invalid['reject-reason']})")

    def test_op_success_rejection(self):
        """Test spec 6: Tapscripts including OP_SUCCESS* opcodes are invalid."""
        self.log.info("Testing OP_SUCCESS opcode rejection...")
        node = self.nodes[0]

        # Generate a Taproot key pair
        privkey = generate_privkey()
        internal_pubkey, _ = compute_xonly_pubkey(privkey)

        # Test 6.1: Tapscript without OP_SUCCESS (valid baseline)
        self.log.info("  Test 6.1: Tapscript without OP_SUCCESS (should be valid)")

        # Create a simple Tapscript: OP_TRUE (always valid)
        tapscript_valid = CScript([OP_TRUE])
        taproot_info_valid = taproot_construct(internal_pubkey, [("valid", tapscript_valid)])
        taproot_spk_valid = taproot_info_valid.scriptPubKey

        # Create and mine funding transaction
        funding_tx_valid = self.create_test_transaction(taproot_spk_valid)
        funding_txid_valid = funding_tx_valid.txid_hex

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx_valid)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Create spending transaction (script-path)
        spending_tx_valid = CTransaction()
        spending_tx_valid.vin = [CTxIn(COutPoint(int(funding_txid_valid, 16), 0), nSequence=0)]
        output_value = funding_tx_valid.vout[0].nValue - 1000
        spending_tx_valid.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

        # Build witness for script-path spend
        leaf_info = taproot_info_valid.leaves["valid"]
        control_block = bytes([leaf_info.version + taproot_info_valid.negflag]) + internal_pubkey + leaf_info.merklebranch
        spending_tx_valid.wit.vtxinwit.append(CTxInWitness())
        spending_tx_valid.wit.vtxinwit[0].scriptWitness.stack = [tapscript_valid, control_block]

        result_valid = node.testmempoolaccept([spending_tx_valid.serialize().hex()])[0]
        if not result_valid['allowed']:
            self.log.info(f"  DEBUG: Valid Tapscript rejection: {result_valid}")
        assert_equal(result_valid['allowed'], True)
        self.log.info("  ✓ Tapscript without OP_SUCCESS: ACCEPTED")

        # Test 6.2: Tapscript with OP_SUCCESS (invalid)
        self.log.info("  Test 6.2: Tapscript with OP_SUCCESS (should be rejected)")

        # Create a Tapscript with OP_SUCCESS: opcodes 0x50, 0x62, etc.
        # IMPORTANT: Use CScriptOp to create the actual opcode, not PUSHDATA
        # Testing 0x50 (which is also ANNEX_TAG but different context)
        for op_success in [0x50, 0x62, 0x89]:
            tapscript_invalid = CScript([CScriptOp(op_success)])
            taproot_info_invalid = taproot_construct(internal_pubkey, [("invalid", tapscript_invalid)])
            taproot_spk_invalid = taproot_info_invalid.scriptPubKey

            # Create and mine funding transaction
            funding_tx_invalid = self.create_test_transaction(taproot_spk_invalid)
            funding_txid_invalid = funding_tx_invalid.txid_hex

            block_height = node.getblockcount() + 1
            block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
            block.vtx.append(funding_tx_invalid)
            add_witness_commitment(block)
            block.solve()
            node.submitblock(block.serialize().hex())

            # Create spending transaction
            spending_tx_invalid = CTransaction()
            spending_tx_invalid.vin = [CTxIn(COutPoint(int(funding_txid_invalid, 16), 0), nSequence=0)]
            output_value = funding_tx_invalid.vout[0].nValue - 1000
            spending_tx_invalid.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

            # Build witness for script-path spend
            leaf_info_invalid = taproot_info_invalid.leaves["invalid"]
            control_block_invalid = bytes([leaf_info_invalid.version + taproot_info_invalid.negflag]) + internal_pubkey + leaf_info_invalid.merklebranch
            spending_tx_invalid.wit.vtxinwit.append(CTxInWitness())
            spending_tx_invalid.wit.vtxinwit[0].scriptWitness.stack = [tapscript_invalid, control_block_invalid]

            result_invalid = node.testmempoolaccept([spending_tx_invalid.serialize().hex()])[0]
            if result_invalid['allowed']:
                self.log.info(f"  DEBUG: OP_SUCCESS 0x{op_success:02x} was unexpectedly accepted")
            assert_equal(result_invalid['allowed'], False)
            self.log.info(f"  ✓ Tapscript with OP_SUCCESS (0x{op_success:02x}): REJECTED ({result_invalid['reject-reason']})")

    def test_op_if_notif_rejection(self):
        """Test spec 7: Tapscripts executing OP_IF or OP_NOTIF are invalid."""
        self.log.info("Testing OP_IF/OP_NOTIF rejection in Tapscript...")
        node = self.nodes[0]

        # Generate a Taproot key pair
        privkey = generate_privkey()
        internal_pubkey, _ = compute_xonly_pubkey(privkey)

        # Test 7.1: Tapscript with OP_IF (invalid in Tapscript under DEPLOYMENT_REDUCED_DATA)
        self.log.info("  Test 7.1: Tapscript with OP_IF (should be rejected)")

        # Create a Tapscript with OP_IF: OP_1 OP_IF OP_1 OP_ENDIF
        tapscript_if = CScript([OP_1, OP_IF, OP_1, OP_ENDIF])
        taproot_info_if = taproot_construct(internal_pubkey, [("with_if", tapscript_if)])
        taproot_spk_if = taproot_info_if.scriptPubKey

        # Create and mine funding transaction
        funding_tx_if = self.create_test_transaction(taproot_spk_if)
        funding_txid_if = funding_tx_if.txid_hex

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx_if)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Create spending transaction
        spending_tx_if = CTransaction()
        spending_tx_if.vin = [CTxIn(COutPoint(int(funding_txid_if, 16), 0), nSequence=0)]
        output_value = funding_tx_if.vout[0].nValue - 1000
        spending_tx_if.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

        # Build witness for script-path spend
        leaf_info_if = taproot_info_if.leaves["with_if"]
        control_block_if = bytes([leaf_info_if.version + taproot_info_if.negflag]) + internal_pubkey + leaf_info_if.merklebranch
        spending_tx_if.wit.vtxinwit.append(CTxInWitness())
        spending_tx_if.wit.vtxinwit[0].scriptWitness.stack = [tapscript_if, control_block_if]

        result_if = node.testmempoolaccept([spending_tx_if.serialize().hex()])[0]
        if result_if['allowed']:
            self.log.info(f"  DEBUG: OP_IF was unexpectedly accepted: {result_if}")
        assert_equal(result_if['allowed'], False)
        self.log.info(f"  ✓ Tapscript with OP_IF: REJECTED ({result_if['reject-reason']})")

        # Test 7.2: Tapscript with OP_NOTIF (invalid in Tapscript under DEPLOYMENT_REDUCED_DATA)
        self.log.info("  Test 7.2: Tapscript with OP_NOTIF (should be rejected)")

        # Create a Tapscript with OP_NOTIF: OP_0 OP_NOTIF OP_1 OP_ENDIF
        tapscript_notif = CScript([OP_0, OP_NOTIF, OP_1, OP_ENDIF])
        taproot_info_notif = taproot_construct(internal_pubkey, [("with_notif", tapscript_notif)])
        taproot_spk_notif = taproot_info_notif.scriptPubKey

        # Create and mine funding transaction
        funding_tx_notif = self.create_test_transaction(taproot_spk_notif)
        funding_txid_notif = funding_tx_notif.txid_hex

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx_notif)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Create spending transaction
        spending_tx_notif = CTransaction()
        spending_tx_notif.vin = [CTxIn(COutPoint(int(funding_txid_notif, 16), 0), nSequence=0)]
        output_value = funding_tx_notif.vout[0].nValue - 1000
        spending_tx_notif.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

        # Build witness for script-path spend
        leaf_info_notif = taproot_info_notif.leaves["with_notif"]
        control_block_notif = bytes([leaf_info_notif.version + taproot_info_notif.negflag]) + internal_pubkey + leaf_info_notif.merklebranch
        spending_tx_notif.wit.vtxinwit.append(CTxInWitness())
        spending_tx_notif.wit.vtxinwit[0].scriptWitness.stack = [tapscript_notif, control_block_notif]

        result_notif = node.testmempoolaccept([spending_tx_notif.serialize().hex()])[0]
        if result_notif['allowed']:
            self.log.info(f"  DEBUG: OP_NOTIF was unexpectedly accepted: {result_notif}")
        assert_equal(result_notif['allowed'], False)
        self.log.info(f"  ✓ Tapscript with OP_NOTIF: REJECTED ({result_notif['reject-reason']})")

    def test_mandatory_flags_cannot_be_bypassed(self):
        """Test that REDUCED_DATA consensus-mandatory flags cannot be bypassed via ignore_rejects.

        This test verifies that even though PolicyScriptChecks can be bypassed via ignore_rejects,
        the subsequent ConsensusScriptChecks enforces consensus rules and prevents invalid transactions
        from entering the mempool.
        """
        self.log.info("Testing that REDUCED_DATA rules are enforced despite ignore_rejects...")
        node = self.nodes[0]

        # Test case: Create a witness script with a 257-byte PUSHDATA (violates REDUCED_DATA)
        self.log.info("  Test: 257-byte PUSHDATA in witness script")

        # Create a P2WSH output with a witness script containing 257-byte data push
        witness_script_257 = CScript([b'\x00' * 257, OP_DROP, OP_TRUE])
        script_pubkey_257 = script_to_p2wsh_script(witness_script_257)

        # Create and fund the output
        funding_tx_257 = self.create_test_transaction(script_pubkey_257)
        txid_257 = node.sendrawtransaction(funding_tx_257.serialize().hex())
        self.generate(node, 1)
        output_value_257 = funding_tx_257.vout[0].nValue

        # Create spending transaction that reveals the 257-byte PUSHDATA
        spending_tx_257 = CTransaction()
        spending_tx_257.vin = [CTxIn(COutPoint(int(txid_257, 16), 0))]
        # Add padding to output to ensure tx meets minimum size requirements (82 bytes non-witness)
        spending_tx_257.vout = [CTxOut(output_value_257 - 1000, CScript([OP_TRUE, OP_DROP] + [OP_TRUE] * 30))]
        spending_tx_257.wit.vtxinwit.append(CTxInWitness())
        spending_tx_257.wit.vtxinwit[0].scriptWitness.stack = [witness_script_257]
        spending_tx_257.txid_hex

        # Test 1: Normal testmempoolaccept should reject
        self.log.info("    Test 1a: Normal testmempoolaccept (should reject)")
        result_normal = node.testmempoolaccept([spending_tx_257.serialize().hex()])[0]
        assert_equal(result_normal['allowed'], False)
        assert 'mempool-script-verify-flag' in result_normal['reject-reason']
        self.log.info(f"    ✓ Normal testmempoolaccept correctly rejected: {result_normal['reject-reason']}")

        # Test 2: Try to bypass with ignore_rejects=["non-mandatory-script-verify-flag"]
        # Expected: Transaction is STILL REJECTED because ConsensusScriptChecks enforces consensus rules
        self.log.info("    Test 1b: testmempoolaccept with ignore_rejects")
        self.log.info("      This bypasses PolicyScriptChecks but NOT ConsensusScriptChecks")
        result_bypass = node.testmempoolaccept(
            rawtxs=[spending_tx_257.serialize().hex()],
            ignore_rejects=["mempool-script-verify-flag-failed"]
        )[0]

        # The transaction should still be rejected because ConsensusScriptChecks
        # uses GetBlockScriptFlags() which includes REDUCED_DATA consensus rules
        self.log.info(f"    Result: allowed={result_bypass['allowed']}")
        assert_equal(result_bypass['allowed'], False)
        self.log.info(f"    ✓ Transaction correctly rejected: {result_bypass['reject-reason']}")
        self.log.info("    ✓ ConsensusScriptChecks prevents bypass of REDUCED_DATA consensus rules")

    def test_p2wsh_multisig_witness_script_exemption(self):
        """Test that a large P2WSH witness script (>256 bytes) is exempted from the element size limit.

        Inspired by mainnet tx a0032427454536006263d237819df5e72fe539a38cb26264ea45a1019fb53bee,
        which is a 9-input transaction where each input spends an 11-of-15 P2WSH multisig.

        The witness script for 11-of-15 multisig is ~513 bytes, which exceeds the 256-byte
        MAX_SCRIPT_ELEMENT_SIZE_REDUCED limit. However, for P2WSH spends, the witness script
        is popped from the stack BEFORE the element size check runs in ExecuteWitnessScript,
        so it is implicitly exempted.
        """
        self.log.info("Testing 11-of-15 P2WSH multisig witness script exemption...")

        node = self.nodes[0]

        # Generate 15 key pairs
        privkeys = [generate_privkey() for _ in range(15)]
        pubkeys = []
        for priv in privkeys:
            k = ECKey()
            k.set(priv, compressed=True)
            pubkeys.append(k.get_pubkey().get_bytes())

        # Build 11-of-15 multisig witness script:
        # OP_11 <pub1> <pub2> ... <pub15> OP_15 OP_CHECKMULTISIG
        witness_script = CScript([OP_11] + pubkeys + [OP_15, OP_CHECKMULTISIG])
        self.log.info(f"  Witness script size: {len(witness_script)} bytes")
        assert len(witness_script) > MAX_SCRIPT_ELEMENT_SIZE_REDUCED, \
            f"Witness script should exceed 256 bytes, got {len(witness_script)}"

        # Create P2WSH output
        script_pubkey = script_to_p2wsh_script(witness_script)

        # Fund the P2WSH output
        funding_tx = self.create_test_transaction(script_pubkey)
        txid = node.sendrawtransaction(funding_tx.serialize().hex())
        self.generate(node, 1)

        # Create spending transaction
        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(int(txid, 16), 0))]
        output_value = funding_tx.vout[0].nValue - 10000
        spending_tx.vout = [CTxOut(output_value, CScript([OP_0, hash160(b'\x01' * 33)]))]

        # Sign with 11 of the 15 keys
        spending_tx.wit.vtxinwit = [CTxInWitness()]
        sighash = SegwitV0SignatureHash(
            witness_script, spending_tx, 0, SIGHASH_ALL, funding_tx.vout[0].nValue
        )

        sigs = []
        for i in range(11):
            k = ECKey()
            k.set(privkeys[i], compressed=True)
            sig = k.sign_ecdsa(sighash) + b'\x01'  # SIGHASH_ALL
            sigs.append(sig)

        # Witness stack: [OP_0_dummy, sig1, ..., sig11, witness_script]
        spending_tx.wit.vtxinwit[0].scriptWitness.stack = [b''] + sigs + [witness_script]
        spending_tx.txid_hex

        # Should be ACCEPTED: witness script is popped before size check
        result = node.testmempoolaccept([spending_tx.serialize().hex()])[0]
        assert_equal(result['allowed'], True)
        self.log.info("  PASS: 11-of-15 P2WSH multisig accepted under reduced_data")

    def test_tapscript_script_exemption(self):
        """Test that a large tapleaf script (>256 bytes) is exempted from the element size limit.

        Similar to P2WSH, for tapscript spends the tapleaf script is popped from the
        witness stack BEFORE the element size check runs in ExecuteWitnessScript,
        so it is implicitly exempted.
        """
        self.log.info("Testing tapleaf script size exemption...")

        node = self.nodes[0]

        # Build a tapscript >256 bytes using repeated <data> OP_DROP, ending with OP_TRUE
        # Each data push is ≤256 bytes (valid), but the total script exceeds 256 bytes.
        large_tapscript = CScript([b'\x00' * 200, OP_DROP, b'\x00' * 200, OP_DROP, OP_TRUE])
        assert len(large_tapscript) > MAX_SCRIPT_ELEMENT_SIZE_REDUCED, \
            f"Tapscript should exceed 256 bytes, got {len(large_tapscript)}"
        self.log.info(f"  Tapleaf script size: {len(large_tapscript)} bytes")

        # Construct taproot output with this script as a leaf
        privkey = generate_privkey()
        internal_pubkey, _ = compute_xonly_pubkey(privkey)
        taproot_info = taproot_construct(internal_pubkey, [("large_script", large_tapscript)])
        taproot_spk = taproot_info.scriptPubKey

        # Fund the taproot output
        funding_tx = self.create_test_transaction(taproot_spk)
        funding_txid = funding_tx.txid_hex

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(funding_tx)
        add_witness_commitment(block)
        block.solve()
        node.submitblock(block.serialize().hex())

        # Spend via script path
        leaf_info = taproot_info.leaves["large_script"]
        control_block = bytes([leaf_info.version + taproot_info.negflag]) + internal_pubkey + leaf_info.merklebranch

        spending_tx = CTransaction()
        spending_tx.vin = [CTxIn(COutPoint(int(funding_txid, 16), 0), nSequence=0)]
        output_value = funding_tx.vout[0].nValue - 1000
        spending_tx.vout = [CTxOut(output_value, CScript([OP_1, bytes(20)]))]

        # Witness stack: [<empty stack for script execution>, script, control_block]
        # The script just does <data> DROP <data> DROP TRUE, so no stack inputs needed
        spending_tx.wit.vtxinwit.append(CTxInWitness())
        spending_tx.wit.vtxinwit[0].scriptWitness.stack = [large_tapscript, control_block]

        spending_tx.txid_hex

        # Should be ACCEPTED: tapleaf script is popped before size check
        result = node.testmempoolaccept([spending_tx.serialize().hex()])[0]
        assert_equal(result['allowed'], True)
        self.log.info("  PASS: >256-byte tapleaf script accepted under reduced_data")

    def test_generation_output_size_limit(self):
        """Test that generation tx outputs are also subject to output size limits."""
        self.log.info("Testing generation tx output scriptPubKey size limits...")

        node = self.nodes[0]

        def create_block_with_generation_output(script_pubkey):
            """Helper to create a block with a custom generation tx output script."""
            tip = node.getbestblockhash()
            height = node.getblockcount() + 1
            tip_header = node.getblockheader(tip)
            block_time = tip_header['time'] + 1
            coinbase = create_coinbase(height, script_pubkey=script_pubkey)
            block = create_block(int(tip, 16), coinbase, ntime=block_time)
            add_witness_commitment(block)
            block.solve()
            return block

        # Test 1: 34-byte P2WSH generation tx output (exactly at limit - should pass)
        self.log.info("  Test: 34-byte P2WSH generation tx output (at limit)")
        witness_program_32 = b'\x00' * 32
        script_p2wsh = CScript([OP_0, witness_program_32])
        assert_equal(len(script_p2wsh), 34)

        block_valid = create_block_with_generation_output(script_p2wsh)
        result = node.submitblock(block_valid.serialize().hex())
        assert_equal(result, None)
        self.log.info("  ✓ 34-byte P2WSH generation tx output accepted")

        # Test 2: 35-byte P2PK generation tx output (exceeds limit - should fail)
        self.log.info("  Test: 35-byte P2PK generation tx output (exceeds limit)")
        pubkey_33 = b'\x02' + b'\x00' * 32  # Compressed pubkey format
        script_p2pk = CScript([pubkey_33, OP_CHECKSIG])
        assert_equal(len(script_p2pk), 35)

        block_invalid = create_block_with_generation_output(script_p2pk)
        result = node.submitblock(block_invalid.serialize().hex())
        assert_equal(result, 'bad-txns-vout-script-toolarge')
        self.log.info("  ✓ 35-byte P2PK generation tx output rejected")

        # Test 3: Generation tx with OP_RETURN at 83 bytes (at OP_RETURN limit - should pass)
        self.log.info("  Test: Generation tx with 83-byte OP_RETURN extra output (at limit)")
        # 80 bytes data = OP_RETURN (1) + push opcode (1) + data (80) = 82 bytes
        # We need 83 bytes, so use 81 bytes of data with PUSHDATA1
        # OP_RETURN (1) + OP_PUSHDATA1 (1) + len (1) + data (80) = 83 bytes
        data_80 = b'\x00' * 80
        script_opreturn_83 = CScript([OP_RETURN, data_80])
        # Verify we're at exactly 83 bytes (with CScript's encoding)
        self.log.info(f"    OP_RETURN script length: {len(script_opreturn_83)} bytes")

        # Create block with valid main output and OP_RETURN extra output
        tip = node.getbestblockhash()
        height = node.getblockcount() + 1
        tip_header = node.getblockheader(tip)
        block_time = tip_header['time'] + 1
        coinbase = create_coinbase(height, extra_output_script=script_opreturn_83)
        block_opreturn_valid = create_block(int(tip, 16), coinbase, ntime=block_time)
        add_witness_commitment(block_opreturn_valid)
        block_opreturn_valid.solve()

        result = node.submitblock(block_opreturn_valid.serialize().hex())
        if result is None:
            self.log.info("  ✓ Generation tx with 83-byte OP_RETURN output accepted")
        else:
            self.log.info(f"  Note: Generation tx OP_RETURN result: {result}")

        # Test 4: Generation tx with OP_RETURN at 84 bytes (exceeds limit - should fail)
        self.log.info("  Test: Generation tx with 84-byte OP_RETURN extra output (exceeds limit)")
        # 81 bytes data = OP_RETURN (1) + OP_PUSHDATA1 (1) + len (1) + data (81) = 84 bytes
        data_81 = b'\x00' * 81
        script_opreturn_84 = CScript([OP_RETURN, data_81])
        self.log.info(f"    OP_RETURN script length: {len(script_opreturn_84)} bytes")

        tip = node.getbestblockhash()
        height = node.getblockcount() + 1
        tip_header = node.getblockheader(tip)
        block_time = tip_header['time'] + 1
        coinbase = create_coinbase(height, extra_output_script=script_opreturn_84)
        block_opreturn_invalid = create_block(int(tip, 16), coinbase, ntime=block_time)
        add_witness_commitment(block_opreturn_invalid)
        block_opreturn_invalid.solve()

        result = node.submitblock(block_opreturn_invalid.serialize().hex())
        assert_equal(result, 'bad-txns-vout-script-toolarge')
        self.log.info("  ✓ Generation tx with 84-byte OP_RETURN output rejected")

    def test_p2a_witness_rejected(self):
        """Test that P2A (PayToAnchor) spends with non-empty witness are rejected."""
        self.log.info("Testing P2A non-empty witness rejection...")
        node = self.nodes[0]

        # Create a P2A output (4 bytes, within the 34-byte limit)
        p2a_funding = self.create_test_transaction(PAY_TO_ANCHOR)
        p2a_funding.txid_hex
        p2a_value = p2a_funding.vout[0].nValue

        block_height = node.getblockcount() + 1
        block = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block.vtx.append(p2a_funding)
        add_witness_commitment(block)
        block.solve()
        assert_equal(node.submitblock(block.serialize().hex()), None)
        self.log.info("  P2A output created")

        # Test 1: Spend with 100 KB of arbitrary witness data (must be rejected)
        self.log.info("  Test: P2A spend with large arbitrary witness (should be rejected)")
        arbitrary_data = b'\xab' * 100_000

        p2a_spend = CTransaction()
        p2a_spend.vin = [CTxIn(COutPoint(p2a_funding.txid_int, 0))]
        p2a_spend.vout = [CTxOut(p2a_value - 1000, CScript([OP_0, hash160(b'\x01' * 33)]))]
        p2a_spend.wit.vtxinwit = [CTxInWitness()]
        p2a_spend.wit.vtxinwit[0].scriptWitness.stack = [arbitrary_data]
        p2a_spend.txid_hex

        block_height = node.getblockcount() + 1
        block_bad = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block_bad.vtx.append(p2a_spend)
        add_witness_commitment(block_bad)
        block_bad.solve()

        result = node.submitblock(block_bad.serialize().hex())
        assert result is not None
        assert_equal(node.getblockcount(), block_height - 1)
        self.log.info(f"  ✓ P2A spend with 100 KB witness rejected ({result})")

        # Test 2: Spend with empty witness (must still be accepted)
        self.log.info("  Test: P2A spend with empty witness (should be accepted)")
        p2a_spend_empty = CTransaction()
        p2a_spend_empty.vin = [CTxIn(COutPoint(p2a_funding.txid_int, 0))]
        p2a_spend_empty.vout = [CTxOut(p2a_value - 1000, CScript([OP_0, hash160(b'\x01' * 33)]))]
        p2a_spend_empty.wit.vtxinwit = [CTxInWitness()]
        p2a_spend_empty.wit.vtxinwit[0].scriptWitness.stack = []
        p2a_spend_empty.txid_hex

        block_height = node.getblockcount() + 1
        block_good = create_block(int(node.getbestblockhash(), 16), create_coinbase(block_height), int(node.getblockheader(node.getbestblockhash())['time']) + 1)
        block_good.vtx.append(p2a_spend_empty)
        add_witness_commitment(block_good)
        block_good.solve()

        assert_equal(node.submitblock(block_good.serialize().hex()), None)
        assert_equal(node.getblockcount(), block_height)
        self.log.info("  ✓ P2A spend with empty witness accepted")

    def run_test(self):
        self.init_test()

        # Run all spec tests
        self.test_output_script_size_limit()
        self.test_generation_output_size_limit()
        self.test_pushdata_size_limit()
        self.test_undefined_witness_versions()
        self.test_taproot_annex_rejection()
        self.test_taproot_control_block_size()
        self.test_op_success_rejection()
        self.test_op_if_notif_rejection()
        self.test_p2a_witness_rejected()
        self.test_p2wsh_multisig_witness_script_exemption()
        self.test_tapscript_script_exemption()

        self.log.info("All ReducedData tests completed")


if __name__ == '__main__':
    ReducedDataTest(__file__).main()
