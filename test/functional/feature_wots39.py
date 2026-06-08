#!/usr/bin/env python3
# Copyright (c) 2026 The WOTS-39 Authors
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
WOTS-39 Post-Quantum Bitcoin Integration Test -- P2WOTS (Witness Version 3)

DESIGN OVERVIEW
---------------
P2WOTS is a purely post-quantum output type with no elliptic-curve material:

  scriptPubKey = OP_3 (0x53) || PUSH32 (0x20) || address_commitment[32]

  address_commitment = Merkle root over 64 independent WOTS+ slot keys.
  Each slot is a fresh one-time WOTS key; each spend uses exactly one slot.
  Address reuse and RBF are safe: each signing operation uses a distinct slot.

Key derivation:
  address_nonce  = SHA256("wots39-addr-v1" || mskWOTS || uint64_be(address_index))
  slot_nonce[i]  = SHA256("wots39-slot-v1" || mskWOTS || address_nonce || uint64_be(i))
  wots_pk[i]     = SHA256(chain_tip[0] || ... || chain_tip[33])
  leaf[i]        = SHA256("wots39-leaf-v1" || wots_pk[i])
  root           = Merkle root over leaf[0..63]

Witness (42 items for single-sig):
  stack[0..33]  -- 34 WOTS+ sig elements (32 bytes each)
  stack[34]     -- slot_nonce  (32 bytes)
  stack[35]     -- key_index   (1 byte, 0-63)
  stack[36..41] -- auth_path   (6 x 32 bytes, leaf-to-root siblings)

Multi-sig witness (1 + n + k*43 items):
  stack[0]       -- {k, n} header (2 bytes)
  stack[1..n]    -- n Merkle roots (n x 32 bytes)
  Per signer p:
    signer_index (1B) + 34 sig elements + slot_nonce + key_index + 6 auth_path

TESTS
-----
Pure Python (no node required):
  test_address_slot_nonce_derivation   -- nonce uniqueness and determinism
  test_merkle_key_tree                 -- Merkle root construction and auth paths
  test_base_w_encode_w256              -- w=256 encoding and checksum math
  test_sign_verify_roundtrip           -- Python sign/verify self-check with
                                          element corruption detection
  test_multisig_commitment             -- k-of-n commitment derivation

On-chain (requires patched regtest node with SCRIPT_VERIFY_P2WOTS active):
  test_p2wots_fund_and_spend           -- E2E: fund P2WOTS, spend with slot 0
  test_spend_slot_boundaries           -- slot 0 and slot 63 boundary cases
  test_address_reuse_multiple_slots    -- same address, two UTXOs, two slots
  test_invalid_sig_rejected            -- corrupt sig element -> node rejects
  test_wrong_slot_nonce_rejected       -- wrong slot_nonce in witness -> rejected
  test_wrong_key_index_rejected        -- key_index mismatch -> rejected
  test_wrong_auth_path_rejected        -- bad auth_path node -> rejected
  test_witness_item_count_rejected     -- 41 or 43 items (not 42) -> rejected
  test_key_index_out_of_range          -- key_index >= 64 -> rejected
  test_double_spend_rejected           -- spend same UTXO twice -> rejected
  test_multisig_2of2                   -- fund and spend a 2-of-2 output
  test_multisig_2of3_threshold         -- any 2 of 3 keys can sign
  test_multisig_duplicate_signer       -- same signer_index used twice -> rejected
  test_multisig_wrong_commitment       -- roots don't match commitment -> rejected
  test_multiple_p2wots_inputs_one_tx   -- two P2WOTS inputs in the same spending tx
"""

import hashlib
import struct

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import (
    CTransaction, CTxIn, CTxOut, COutPoint,
    CTxInWitness, COIN,
)
from test_framework.script import CScript
from test_framework.wots39 import (
    # Constants
    WOTS_L, WOTS_TREE_HEIGHT, WOTS_TREE_SLOTS, WOTS_WITNESS_ITEMS,
    WOTS_MULTISIG_ITEMS_PER_SIGNER, WOTS_W,
    # Nonce derivation
    compute_address_nonce, compute_slot_nonce,
    # WOTS+ primitives
    compute_msg_hash, derive_sk, chain_step, chain_up,
    base_w_encode, generate_wots_pk,
    sign_wots, verify_wots, verify_from_stack_elements,
    # Merkle Key Tree
    wots_merkle_leaf, wots_merkle_node,
    compute_merkle_root, generate_auth_path, verify_auth_path,
    # Multi-sig
    compute_multisig_commitment,
    # Builders
    build_p2wots_script, build_p2wots_witness,
    build_p2wots_multisig_witness, p2wots_address,
    # Sighash
    compute_p2wots_sighash,
    # High-level helpers
    sign_p2wots_input, sign_and_build_witness,
)


class WOTS39Test(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[
            '-acceptnonstdtxn=1',
            '-txindex=1',
        ]]
        self.wallet_names = [self.default_wallet_name]
        self.uses_wallet = True

    def run_test(self):
        # ── Pure Python tests (no node needed) ────────────────────────────────
        self.log.info("=== Pure Python WOTS-39 tests ===")
        self.test_address_slot_nonce_derivation()
        self.test_merkle_key_tree()
        self.test_base_w_encode_w256()
        self.test_sign_verify_roundtrip()
        self.test_multisig_commitment()

        # ── On-chain tests (requires patched node) ─────────────────────────────
        self.log.info("=== On-chain P2WOTS tests ===")
        self.generate(self.nodes[0], 101)   # mature coins for the wallet

        self.test_p2wots_fund_and_spend()
        self.test_spend_slot_boundaries()
        self.test_address_reuse_multiple_slots()
        self.test_invalid_sig_rejected()
        self.test_wrong_slot_nonce_rejected()
        self.test_wrong_key_index_rejected()
        self.test_wrong_auth_path_rejected()
        self.test_witness_item_count_rejected()
        self.test_key_index_out_of_range()
        self.test_double_spend_rejected()
        self.test_multisig_2of2()
        self.test_multisig_2of3_threshold()
        self.test_multisig_duplicate_signer()
        self.test_multisig_wrong_commitment()
        self.test_multiple_p2wots_inputs_one_tx()

    # =========================================================================
    # PURE PYTHON TESTS
    # =========================================================================

    def test_address_slot_nonce_derivation(self):
        """
        Address nonce and slot nonce derivation:
          - Deterministic for fixed inputs
          - Unique across address_index and slot_index
          - Unique across different mskWOTS
          - Domain-isolated (address_nonce != slot_nonce for same inputs)
        """
        msk = bytes([0x55] * 32)

        an0 = compute_address_nonce(msk, 0)
        an1 = compute_address_nonce(msk, 1)
        an2 = compute_address_nonce(msk, 2)

        # Deterministic
        assert_equal(compute_address_nonce(msk, 0), an0)

        # Unique across address_index
        assert an0 != an1 and an1 != an2 and an0 != an2, \
            "address_nonce must differ across address_index"

        # Unique across mskWOTS
        msk2 = bytes([0xAA] * 32)
        assert compute_address_nonce(msk2, 0) != an0, \
            "Different mskWOTS must produce different address_nonce"

        # Slot nonces are unique within an address
        sn = [compute_slot_nonce(msk, an0, i) for i in range(WOTS_TREE_SLOTS)]
        assert len(set(sn)) == WOTS_TREE_SLOTS, \
            f"All {WOTS_TREE_SLOTS} slot nonces must be distinct"

        # Slot nonces from different addresses are distinct
        sn2 = compute_slot_nonce(msk, an1, 0)
        assert sn[0] != sn2, "Same slot_index but different address_nonce -> different slot_nonce"

        # Domain isolation: address_nonce != slot_nonce even with same structure
        assert an0 != sn[0], "address_nonce and slot_nonce must be domain-separated"

        self.log.info(f"  address_nonce[0]: {an0.hex()[:16]}...")
        self.log.info(f"  slot_nonce[0]:    {sn[0].hex()[:16]}...")
        self.log.info("test_address_slot_nonce_derivation: PASS")

    def test_merkle_key_tree(self):
        """
        Merkle Key Tree construction and authentication paths:
          - Leaves are distinct and committed to their slot PK
          - Root is deterministic and depends on all 64 slots
          - Auth path verifies for every slot (spot-check key slots)
          - Wrong slot_index or corrupted auth_path -> verify fails
          - Cross-address isolation: auth path from one address fails for another
        """
        msk = bytes([0x11] * 32)
        an0 = compute_address_nonce(msk, 0)
        an1 = compute_address_nonce(msk, 1)

        root0 = compute_merkle_root(msk, an0)
        root1 = compute_merkle_root(msk, an1)
        assert root0 != root1, "Different addresses must produce different Merkle roots"

        # Auth path verification for a selection of slots
        for slot_idx in [0, 1, 31, 32, 62, 63]:
            sn    = compute_slot_nonce(msk, an0, slot_idx)
            pk    = generate_wots_pk(msk, sn)
            apath = generate_auth_path(msk, an0, slot_idx)

            assert len(apath) == WOTS_TREE_HEIGHT, \
                f"auth_path must have exactly {WOTS_TREE_HEIGHT} nodes"

            assert verify_auth_path(pk, slot_idx, apath, root0), \
                f"auth_path must verify for slot {slot_idx}"

            # Wrong slot_index fails
            assert not verify_auth_path(pk, slot_idx ^ 1, apath, root0), \
                f"auth_path must fail with wrong slot_index"

            # Corrupt one auth_path node
            bad_path = list(apath)
            bad_path[0] = bytes(32)
            assert not verify_auth_path(pk, slot_idx, bad_path, root0), \
                "Corrupted auth_path node must fail verification"

        # Cross-address isolation
        sn_a0 = compute_slot_nonce(msk, an0, 0)
        pk_a0 = generate_wots_pk(msk, sn_a0)
        apath_a0 = generate_auth_path(msk, an0, 0)

        # pk from address 0 does not verify against root of address 1
        assert not verify_auth_path(pk_a0, 0, apath_a0, root1), \
            "auth_path from address 0 must not verify against root of address 1"

        # Leaf hash includes domain separation
        leaf = wots_merkle_leaf(pk_a0)
        assert leaf != pk_a0, "wots_merkle_leaf must differ from raw pk"

        # Internal node is symmetric-aware (left != right must produce unique result)
        sn1 = compute_slot_nonce(msk, an0, 1)
        pk1 = generate_wots_pk(msk, sn1)
        leaf1 = wots_merkle_leaf(pk1)
        assert wots_merkle_node(leaf, leaf1) != wots_merkle_node(leaf1, leaf), \
            "Merkle node must be order-sensitive (left||right != right||left)"

        self.log.info(f"  root0: {root0.hex()[:16]}...")
        self.log.info(f"  root1: {root1.hex()[:16]}...")
        self.log.info("test_merkle_key_tree: PASS")

    def test_base_w_encode_w256(self):
        """
        base_w_encode with w=256:
          - Message digits are the raw 32 bytes (each 0..255)
          - Checksum = sum(255 - d) for d in digits[0..31], max = 32*255 = 8160
          - digits[32] = high byte of checksum, digits[33] = low byte
          - Total length is always 34 (WOTS_L)
          - All-zeros msg -> checksum = 8160 = 0x1FE0
          - All-0xFF msg  -> checksum = 0
        """
        # All-zero message: max checksum = 32 * 255 = 8160 = 0x1FE0
        zero_msg = bytes(32)
        d = base_w_encode(zero_msg)
        assert len(d) == WOTS_L, f"Expected {WOTS_L} digits, got {len(d)}"
        assert all(d[i] == 0 for i in range(32)), "First 32 digits must equal msg bytes"
        assert d[32] == 0x1F and d[33] == 0xE0, \
            f"Checksum for all-zero msg must be 0x1FE0 (got {d[32]:02x}{d[33]:02x})"

        # All-0xFF message: checksum = 0
        ff_msg = bytes([0xFF] * 32)
        d = base_w_encode(ff_msg)
        assert d[32] == 0 and d[33] == 0, "Checksum for all-0xFF msg must be 0"
        assert all(d[i] == 0xFF for i in range(32)), "First 32 digits must be 0xFF"

        # Mixed message: bytes [0x00, 0x80, 0xFF, 0x40, ...]
        import hashlib
        mixed = hashlib.sha256(b"wots39-test-vector").digest()
        d = base_w_encode(mixed)
        assert len(d) == WOTS_L
        for i in range(32):
            assert d[i] == mixed[i], f"Digit[{i}] must equal msg byte {mixed[i]}"
        expected_csum = sum(255 - mixed[i] for i in range(32))
        assert d[32] == (expected_csum >> 8) & 0xFF
        assert d[33] == expected_csum & 0xFF
        assert 0 <= expected_csum <= 8160, "Checksum out of range"

        # Checksum invariant: sum of chain lengths = constant for any message
        # (sum(digits[0..31]) + sum(digits[32..33]) = 32*255 - csum + csum = constant)
        # i.e. sum(255 - d[i]) for i<32 == sum(d[32:34]) * 256^powers
        csum_encoded = d[32] * 256 + d[33]
        csum_raw = sum(255 - d[i] for i in range(32))
        assert_equal(csum_raw, csum_encoded)

        self.log.info("test_base_w_encode_w256: PASS")

    def test_sign_verify_roundtrip(self):
        """
        Python sign -> Python verify:
          - Valid signature verifies
          - Wrong msg_hash fails
          - Wrong slot_nonce fails (domain separator)
          - Wrong wots_pk fails
          - Corrupting each of the 34 sig elements individually fails
          - Stack-element form (VerifyFromStackElements) matches flat form
          - Cross-message: sig for msg1 fails to verify against msg2
        """
        msk       = bytes([0x42] * 32)
        an        = compute_address_nonce(msk, 0)
        sn        = compute_slot_nonce(msk, an, 0)
        sighash   = bytes([0xCD] * 32)
        msg_hash  = compute_msg_hash(sighash)

        pk  = generate_wots_pk(msk, sn)
        sig = sign_wots(msk, sn, msg_hash)
        assert len(sig) == 34 * 32

        assert verify_wots(pk, sn, msg_hash, sig), "Valid sig must verify"

        # Wrong message
        msg2 = compute_msg_hash(bytes([0xEE] * 32))
        assert not verify_wots(pk, sn, msg2, sig), \
            "Sig with wrong msg must fail"

        # Sig for msg1 does not verify for msg2 (and vice versa)
        sig2 = sign_wots(msk, sn, msg2)
        assert not verify_wots(pk, sn, msg_hash, sig2), \
            "sig2 must not verify for msg1"
        assert not verify_wots(pk, sn, msg2, sig), \
            "sig1 must not verify for msg2"

        # Wrong slot nonce
        sn2 = compute_slot_nonce(msk, an, 1)
        assert not verify_wots(pk, sn2, msg_hash, sig), \
            "Sig with wrong slot_nonce must fail"

        # Wrong public key
        pk2 = generate_wots_pk(msk, sn2)
        assert not verify_wots(pk2, sn, msg_hash, sig), \
            "Sig verified against wrong pk must fail"

        # Corrupt each of the 34 sig elements one at a time
        for elem_i in range(34):
            bad = bytearray(sig)
            bad[elem_i * 32] ^= 0xFF   # flip first byte of element
            assert not verify_wots(pk, sn, msg_hash, bytes(bad)), \
                f"Corrupting sig element {elem_i} must fail verification"

        # Stack-element form must agree with flat form
        sig_elems = [sig[i * 32:(i + 1) * 32] for i in range(WOTS_L)]
        assert verify_from_stack_elements(pk, sn, msg_hash, sig_elems), \
            "VerifyFromStackElements must succeed for valid sig"
        assert not verify_from_stack_elements(pk, sn, msg2, sig_elems), \
            "VerifyFromStackElements must fail for wrong msg"
        assert not verify_from_stack_elements(pk, sn, msg_hash, sig_elems + [b'\x00' * 32]), \
            "Too many stack elements must fail"
        assert not verify_from_stack_elements(pk, sn, msg_hash, sig_elems[:-1]), \
            "Too few stack elements must fail"

        self.log.info("test_sign_verify_roundtrip: PASS")

    def test_multisig_commitment(self):
        """
        k-of-n multi-sig commitment derivation:
          - Deterministic and non-zero
          - Distinct from any individual Merkle root
          - Different k -> different commitment
          - Different n -> different commitment
          - Different root order -> different commitment
        """
        msk = bytes([0x77] * 32)
        roots = []
        for i in range(3):
            an = compute_address_nonce(msk, i)
            roots.append(compute_merkle_root(msk, an))

        # 1-of-1
        c1_1 = compute_multisig_commitment(1, 1, [roots[0]])
        assert len(c1_1) == 32
        assert c1_1 != bytes(32), "Commitment must be non-zero"
        assert c1_1 != roots[0], "Commitment must differ from raw Merkle root"

        # 2-of-2
        c2_2 = compute_multisig_commitment(2, 2, roots[:2])
        assert c2_2 != c1_1, "2-of-2 must differ from 1-of-1"

        # 2-of-3
        c2_3 = compute_multisig_commitment(2, 3, roots[:3])
        assert c2_3 != c2_2, "2-of-3 must differ from 2-of-2"

        # 1-of-3 != 2-of-3 (different k)
        c1_3 = compute_multisig_commitment(1, 3, roots[:3])
        assert c1_3 != c2_3, "Different k must produce different commitment"

        # Root order matters
        c_rev = compute_multisig_commitment(2, 3, list(reversed(roots[:3])))
        assert c_rev != c2_3, "Root order must affect commitment"

        # Deterministic
        assert_equal(compute_multisig_commitment(2, 3, roots[:3]), c2_3)

        self.log.info(f"  1-of-1: {c1_1.hex()[:16]}...")
        self.log.info(f"  2-of-3: {c2_3.hex()[:16]}...")
        self.log.info("test_multisig_commitment: PASS")

    # =========================================================================
    # ON-CHAIN TESTS
    # =========================================================================

    def test_p2wots_fund_and_spend(self):
        """
        Full E2E: fund a P2WOTS output and spend it with slot 0.
        The node must accept the transaction (verifies Python sighash == C++ sighash
        and Python WOTS math == C++ WOTS math).
        """
        node = self.nodes[0]
        msk  = bytes([0x07] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.5 * COIN))
        dest_spk  = self._get_dest_spk(node)
        spend_txid = self._spend_p2wots(node, txid_int, vout, msk, an, ac, spk,
                                         int(0.5 * COIN), dest_spk, slot_index=0)
        self.generate(node, 1)
        assert spend_txid in node.getblock(node.getbestblockhash())['tx'], \
            "P2WOTS spend must confirm on-chain"
        self.log.info(f"  spend txid: {spend_txid[:16]}...")
        self.log.info("test_p2wots_fund_and_spend: PASS")

    def test_spend_slot_boundaries(self):
        """
        Spend using slot 0 (all-left auth path) and slot 63 (all-right auth path).
        Both boundary cases must verify correctly.
        """
        node = self.nodes[0]
        msk  = bytes([0x08] * 32)

        for slot_idx, label in [(0, "slot 0"), (63, "slot 63")]:
            txid_int, vout, an, ac, spk = self._fund_p2wots(
                node, msk, slot_idx, int(0.2 * COIN)
            )
            dest_spk = self._get_dest_spk(node)
            spend_txid = self._spend_p2wots(
                node, txid_int, vout, msk, an, ac, spk,
                int(0.2 * COIN), dest_spk, slot_index=slot_idx
            )
            self.generate(node, 1)
            assert spend_txid in node.getblock(node.getbestblockhash())['tx'], \
                f"P2WOTS spend must confirm for {label}"
            self.log.info(f"  {label}: spend {spend_txid[:16]}... OK")

        self.log.info("test_spend_slot_boundaries: PASS")

    def test_address_reuse_multiple_slots(self):
        """
        Same address_commitment (same msk, same address_index), two separate UTXOs.
        Each UTXO is spent using a different slot key -- one-time property preserved.
        """
        node = self.nodes[0]
        msk  = bytes([0x09] * 32)

        # Fund two UTXOs to the SAME P2WOTS address
        an = compute_address_nonce(msk, 0)
        ac = compute_merkle_root(msk, an)
        spk = build_p2wots_script(ac)

        txid_a = self._fund_output_raw(node, spk, int(0.15 * COIN))
        self.generate(node, 1)
        tx_data_a = node.getrawtransaction(txid_a, True)
        vout_a = next(i for i, o in enumerate(tx_data_a['vout'])
                      if bytes.fromhex(o['scriptPubKey']['hex']) == spk)

        txid_b = self._fund_output_raw(node, spk, int(0.12 * COIN))
        self.generate(node, 1)
        tx_data_b = node.getrawtransaction(txid_b, True)
        vout_b = next(i for i, o in enumerate(tx_data_b['vout'])
                      if bytes.fromhex(o['scriptPubKey']['hex']) == spk)

        txid_a_int = int(txid_a, 16)
        txid_b_int = int(txid_b, 16)

        # Spend UTXO-A with slot 5, UTXO-B with slot 17
        for txid_int, vout, slot_idx, amount_sat in [
            (txid_a_int, vout_a, 5,  int(0.15 * COIN)),
            (txid_b_int, vout_b, 17, int(0.12 * COIN)),
        ]:
            dest_spk = self._get_dest_spk(node)
            spend_txid = self._spend_p2wots(
                node, txid_int, vout, msk, an, ac, spk,
                amount_sat, dest_spk, slot_index=slot_idx
            )
            self.generate(node, 1)
            assert spend_txid in node.getblock(node.getbestblockhash())['tx'], \
                f"Address reuse spend (slot {slot_idx}) must confirm"
            self.log.info(f"  slot {slot_idx}: spend {spend_txid[:16]}... OK")

        self.log.info("test_address_reuse_multiple_slots: PASS")

    def test_invalid_sig_rejected(self):
        """A corrupted WOTS+ signature element must be rejected by the node."""
        node = self.nodes[0]
        msk  = bytes([0x0B] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.3 * COIN))
        dest_spk = self._get_dest_spk(node)

        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.3 * COIN),
                                                        spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn  = compute_slot_nonce(msk, an, 0)
        sig = bytearray(sign_wots(msk, sn, msg_hash))
        sig[500] ^= 0xFF  # corrupt one byte in sig element 15

        apath = generate_auth_path(msk, an, 0)
        items = build_p2wots_witness(bytes(sig), sn, 0, apath)
        self._set_witness(spend_tx, 0, items)

        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx.serialize().hex())
        self.log.info("test_invalid_sig_rejected: PASS")

    def test_wrong_slot_nonce_rejected(self):
        """
        Witness carries the slot_nonce for slot 1, but the WOTS sig was built
        for slot 0.  Chain tips will be computed under the wrong domain separator
        and the reconstructed PK will not match any Merkle leaf.
        """
        node = self.nodes[0]
        msk  = bytes([0x0C] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.3 * COIN))
        dest_spk = self._get_dest_spk(node)

        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.3 * COIN),
                                                        spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn_good  = compute_slot_nonce(msk, an, 0)
        sn_wrong = compute_slot_nonce(msk, an, 1)  # wrong slot nonce
        sig      = sign_wots(msk, sn_good, msg_hash)
        apath    = generate_auth_path(msk, an, 0)

        # Python-side sanity: PK reconstructed under wrong nonce won't match
        pk_good = generate_wots_pk(msk, sn_good)
        assert not verify_wots(pk_good, sn_wrong, msg_hash, sig)

        items = build_p2wots_witness(sig, sn_wrong, 0, apath)
        self._set_witness(spend_tx, 0, items)

        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx.serialize().hex())
        self.log.info("test_wrong_slot_nonce_rejected: PASS")

    def test_wrong_key_index_rejected(self):
        """
        key_index claims slot 5, but sig and slot_nonce are for slot 3.
        Auth path is for slot 3.  The reconstructed PK is correct for slot 3
        but VerifyAuthPath is called with key_index=5, so verification fails.
        """
        node = self.nodes[0]
        msk  = bytes([0x0D] * 32)

        # Sign with slot 3, but put key_index=5 in witness
        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.25 * COIN))
        dest_spk = self._get_dest_spk(node)

        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.25 * COIN),
                                                        spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn3   = compute_slot_nonce(msk, an, 3)
        sig   = sign_wots(msk, sn3, msg_hash)
        apath = generate_auth_path(msk, an, 3)

        # key_index=5 is wrong for this sig/path (built for slot 3)
        items = build_p2wots_witness(sig, sn3, 5, apath)
        self._set_witness(spend_tx, 0, items)

        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx.serialize().hex())
        self.log.info("test_wrong_key_index_rejected: PASS")

    def test_wrong_auth_path_rejected(self):
        """
        Auth path is for slot 10, but key_index=10 with sig/nonce for slot 10.
        We corrupt one auth_path sibling node -- root reconstruction will differ.
        """
        node = self.nodes[0]
        msk  = bytes([0x0E] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.25 * COIN))
        dest_spk = self._get_dest_spk(node)

        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.25 * COIN),
                                                        spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn    = compute_slot_nonce(msk, an, 10)
        sig   = sign_wots(msk, sn, msg_hash)
        apath = list(generate_auth_path(msk, an, 10))
        apath[3] = bytes(32)  # corrupt one sibling node

        items = build_p2wots_witness(sig, sn, 10, apath)
        self._set_witness(spend_tx, 0, items)

        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx.serialize().hex())
        self.log.info("test_wrong_auth_path_rejected: PASS")

    def test_witness_item_count_rejected(self):
        """
        42 items required.  41 (missing auth_path element) and 43 (extra element)
        both must be rejected as invalid witness format.
        """
        node = self.nodes[0]
        msk  = bytes([0x0F] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.2 * COIN))
        dest_spk = self._get_dest_spk(node)

        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.2 * COIN),
                                                        spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn    = compute_slot_nonce(msk, an, 0)
        sig   = sign_wots(msk, sn, msg_hash)
        apath = generate_auth_path(msk, an, 0)
        items = build_p2wots_witness(sig, sn, 0, apath)
        assert len(items) == WOTS_WITNESS_ITEMS  # sanity: 42

        # 41 items: remove last auth_path sibling
        spend_tx_41 = CTransaction(spend_tx)
        self._set_witness(spend_tx_41, 0, items[:-1])
        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx_41.serialize().hex())

        # 43 items: add a spurious extra item
        spend_tx_43 = CTransaction(spend_tx)
        self._set_witness(spend_tx_43, 0, items + [bytes(32)])
        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx_43.serialize().hex())

        self.log.info("test_witness_item_count_rejected: PASS")

    def test_key_index_out_of_range(self):
        """
        key_index must be 0..63.  Values 64 and 255 must be rejected.
        """
        node = self.nodes[0]
        msk  = bytes([0x10] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.2 * COIN))
        dest_spk = self._get_dest_spk(node)

        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.2 * COIN),
                                                        spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn    = compute_slot_nonce(msk, an, 0)
        sig   = sign_wots(msk, sn, msg_hash)
        apath = generate_auth_path(msk, an, 0)
        good  = build_p2wots_witness(sig, sn, 0, apath)

        for bad_idx in [64, 127, 255]:
            # Manually patch the key_index item (position 35 in witness)
            bad_items = list(good)
            bad_items[WOTS_L + 1] = bytes([bad_idx])   # replace key_index byte
            tx_bad = CTransaction(spend_tx)
            self._set_witness(tx_bad, 0, bad_items)
            assert_raises_rpc_error(-26, None,
                node.sendrawtransaction, tx_bad.serialize().hex())
            self.log.info(f"  key_index={bad_idx}: correctly rejected")

        self.log.info("test_key_index_out_of_range: PASS")

    def test_double_spend_rejected(self):
        """
        After spending a P2WOTS UTXO, attempting to spend it again must fail.
        The UTXO set (standard Bitcoin) rejects any input referencing a
        previously spent outpoint.
        """
        node = self.nodes[0]
        msk  = bytes([0x13] * 32)

        txid_int, vout, an, ac, spk = self._fund_p2wots(node, msk, 0, int(0.4 * COIN))
        dest_spk = self._get_dest_spk(node)

        # First spend: must succeed
        spend_txid = self._spend_p2wots(node, txid_int, vout, msk, an, ac, spk,
                                         int(0.4 * COIN), dest_spk, slot_index=0)
        self.generate(node, 1)
        assert spend_txid in node.getblock(node.getbestblockhash())['tx']
        self.log.info(f"  First spend confirmed: {spend_txid[:16]}...")

        # Second spend attempt with a different slot -- must fail (UTXO gone)
        dest_spk2 = self._get_dest_spk(node)
        spend_tx2, spent_out2 = self._make_tx_and_spent(txid_int, vout, int(0.4 * COIN),
                                                          spk, dest_spk2)
        sighash2  = compute_p2wots_sighash(spend_tx2, 0, [spent_out2])
        msg_hash2 = compute_msg_hash(sighash2)
        sn2  = compute_slot_nonce(msk, an, 1)
        sig2 = sign_wots(msk, sn2, msg_hash2)
        apath2 = generate_auth_path(msk, an, 1)
        items2 = build_p2wots_witness(sig2, sn2, 1, apath2)
        self._set_witness(spend_tx2, 0, items2)

        assert_raises_rpc_error(-25, None,
            node.sendrawtransaction, spend_tx2.serialize().hex())
        self.log.info("  Double-spend correctly rejected (UTXO already spent)")
        self.log.info("test_double_spend_rejected: PASS")

    def test_multisig_2of2(self):
        """
        Fund a 2-of-2 P2WOTS multisig output and spend it with both keys.
        Both signers must provide valid WOTS+ signatures from their own address.
        """
        node = self.nodes[0]
        msk0 = bytes([0x20] * 32)
        msk1 = bytes([0x21] * 32)

        an0   = compute_address_nonce(msk0, 0)
        an1   = compute_address_nonce(msk1, 0)
        root0 = compute_merkle_root(msk0, an0)
        root1 = compute_merkle_root(msk1, an1)

        commitment = compute_multisig_commitment(2, 2, [root0, root1])
        ms_spk     = build_p2wots_script(commitment)

        fund_txid = self._fund_output_raw(node, ms_spk, int(0.5 * COIN))
        self.generate(node, 1)
        raw = node.getrawtransaction(fund_txid, True)
        vout = next(i for i, o in enumerate(raw['vout'])
                    if bytes.fromhex(o['scriptPubKey']['hex']) == ms_spk)
        txid_int = int(fund_txid, 16)

        dest_spk = self._get_dest_spk(node)
        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.5 * COIN),
                                                        ms_spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        # Signer 0: uses msk0, slot 0
        sn0   = compute_slot_nonce(msk0, an0, 0)
        sig0  = sign_wots(msk0, sn0, msg_hash)
        ap0   = generate_auth_path(msk0, an0, 0)

        # Signer 1: uses msk1, slot 0
        sn1   = compute_slot_nonce(msk1, an1, 0)
        sig1  = sign_wots(msk1, sn1, msg_hash)
        ap1   = generate_auth_path(msk1, an1, 0)

        items = build_p2wots_multisig_witness(
            k=2, n=2,
            merkle_roots=[root0, root1],
            signers=[
                {'signer_index': 0, 'sig': sig0, 'slot_nonce': sn0, 'key_index': 0, 'auth_path': ap0},
                {'signer_index': 1, 'sig': sig1, 'slot_nonce': sn1, 'key_index': 0, 'auth_path': ap1},
            ]
        )
        assert len(items) == 1 + 2 + 2 * WOTS_MULTISIG_ITEMS_PER_SIGNER  # 89

        self._set_witness(spend_tx, 0, items)
        spend_txid = node.sendrawtransaction(spend_tx.serialize().hex())
        self.generate(node, 1)
        assert spend_txid in node.getblock(node.getbestblockhash())['tx'], \
            "2-of-2 multisig spend must confirm"
        self.log.info(f"  2-of-2 spend txid: {spend_txid[:16]}...")
        self.log.info("test_multisig_2of2: PASS")

    def test_multisig_2of3_threshold(self):
        """
        Fund a 2-of-3 P2WOTS multisig.  Any 2 of the 3 signers can spend.
        Test with signers 0+2 (skipping signer 1).
        """
        node = self.nodes[0]
        msks = [bytes([0x30 + i] * 32) for i in range(3)]
        ans  = [compute_address_nonce(m, 0) for m in msks]
        roots = [compute_merkle_root(m, a) for m, a in zip(msks, ans)]

        commitment = compute_multisig_commitment(2, 3, roots)
        ms_spk     = build_p2wots_script(commitment)

        fund_txid = self._fund_output_raw(node, ms_spk, int(0.4 * COIN))
        self.generate(node, 1)
        raw = node.getrawtransaction(fund_txid, True)
        vout = next(i for i, o in enumerate(raw['vout'])
                    if bytes.fromhex(o['scriptPubKey']['hex']) == ms_spk)
        txid_int = int(fund_txid, 16)

        dest_spk = self._get_dest_spk(node)
        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.4 * COIN),
                                                        ms_spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        # Use signers 0 and 2 (skip signer 1)
        signers_data = []
        for si in [0, 2]:
            sn  = compute_slot_nonce(msks[si], ans[si], 5)
            sig = sign_wots(msks[si], sn, msg_hash)
            ap  = generate_auth_path(msks[si], ans[si], 5)
            signers_data.append({
                'signer_index': si, 'sig': sig,
                'slot_nonce': sn, 'key_index': 5, 'auth_path': ap,
            })

        items = build_p2wots_multisig_witness(k=2, n=3, merkle_roots=roots,
                                               signers=signers_data)
        assert len(items) == 1 + 3 + 2 * WOTS_MULTISIG_ITEMS_PER_SIGNER  # 90

        self._set_witness(spend_tx, 0, items)
        spend_txid = node.sendrawtransaction(spend_tx.serialize().hex())
        self.generate(node, 1)
        assert spend_txid in node.getblock(node.getbestblockhash())['tx'], \
            "2-of-3 multisig spend must confirm"
        self.log.info(f"  2-of-3 (signers 0+2) spend txid: {spend_txid[:16]}...")
        self.log.info("test_multisig_2of3_threshold: PASS")

    def test_multisig_duplicate_signer(self):
        """
        Reusing the same signer_index twice in a 2-of-2 must be rejected.
        The interpreter tracks signerUsed[] and rejects duplicate policy slots.
        """
        node = self.nodes[0]
        msk0 = bytes([0x40] * 32)
        msk1 = bytes([0x41] * 32)

        an0   = compute_address_nonce(msk0, 0)
        an1   = compute_address_nonce(msk1, 0)
        root0 = compute_merkle_root(msk0, an0)
        root1 = compute_merkle_root(msk1, an1)

        commitment = compute_multisig_commitment(2, 2, [root0, root1])
        ms_spk     = build_p2wots_script(commitment)

        fund_txid = self._fund_output_raw(node, ms_spk, int(0.3 * COIN))
        self.generate(node, 1)
        raw = node.getrawtransaction(fund_txid, True)
        vout = next(i for i, o in enumerate(raw['vout'])
                    if bytes.fromhex(o['scriptPubKey']['hex']) == ms_spk)
        txid_int = int(fund_txid, 16)

        dest_spk = self._get_dest_spk(node)
        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.3 * COIN),
                                                        ms_spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        # Both signers use signer_index=0 (root0) -- duplicate
        sn0  = compute_slot_nonce(msk0, an0, 0)
        sig0 = sign_wots(msk0, sn0, msg_hash)
        ap0  = generate_auth_path(msk0, an0, 0)

        sn0b  = compute_slot_nonce(msk0, an0, 1)
        sig0b = sign_wots(msk0, sn0b, msg_hash)
        ap0b  = generate_auth_path(msk0, an0, 1)

        items = build_p2wots_multisig_witness(
            k=2, n=2,
            merkle_roots=[root0, root1],
            signers=[
                {'signer_index': 0, 'sig': sig0,  'slot_nonce': sn0,  'key_index': 0, 'auth_path': ap0},
                {'signer_index': 0, 'sig': sig0b, 'slot_nonce': sn0b, 'key_index': 1, 'auth_path': ap0b},
            ]
        )
        self._set_witness(spend_tx, 0, items)

        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx.serialize().hex())
        self.log.info("test_multisig_duplicate_signer: PASS")

    def test_multisig_wrong_commitment(self):
        """
        The n Merkle roots in the witness must hash to the commitment stored in
        the scriptPubKey.  Providing different roots (same k,n) must be rejected.
        """
        node = self.nodes[0]
        msk0 = bytes([0x50] * 32)
        msk1 = bytes([0x51] * 32)
        msk2 = bytes([0x52] * 32)  # extra key not in commitment

        an0   = compute_address_nonce(msk0, 0)
        an1   = compute_address_nonce(msk1, 0)
        an2   = compute_address_nonce(msk2, 0)
        root0 = compute_merkle_root(msk0, an0)
        root1 = compute_merkle_root(msk1, an1)
        root2 = compute_merkle_root(msk2, an2)

        # Commitment is for [root0, root1], but attacker supplies [root0, root2]
        commitment = compute_multisig_commitment(2, 2, [root0, root1])
        ms_spk     = build_p2wots_script(commitment)

        fund_txid = self._fund_output_raw(node, ms_spk, int(0.25 * COIN))
        self.generate(node, 1)
        raw = node.getrawtransaction(fund_txid, True)
        vout = next(i for i, o in enumerate(raw['vout'])
                    if bytes.fromhex(o['scriptPubKey']['hex']) == ms_spk)
        txid_int = int(fund_txid, 16)

        dest_spk = self._get_dest_spk(node)
        spend_tx, spent_out = self._make_tx_and_spent(txid_int, vout, int(0.25 * COIN),
                                                        ms_spk, dest_spk)
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn0  = compute_slot_nonce(msk0, an0, 0)
        sig0 = sign_wots(msk0, sn0, msg_hash)
        ap0  = generate_auth_path(msk0, an0, 0)
        sn2  = compute_slot_nonce(msk2, an2, 0)
        sig2 = sign_wots(msk2, sn2, msg_hash)
        ap2  = generate_auth_path(msk2, an2, 0)

        # Provide [root0, root2] instead of [root0, root1] -- wrong commitment
        items = build_p2wots_multisig_witness(
            k=2, n=2,
            merkle_roots=[root0, root2],
            signers=[
                {'signer_index': 0, 'sig': sig0, 'slot_nonce': sn0, 'key_index': 0, 'auth_path': ap0},
                {'signer_index': 1, 'sig': sig2, 'slot_nonce': sn2, 'key_index': 0, 'auth_path': ap2},
            ]
        )
        self._set_witness(spend_tx, 0, items)

        assert_raises_rpc_error(-26, None,
            node.sendrawtransaction, spend_tx.serialize().hex())
        self.log.info("test_multisig_wrong_commitment: PASS")

    def test_multiple_p2wots_inputs_one_tx(self):
        """
        A transaction with two P2WOTS inputs must be validated independently
        per input.  Both must carry valid signatures; if either is bad the tx
        is rejected.
        """
        node = self.nodes[0]
        msk  = bytes([0x60] * 32)
        an   = compute_address_nonce(msk, 0)
        ac   = compute_merkle_root(msk, an)
        spk  = build_p2wots_script(ac)

        # Fund two UTXOs to the same address
        fee = 10_000
        amt = int(0.2 * COIN)

        txid_a = self._fund_output_raw(node, spk, amt)
        self.generate(node, 1)
        raw_a = node.getrawtransaction(txid_a, True)
        vout_a = next(i for i, o in enumerate(raw_a['vout'])
                      if bytes.fromhex(o['scriptPubKey']['hex']) == spk)

        txid_b = self._fund_output_raw(node, spk, amt)
        self.generate(node, 1)
        raw_b = node.getrawtransaction(txid_b, True)
        vout_b = next(i for i, o in enumerate(raw_b['vout'])
                      if bytes.fromhex(o['scriptPubKey']['hex']) == spk)

        txid_a_int = int(txid_a, 16)
        txid_b_int = int(txid_b, 16)

        # Build a transaction spending BOTH P2WOTS UTXOs in one go
        dest_spk = self._get_dest_spk(node)
        spend_tx = CTransaction()
        spend_tx.version  = 2
        spend_tx.nLockTime = 0
        spend_tx.vin = [
            CTxIn(COutPoint(txid_a_int, vout_a), nSequence=0xFFFFFFFF),
            CTxIn(COutPoint(txid_b_int, vout_b), nSequence=0xFFFFFFFF),
        ]
        spend_tx.vout = [CTxOut(2 * amt - fee, CScript(dest_spk))]
        spend_tx.wit.vtxinwit = [CTxInWitness(), CTxInWitness()]

        spent_outs = [CTxOut(amt, CScript(spk)), CTxOut(amt, CScript(spk))]

        # Sign each input with a different slot
        for in_pos, slot_idx in [(0, 2), (1, 7)]:
            sighash  = compute_p2wots_sighash(spend_tx, in_pos, spent_outs)
            msg_hash = compute_msg_hash(sighash)
            sn   = compute_slot_nonce(msk, an, slot_idx)
            sig  = sign_wots(msk, sn, msg_hash)
            apath = generate_auth_path(msk, an, slot_idx)
            items = build_p2wots_witness(sig, sn, slot_idx, apath)
            spend_tx.wit.vtxinwit[in_pos].scriptWitness.stack = items

        spend_txid = node.sendrawtransaction(spend_tx.serialize().hex())
        self.generate(node, 1)
        assert spend_txid in node.getblock(node.getbestblockhash())['tx'], \
            "Multi-input P2WOTS spend must confirm"
        self.log.info(f"  Multi-input spend txid: {spend_txid[:16]}...")
        self.log.info("test_multiple_p2wots_inputs_one_tx: PASS")

    # =========================================================================
    # HELPERS
    # =========================================================================

    def _fund_p2wots(self, node, msk_wots, address_index, amount_sat):
        """
        Create a P2WOTS UTXO using address_index for key derivation.
        Returns (txid_int, vout, address_nonce, address_commitment, scriptPubKey).
        The address_commitment is the Merkle root over all 64 slot keys.
        """
        an  = compute_address_nonce(msk_wots, address_index)
        ac  = compute_merkle_root(msk_wots, an)
        spk = build_p2wots_script(ac)

        txid_hex = self._fund_output_raw(node, spk, amount_sat)
        self.generate(node, 1)

        raw = node.getrawtransaction(txid_hex, True)
        vout = next(i for i, o in enumerate(raw['vout'])
                    if bytes.fromhex(o['scriptPubKey']['hex']) == spk)

        return int(txid_hex, 16), vout, an, ac, spk

    def _spend_p2wots(self, node, txid_int, vout, msk_wots, address_nonce,
                      address_commitment, p2wots_spk, amount_sat, dest_spk,
                      slot_index=0, fee_sat=10_000):
        """
        Build, sign (WOTS+), and broadcast a P2WOTS spending transaction.
        Returns the spend txid hex string.
        """
        spend_tx, spent_out = self._make_tx_and_spent(
            txid_int, vout, amount_sat, p2wots_spk, dest_spk, fee_sat
        )
        sighash  = compute_p2wots_sighash(spend_tx, 0, [spent_out])
        msg_hash = compute_msg_hash(sighash)

        sn    = compute_slot_nonce(msk_wots, address_nonce, slot_index)
        pk    = generate_wots_pk(msk_wots, sn)
        sig   = sign_wots(msk_wots, sn, msg_hash)
        apath = generate_auth_path(msk_wots, address_nonce, slot_index)

        # Python-side pre-verification
        assert verify_wots(pk, sn, msg_hash, sig), \
            "Python WOTS verify must pass before broadcast"
        assert verify_auth_path(pk, slot_index, apath, address_commitment), \
            "Python auth_path must verify before broadcast"

        items = build_p2wots_witness(sig, sn, slot_index, apath)
        self._set_witness(spend_tx, 0, items)

        return node.sendrawtransaction(spend_tx.serialize().hex())

    def _make_tx_and_spent(self, txid_int, vout, amount_sat, p2wots_spk,
                           dest_spk, fee_sat=10_000):
        """Build an unsigned CTransaction and its corresponding spent CTxOut."""
        tx = CTransaction()
        tx.version   = 2
        tx.nLockTime = 0
        tx.vin  = [CTxIn(COutPoint(txid_int, vout), nSequence=0xFFFFFFFF)]
        tx.vout = [CTxOut(amount_sat - fee_sat, CScript(dest_spk))]
        tx.wit.vtxinwit = [CTxInWitness()]
        spent_out = CTxOut(amount_sat, CScript(p2wots_spk))
        return tx, spent_out

    def _set_witness(self, tx, in_pos, items):
        """Set the witness stack for input in_pos in tx."""
        while len(tx.wit.vtxinwit) <= in_pos:
            tx.wit.vtxinwit.append(CTxInWitness())
        tx.wit.vtxinwit[in_pos].scriptWitness.stack = items

    def _fund_output_raw(self, node, scriptpubkey, amount_sat):
        """Fund a custom scriptPubKey via fundrawtransaction. Returns txid hex."""
        partial = CTransaction()
        partial.version = 2
        partial.vout = [CTxOut(amount_sat, CScript(scriptpubkey))]
        funded = node.fundrawtransaction(
            partial.serialize().hex(), {"changePosition": 1}
        )['hex']
        signed = node.signrawtransactionwithwallet(funded)['hex']
        return node.sendrawtransaction(signed)

    def _get_dest_spk(self, node):
        """Get a fresh wallet scriptPubKey for the spending destination."""
        return bytes.fromhex(
            node.getaddressinfo(node.getnewaddress())['scriptPubKey']
        )


if __name__ == '__main__':
    WOTS39Test(__file__).main()
