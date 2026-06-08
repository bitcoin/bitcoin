// Copyright (c) 2026 The WOTS-39 Authors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <crypto/hex_base.h>
#include <crypto/wots_sha256.h>
#include <uint256.h>

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

// ── Helpers ───────────────────────────────────────────────────────────────────

static uint256 FillUint256(uint8_t byte)
{
    uint256 v;
    std::fill(v.begin(), v.end(), byte);
    return v;
}

// HexStr gives bytes in forward order, matching Python's bytes.hex().
static std::string Hex(const uint256& v)
{
    return HexStr(std::span<const uint8_t>{v.begin(), 32});
}

// ── Test suite ─────────────────────────────────────────────────────────────────

BOOST_AUTO_TEST_SUITE(wots_tests)

// ---------------------------------------------------------------------------
// address_nonce derivation  (ComputeAddressNonce / ComputeWotsNonce alias)
// address_nonce = SHA256("wots39-addr-v1" || mskWOTS || uint64_be(address_index))
// One nonce per address; increment address_index to get a fresh address.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(address_nonce_derivation)
{
    uint256 msk = FillUint256(0x01);

    uint256 nonce0 = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce1 = WOTS39::ComputeAddressNonce(msk, 1);
    uint256 nonce2 = WOTS39::ComputeAddressNonce(msk, 2);

    // Deterministic
    BOOST_CHECK(WOTS39::ComputeAddressNonce(msk, 0) == nonce0);

    // Unique across address_index
    BOOST_CHECK(nonce0 != nonce1);
    BOOST_CHECK(nonce1 != nonce2);
    BOOST_CHECK(nonce0 != nonce2);

    // Unique across master keys (different wallets produce different nonces)
    uint256 msk2 = FillUint256(0xAA);
    BOOST_CHECK(WOTS39::ComputeAddressNonce(msk2, 0) != nonce0);

    // ComputeWotsNonce is an alias for ComputeAddressNonce
    BOOST_CHECK(WOTS39::ComputeWotsNonce(msk, 0) == nonce0);
    BOOST_CHECK(WOTS39::ComputeWotsNonce(msk, 1) == nonce1);

    BOOST_TEST_MESSAGE("ComputeAddressNonce(0x01*32, 0): " << Hex(nonce0));
    BOOST_TEST_MESSAGE("ComputeAddressNonce(0x01*32, 1): " << Hex(nonce1));
}

// ---------------------------------------------------------------------------
// slot_nonce derivation  (ComputeSlotNonce)
// slot_nonce = SHA256("wots39-slot-v1" || mskWOTS || address_nonce || uint64_be(slot_index))
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(slot_nonce_derivation)
{
    uint256 msk          = FillUint256(0x01);
    uint256 addr_nonce   = WOTS39::ComputeAddressNonce(msk, 0);

    uint256 slot0 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 slot1 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 1);

    // Deterministic
    BOOST_CHECK(WOTS39::ComputeSlotNonce(msk, addr_nonce, 0) == slot0);

    // Unique across slot_index
    BOOST_CHECK(slot0 != slot1);

    // Unique from address_nonce (domain separation: slot nonces != address nonces)
    BOOST_CHECK(slot0 != addr_nonce);

    // Different address_nonce → different slot_nonce
    uint256 addr_nonce2 = WOTS39::ComputeAddressNonce(msk, 1);
    BOOST_CHECK(WOTS39::ComputeSlotNonce(msk, addr_nonce2, 0) != slot0);

    // Different msk → different slot_nonce
    uint256 msk2 = FillUint256(0x02);
    BOOST_CHECK(WOTS39::ComputeSlotNonce(msk2, addr_nonce, 0) != slot0);

    BOOST_TEST_MESSAGE("ComputeSlotNonce(0x01*32, addr_nonce0, 0): " << Hex(slot0));
}

// ---------------------------------------------------------------------------
// wots_nonce_derivation — alias test (kept for API continuity)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(wots_nonce_derivation)
{
    uint256 msk = FillUint256(0x01);

    uint256 nonce0 = WOTS39::ComputeWotsNonce(msk, 0);
    uint256 nonce1 = WOTS39::ComputeWotsNonce(msk, 1);

    // Deterministic
    BOOST_CHECK(WOTS39::ComputeWotsNonce(msk, 0) == nonce0);

    // Unique across index
    BOOST_CHECK(nonce0 != nonce1);

    // Unique across master keys
    uint256 msk2 = FillUint256(0xAA);
    BOOST_CHECK(WOTS39::ComputeWotsNonce(msk2, 0) != nonce0);
}

// ---------------------------------------------------------------------------
// SK derivation — uses slot_nonce as domain separator
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(sk_derivation)
{
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 sk0        = WOTS39::DeriveSK(msk, nonce, 0);

    BOOST_CHECK(sk0 != uint256{});   // must be non-zero

    // Different chain index → different SK
    BOOST_CHECK(WOTS39::DeriveSK(msk, nonce, 1) != sk0);

    // Different nonce → different SK (domain separation works)
    uint256 nonce1 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 1);
    BOOST_CHECK(WOTS39::DeriveSK(msk, nonce1, 0) != sk0);

    // Deterministic
    BOOST_CHECK(WOTS39::DeriveSK(msk, nonce, 0) == sk0);

    BOOST_TEST_MESSAGE("slot_nonce(0, 0): " << Hex(nonce));
    BOOST_TEST_MESSAGE("sk0: " << Hex(sk0));
}

// ---------------------------------------------------------------------------
// Chain step — uses slot_nonce as domain separator
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(chain_step_deterministic)
{
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 sk0        = WOTS39::DeriveSK(msk, nonce, 0);

    uint256 step1 = WOTS39::ChainStep(sk0, 0, nonce, 0);
    BOOST_CHECK(WOTS39::ChainStep(sk0, 0, nonce, 0) == step1);  // deterministic

    // chain_idx sensitivity
    BOOST_CHECK(WOTS39::ChainStep(sk0, 0, nonce, 1) != step1);

    // step j sensitivity
    BOOST_CHECK(WOTS39::ChainStep(sk0, 1, nonce, 0) != step1);

    // nonce sensitivity (different domain separator → different chain)
    uint256 nonce1 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 1);
    BOOST_CHECK(WOTS39::ChainStep(sk0, 0, nonce1, 0) != step1);

    BOOST_TEST_MESSAGE("ChainStep(sk0, 0, slot_nonce0, 0): " << Hex(step1));
}

// ---------------------------------------------------------------------------
// BaseW encoding — w=256: 32 message bytes + 2 checksum bytes = 34 digits
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(base_w_encode_all_ones)
{
    // 0xFF*32: all bytes = 255 → checksum = 32*(255-255) = 0
    auto digits = WOTS39::BaseWEncode(FillUint256(0xFF));
    BOOST_CHECK_EQUAL(digits.size(), static_cast<size_t>(WOTS39::WOTS_L));  // 34
    for (int i = 0; i < 32; ++i) BOOST_CHECK_EQUAL(digits[i], 255u);
    BOOST_CHECK_EQUAL(digits[32], 0u);
    BOOST_CHECK_EQUAL(digits[33], 0u);
}

BOOST_AUTO_TEST_CASE(base_w_encode_all_zeros)
{
    // 0x00*32: all bytes = 0 → checksum = 32*255 = 8160 = 0x1FE0
    auto digits = WOTS39::BaseWEncode(FillUint256(0x00));
    BOOST_CHECK_EQUAL(digits.size(), static_cast<size_t>(WOTS39::WOTS_L));  // 34
    for (int i = 0; i < 32; ++i) BOOST_CHECK_EQUAL(digits[i], 0u);
    BOOST_CHECK_EQUAL(digits[32], 0x1Fu);  // high byte of 0x1FE0
    BOOST_CHECK_EQUAL(digits[33], 0xE0u);  // low  byte of 0x1FE0
}

BOOST_AUTO_TEST_CASE(base_w_encode_checksum_invariant)
{
    // sum(255 - d[i]) for the 32 message digits must equal the encoded 2-byte checksum
    auto digits = WOTS39::BaseWEncode(FillUint256(0xAB));
    BOOST_CHECK_EQUAL(digits.size(), static_cast<size_t>(WOTS39::WOTS_L));  // 34
    uint64_t csum_msg = 0;
    for (int i = 0; i < 32; ++i) csum_msg += (255 - digits[i]);
    uint64_t csum_enc = (static_cast<uint64_t>(digits[32]) << 8)
                      |  static_cast<uint64_t>(digits[33]);
    BOOST_CHECK_EQUAL(csum_msg, csum_enc);
}

// ---------------------------------------------------------------------------
// GenerateWOTSPK — uses slot_nonce as domain separator
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(generate_pk_properties)
{
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 pk         = WOTS39::GenerateWOTSPK(msk, nonce);

    // Non-zero (with overwhelming probability)
    BOOST_CHECK(pk != uint256{});

    // Deterministic
    BOOST_CHECK(WOTS39::GenerateWOTSPK(msk, nonce) == pk);

    // Different nonce → different PK
    uint256 nonce1 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 1);
    BOOST_CHECK(WOTS39::GenerateWOTSPK(msk, nonce1) != pk);

    // Different MSK → different PK
    BOOST_CHECK(WOTS39::GenerateWOTSPK(FillUint256(0x02), nonce) != pk);

    BOOST_TEST_MESSAGE("GenerateWOTSPK(0x01*32, slot_nonce(addr0, slot0)): " << Hex(pk));
}

// ---------------------------------------------------------------------------
// Sign / verify round-trip — uses slot_nonce as domain separator
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(sign_verify_roundtrip)
{
    uint256 msk        = FillUint256(0x42);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 msg        = FillUint256(0xCD);

    uint256 wotsPK = WOTS39::GenerateWOTSPK(msk, nonce);

    auto sig = WOTS39::Sign(msk, nonce, msg);
    BOOST_CHECK_EQUAL(sig.size(), static_cast<size_t>(WOTS39::WOTS_SIG_SIZE));  // 1088
    BOOST_CHECK(WOTS39::Verify(wotsPK, nonce, msg, sig));

    BOOST_TEST_MESSAGE("sign_verify PK (msk=0x42*32, slot_nonce(addr0,slot0)): " << Hex(wotsPK));
}

BOOST_AUTO_TEST_CASE(verify_fails_wrong_message)
{
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 msg1       = FillUint256(0x03);
    uint256 msg2       = FillUint256(0x04);

    uint256 wotsPK = WOTS39::GenerateWOTSPK(msk, nonce);
    auto sig = WOTS39::Sign(msk, nonce, msg1);

    BOOST_CHECK( WOTS39::Verify(wotsPK, nonce, msg1, sig));
    BOOST_CHECK(!WOTS39::Verify(wotsPK, nonce, msg2, sig));
}

BOOST_AUTO_TEST_CASE(verify_fails_wrong_nonce)
{
    // Security property: sig built with slot_nonce_a must not verify under slot_nonce_b.
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce0     = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 nonce1     = WOTS39::ComputeSlotNonce(msk, addr_nonce, 1);
    uint256 msg        = FillUint256(0x42);

    uint256 wotsPK0 = WOTS39::GenerateWOTSPK(msk, nonce0);
    auto sig0 = WOTS39::Sign(msk, nonce0, msg);

    BOOST_CHECK( WOTS39::Verify(wotsPK0, nonce0, msg, sig0));   // correct nonce
    BOOST_CHECK(!WOTS39::Verify(wotsPK0, nonce1, msg, sig0));   // wrong nonce → fail
}

BOOST_AUTO_TEST_CASE(verify_fails_corrupted_sig)
{
    uint256 msk        = FillUint256(0x07);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 msg        = FillUint256(0x09);

    uint256 wotsPK = WOTS39::GenerateWOTSPK(msk, nonce);
    auto sig = WOTS39::Sign(msk, nonce, msg);

    sig[512] ^= 0xFF;  // flip one byte in element 16 of 34
    BOOST_CHECK(!WOTS39::Verify(wotsPK, nonce, msg, sig));
}

BOOST_AUTO_TEST_CASE(verify_rejects_wrong_sig_size)
{
    uint256 wotsPK = FillUint256(0x00);
    uint256 nonce  = FillUint256(0x00);
    uint256 msg    = FillUint256(0x00);

    BOOST_CHECK(!WOTS39::Verify(wotsPK, nonce, msg, std::vector<uint8_t>(64, 0)));
    BOOST_CHECK(!WOTS39::Verify(wotsPK, nonce, msg,
        std::vector<uint8_t>(WOTS39::WOTS_SIG_SIZE + 1, 0)));
}

// ---------------------------------------------------------------------------
// VerifyFromStackElements — uses slot_nonce
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(verify_from_stack_elements_roundtrip)
{
    uint256 msk        = FillUint256(0x11);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 msg        = FillUint256(0x33);

    uint256 wotsPK = WOTS39::GenerateWOTSPK(msk, nonce);
    auto sig = WOTS39::Sign(msk, nonce, msg);

    // Split flat sig into WOTS_L (34) x 32-byte stack elements
    std::vector<std::vector<uint8_t>> elems(WOTS39::WOTS_L);
    for (int i = 0; i < WOTS39::WOTS_L; ++i) {
        elems[i].assign(sig.begin() + i * 32, sig.begin() + (i + 1) * 32);
    }
    BOOST_CHECK(WOTS39::VerifyFromStackElements(wotsPK, nonce, msg, elems));

    // Wrong element count → fail
    elems.pop_back();
    BOOST_CHECK(!WOTS39::VerifyFromStackElements(wotsPK, nonce, msg, elems));
}

BOOST_AUTO_TEST_CASE(verify_from_stack_wrong_element_size)
{
    uint256 wotsPK = FillUint256(0x00);
    uint256 nonce  = FillUint256(0x00);
    uint256 msg    = FillUint256(0x00);

    // WOTS_L (34) elements but one is 31 bytes instead of 32
    std::vector<std::vector<uint8_t>> elems(WOTS39::WOTS_L, std::vector<uint8_t>(32, 0));
    elems[5].resize(31);
    BOOST_CHECK(!WOTS39::VerifyFromStackElements(wotsPK, nonce, msg, elems));
}

// ---------------------------------------------------------------------------
// Merkle Key Tree — leaf hash
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(merkle_leaf_deterministic)
{
    uint256 pk = FillUint256(0xAB);
    uint256 leaf = WOTS39::WotsMerkleLeaf(pk);

    // Deterministic
    BOOST_CHECK(WOTS39::WotsMerkleLeaf(pk) == leaf);

    // Non-zero
    BOOST_CHECK(leaf != uint256{});

    // Different pk → different leaf
    BOOST_CHECK(WOTS39::WotsMerkleLeaf(FillUint256(0xAC)) != leaf);

    // Leaf hash != pk itself (it's hash-compressed, not identity)
    BOOST_CHECK(leaf != pk);

    BOOST_TEST_MESSAGE("WotsMerkleLeaf(0xAB*32): " << Hex(leaf));
}

// ---------------------------------------------------------------------------
// Merkle Key Tree — internal node hash
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(merkle_node_deterministic)
{
    uint256 left  = FillUint256(0x01);
    uint256 right = FillUint256(0x02);
    uint256 node  = WOTS39::WotsMerkleNode(left, right);

    // Deterministic
    BOOST_CHECK(WOTS39::WotsMerkleNode(left, right) == node);

    // Non-zero
    BOOST_CHECK(node != uint256{});

    // Not commutative: node(L,R) != node(R,L)
    BOOST_CHECK(WOTS39::WotsMerkleNode(right, left) != node);

    // Leaf domain != node domain: hash a leaf-shaped input both ways
    uint256 pk = FillUint256(0x33);
    BOOST_CHECK(WOTS39::WotsMerkleLeaf(pk) != WOTS39::WotsMerkleNode(pk, pk));

    BOOST_TEST_MESSAGE("WotsMerkleNode(0x01*32, 0x02*32): " << Hex(node));
}

// ---------------------------------------------------------------------------
// Merkle Key Tree — root is reproducible
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(merkle_root_builds)
{
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);

    uint256 root = WOTS39::ComputeMerkleRoot(msk, addr_nonce);

    // Deterministic
    BOOST_CHECK(WOTS39::ComputeMerkleRoot(msk, addr_nonce) == root);

    // Non-zero
    BOOST_CHECK(root != uint256{});

    // Different address_nonce → different root
    uint256 addr_nonce2 = WOTS39::ComputeAddressNonce(msk, 1);
    BOOST_CHECK(WOTS39::ComputeMerkleRoot(msk, addr_nonce2) != root);

    // Different msk → different root
    BOOST_CHECK(WOTS39::ComputeMerkleRoot(FillUint256(0x02), addr_nonce) != root);

    BOOST_TEST_MESSAGE("ComputeMerkleRoot(0x01*32, addr_nonce0): " << Hex(root));
}

// ---------------------------------------------------------------------------
// Merkle Key Tree — authentication path round-trip
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(auth_path_verify_roundtrip)
{
    uint256 msk        = FillUint256(0x03);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 root       = WOTS39::ComputeMerkleRoot(msk, addr_nonce);

    // Verify every slot's auth path is valid under the root
    for (uint64_t slot = 0; slot < WOTS39::WOTS_TREE_SLOTS; ++slot) {
        uint256 slot_nonce = WOTS39::ComputeSlotNonce(msk, addr_nonce, slot);
        uint256 pk         = WOTS39::GenerateWOTSPK(msk, slot_nonce);
        auto   auth_path   = WOTS39::GenerateAuthPath(msk, addr_nonce, slot);

        BOOST_CHECK(WOTS39::VerifyAuthPath(pk, slot, auth_path, root));
    }
}

// ---------------------------------------------------------------------------
// Merkle Key Tree — wrong slot index must fail
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(auth_path_wrong_index_fails)
{
    uint256 msk        = FillUint256(0x05);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 root       = WOTS39::ComputeMerkleRoot(msk, addr_nonce);

    uint256 slot_nonce0 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 pk0         = WOTS39::GenerateWOTSPK(msk, slot_nonce0);
    auto    auth_path0  = WOTS39::GenerateAuthPath(msk, addr_nonce, 0);

    // Correct slot → passes
    BOOST_CHECK(WOTS39::VerifyAuthPath(pk0, 0, auth_path0, root));

    // Wrong slot index → fails
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk0, 1, auth_path0, root));

    // Auth path from slot 1 presented for slot 0 → fails
    auto auth_path1 = WOTS39::GenerateAuthPath(msk, addr_nonce, 1);
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk0, 0, auth_path1, root));

    // Wrong pk → fails
    uint256 slot_nonce1 = WOTS39::ComputeSlotNonce(msk, addr_nonce, 1);
    uint256 pk1         = WOTS39::GenerateWOTSPK(msk, slot_nonce1);
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk1, 0, auth_path0, root));
}

// ---------------------------------------------------------------------------
// Full cross-layer summary
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(cross_layer_all_vectors)
{
    uint256 msk        = FillUint256(0x01);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 slot_nonce = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);

    uint256 sk0   = WOTS39::DeriveSK(msk, slot_nonce, 0);
    uint256 step1 = WOTS39::ChainStep(sk0, 0, slot_nonce, 0);
    uint256 pk    = WOTS39::GenerateWOTSPK(msk, slot_nonce);

    // All values must be non-zero and mutually distinct
    BOOST_CHECK(addr_nonce != uint256{});
    BOOST_CHECK(slot_nonce != uint256{});
    BOOST_CHECK(sk0        != uint256{});
    BOOST_CHECK(step1      != uint256{});
    BOOST_CHECK(pk         != uint256{});
    BOOST_CHECK(addr_nonce != slot_nonce);

    // Sign + verify round-trip with canonical inputs
    uint256 msg = FillUint256(0xAB);
    auto sig = WOTS39::Sign(msk, slot_nonce, msg);
    BOOST_CHECK_EQUAL(sig.size(), static_cast<size_t>(WOTS39::WOTS_SIG_SIZE));  // 1088
    BOOST_CHECK(WOTS39::Verify(pk, slot_nonce, msg, sig));

    // Merkle tree round-trip
    uint256 root      = WOTS39::ComputeMerkleRoot(msk, addr_nonce);
    auto    auth_path = WOTS39::GenerateAuthPath(msk, addr_nonce, 0);
    BOOST_CHECK(WOTS39::VerifyAuthPath(pk, 0, auth_path, root));

    // Log values for cross-reference with Python test vectors
    BOOST_TEST_MESSAGE("addr_nonce: " << Hex(addr_nonce));
    BOOST_TEST_MESSAGE("slot_nonce: " << Hex(slot_nonce));
    BOOST_TEST_MESSAGE("sk0:        " << Hex(sk0));
    BOOST_TEST_MESSAGE("step1:      " << Hex(step1));
    BOOST_TEST_MESSAGE("pk(slot0):  " << Hex(pk));
    BOOST_TEST_MESSAGE("merkle_root:" << Hex(root));
}

// ---------------------------------------------------------------------------
// Multi-sig — ComputeMultiSigCommitment is deterministic and unique
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_commitment_deterministic)
{
    uint256 msk = FillUint256(0x01);

    // Build two independent addresses for a 2-of-2 policy
    uint256 addr0 = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 addr1 = WOTS39::ComputeAddressNonce(msk, 1);
    uint256 root0 = WOTS39::ComputeMerkleRoot(msk, addr0);
    uint256 root1 = WOTS39::ComputeMerkleRoot(msk, addr1);

    std::vector<uint256> roots_2of2 = {root0, root1};
    uint256 c1 = WOTS39::ComputeMultiSigCommitment(2, 2, roots_2of2);

    // Deterministic
    BOOST_CHECK(WOTS39::ComputeMultiSigCommitment(2, 2, roots_2of2) == c1);

    // Non-zero
    BOOST_CHECK(c1 != uint256{});

    // Different k → different commitment (same roots)
    uint256 c2 = WOTS39::ComputeMultiSigCommitment(1, 2, roots_2of2);
    BOOST_CHECK(c1 != c2);

    // Different n → different commitment (extend to 3 roots)
    uint256 addr2 = WOTS39::ComputeAddressNonce(msk, 2);
    uint256 root2 = WOTS39::ComputeMerkleRoot(msk, addr2);
    std::vector<uint256> roots_2of3 = {root0, root1, root2};
    uint256 c3 = WOTS39::ComputeMultiSigCommitment(2, 3, roots_2of3);
    BOOST_CHECK(c1 != c3);

    // Order of roots matters
    std::vector<uint256> roots_reversed = {root1, root0};
    BOOST_CHECK(WOTS39::ComputeMultiSigCommitment(2, 2, roots_reversed) != c1);

    // Multi-sig commitment is distinct from a single-sig merkle root
    BOOST_CHECK(c1 != root0);
    BOOST_CHECK(c1 != root1);

    BOOST_TEST_MESSAGE("2-of-2 commitment (msk=0x01*32, addr0, addr1): " << Hex(c1));
    BOOST_TEST_MESSAGE("1-of-2 commitment (different k): " << Hex(c2));
}

// ---------------------------------------------------------------------------
// Multi-sig — 2-of-2 sign/verify round-trip (Lightning-channel style)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_2of2_roundtrip)
{
    // Two participants each have their own msk and address.
    // They construct a shared 2-of-2 P2WOTS output.
    uint256 msk_a = FillUint256(0xAA);
    uint256 msk_b = FillUint256(0xBB);

    uint256 addr_a = WOTS39::ComputeAddressNonce(msk_a, 0);
    uint256 addr_b = WOTS39::ComputeAddressNonce(msk_b, 0);
    uint256 root_a = WOTS39::ComputeMerkleRoot(msk_a, addr_a);
    uint256 root_b = WOTS39::ComputeMerkleRoot(msk_b, addr_b);

    // Shared commitment
    std::vector<uint256> roots = {root_a, root_b};
    uint256 commitment = WOTS39::ComputeMultiSigCommitment(2, 2, roots);
    BOOST_CHECK(commitment != uint256{});

    // Each participant signs the same message using slot 0
    uint256 msg = FillUint256(0xCD);

    uint256 snonce_a = WOTS39::ComputeSlotNonce(msk_a, addr_a, 0);
    uint256 snonce_b = WOTS39::ComputeSlotNonce(msk_b, addr_b, 0);

    auto sig_a = WOTS39::Sign(msk_a, snonce_a, msg);
    auto sig_b = WOTS39::Sign(msk_b, snonce_b, msg);

    uint256 pk_a = WOTS39::GenerateWOTSPK(msk_a, snonce_a);
    uint256 pk_b = WOTS39::GenerateWOTSPK(msk_b, snonce_b);

    // Both signatures individually verify
    BOOST_CHECK(WOTS39::Verify(pk_a, snonce_a, msg, sig_a));
    BOOST_CHECK(WOTS39::Verify(pk_b, snonce_b, msg, sig_b));

    // Both PKs are members of their respective roots
    auto path_a = WOTS39::GenerateAuthPath(msk_a, addr_a, 0);
    auto path_b = WOTS39::GenerateAuthPath(msk_b, addr_b, 0);
    BOOST_CHECK(WOTS39::VerifyAuthPath(pk_a, 0, path_a, root_a));
    BOOST_CHECK(WOTS39::VerifyAuthPath(pk_b, 0, path_b, root_b));

    // Cross-signer: A's sig does NOT verify under B's PK / root
    BOOST_CHECK(!WOTS39::Verify(pk_b, snonce_b, msg, sig_a));
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk_a, 0, path_a, root_b));

    BOOST_TEST_MESSAGE("2-of-2 commitment: " << Hex(commitment));
    BOOST_TEST_MESSAGE("pk_a: " << Hex(pk_a));
    BOOST_TEST_MESSAGE("pk_b: " << Hex(pk_b));
}

// ---------------------------------------------------------------------------
// Multi-sig — 2-of-3 threshold round-trip
// (Any 2 of the 3 participants can co-sign; the third is not required)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_2of3_threshold)
{
    uint256 msk0 = FillUint256(0x10);
    uint256 msk1 = FillUint256(0x20);
    uint256 msk2 = FillUint256(0x30);

    uint256 addr0 = WOTS39::ComputeAddressNonce(msk0, 0);
    uint256 addr1 = WOTS39::ComputeAddressNonce(msk1, 0);
    uint256 addr2 = WOTS39::ComputeAddressNonce(msk2, 0);

    uint256 root0 = WOTS39::ComputeMerkleRoot(msk0, addr0);
    uint256 root1 = WOTS39::ComputeMerkleRoot(msk1, addr1);
    uint256 root2 = WOTS39::ComputeMerkleRoot(msk2, addr2);

    std::vector<uint256> all_roots = {root0, root1, root2};
    uint256 commitment = WOTS39::ComputeMultiSigCommitment(2, 3, all_roots);

    // Signers 0 and 2 cooperate (skipping signer 1)
    uint256 msg = FillUint256(0xEF);
    uint256 sn0 = WOTS39::ComputeSlotNonce(msk0, addr0, 0);
    uint256 sn2 = WOTS39::ComputeSlotNonce(msk2, addr2, 0);
    uint256 pk0 = WOTS39::GenerateWOTSPK(msk0, sn0);
    uint256 pk2 = WOTS39::GenerateWOTSPK(msk2, sn2);

    auto sig0 = WOTS39::Sign(msk0, sn0, msg);
    auto sig2 = WOTS39::Sign(msk2, sn2, msg);

    // Verify both participating signers
    BOOST_CHECK(WOTS39::Verify(pk0, sn0, msg, sig0));
    BOOST_CHECK(WOTS39::Verify(pk2, sn2, msg, sig2));

    // Their PKs belong to their respective roots (index 0 and 2 from the policy)
    auto ap0 = WOTS39::GenerateAuthPath(msk0, addr0, 0);
    auto ap2 = WOTS39::GenerateAuthPath(msk2, addr2, 0);
    BOOST_CHECK(WOTS39::VerifyAuthPath(pk0, 0, ap0, root0));
    BOOST_CHECK(WOTS39::VerifyAuthPath(pk2, 0, ap2, root2));

    // Non-participant signer 1's root is still in the commitment, but not signing
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk0, 0, ap0, root1));  // pk0 doesn't belong to root1

    // The commitment is distinct from any individual root
    BOOST_CHECK(commitment != root0);
    BOOST_CHECK(commitment != root1);
    BOOST_CHECK(commitment != root2);

    BOOST_TEST_MESSAGE("2-of-3 commitment: " << Hex(commitment));
}

// ---------------------------------------------------------------------------
// Multi-sig — wrong signer count (k-1 signatures) must not satisfy k-of-n
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_wrong_k_fails)
{
    uint256 msk_a = FillUint256(0xCA);
    uint256 msk_b = FillUint256(0xCB);

    uint256 addr_a = WOTS39::ComputeAddressNonce(msk_a, 0);
    uint256 addr_b = WOTS39::ComputeAddressNonce(msk_b, 0);
    uint256 root_a = WOTS39::ComputeMerkleRoot(msk_a, addr_a);
    uint256 root_b = WOTS39::ComputeMerkleRoot(msk_b, addr_b);

    std::vector<uint256> roots = {root_a, root_b};
    uint256 c22 = WOTS39::ComputeMultiSigCommitment(2, 2, roots);
    uint256 c12 = WOTS39::ComputeMultiSigCommitment(1, 2, roots);

    // A 2-of-2 commitment is distinct from a 1-of-2 commitment over the same roots.
    // This means a single-signer witness for a 2-of-2 output cannot be misinterpreted
    // as satisfying the policy (item-count + commitment mismatch both catch it).
    BOOST_CHECK(c22 != c12);

    // A 1-of-2 commitment is distinct from any single root
    BOOST_CHECK(c12 != root_a);
    BOOST_CHECK(c12 != root_b);
}

// ---------------------------------------------------------------------------
// Multi-sig — commitment changes when any root changes (key binding)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_root_binding)
{
    uint256 msk = FillUint256(0x77);
    uint256 addr0 = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 addr1 = WOTS39::ComputeAddressNonce(msk, 1);
    uint256 addr2 = WOTS39::ComputeAddressNonce(msk, 2);

    uint256 root0 = WOTS39::ComputeMerkleRoot(msk, addr0);
    uint256 root1 = WOTS39::ComputeMerkleRoot(msk, addr1);
    uint256 root2 = WOTS39::ComputeMerkleRoot(msk, addr2);

    std::vector<uint256> roots_orig    = {root0, root1};
    std::vector<uint256> roots_altered = {root0, root2};  // replace second signer

    uint256 c_orig    = WOTS39::ComputeMultiSigCommitment(2, 2, roots_orig);
    uint256 c_altered = WOTS39::ComputeMultiSigCommitment(2, 2, roots_altered);

    // Replacing one signer's root produces a completely different commitment
    BOOST_CHECK(c_orig != c_altered);

    // An attacker who substitutes their own root cannot produce the original commitment
    BOOST_CHECK(c_altered != c_orig);

    BOOST_TEST_MESSAGE("original 2-of-2 commitment:  " << Hex(c_orig));
    BOOST_TEST_MESSAGE("tampered 2-of-2 commitment:  " << Hex(c_altered));
}

// ---------------------------------------------------------------------------
// chain_up identity: chain_up(x, j, j, ...) must return x unchanged (0 steps)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(chain_up_zero_steps)
{
    uint256 x     = FillUint256(0x42);
    uint256 nonce = FillUint256(0x11);

    // Zero steps at any starting position must leave x unchanged
    BOOST_CHECK(WOTS39::ChainUp(x,   0,   0, nonce, 0) == x);
    BOOST_CHECK(WOTS39::ChainUp(x,   5,   5, nonce, 0) == x);
    BOOST_CHECK(WOTS39::ChainUp(x, 127, 127, nonce, 3) == x);
    BOOST_CHECK(WOTS39::ChainUp(x, 255, 255, nonce, 7) == x);

    // One step must change x (overwhelmingly likely)
    BOOST_CHECK(WOTS39::ChainUp(x, 0, 1, nonce, 0) != x);
}

// ---------------------------------------------------------------------------
// BaseWEncode — mid-range and boundary byte patterns
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(basew_encode_midrange)
{
    // 0x80*32: each byte = 128, checksum = 32*(255-128) = 4064 = 0x0FE0
    {
        auto d = WOTS39::BaseWEncode(FillUint256(0x80));
        BOOST_CHECK_EQUAL(d.size(), static_cast<size_t>(WOTS39::WOTS_L));
        for (int i = 0; i < 32; ++i) BOOST_CHECK_EQUAL(d[i], 128u);
        uint64_t enc = (static_cast<uint64_t>(d[32]) << 8) | d[33];
        BOOST_CHECK_EQUAL(enc, 32u * (255u - 128u));  // 4064
    }

    // 0x01*32: each byte = 1, checksum = 32*254 = 8128 = 0x1FC0
    {
        auto d = WOTS39::BaseWEncode(FillUint256(0x01));
        for (int i = 0; i < 32; ++i) BOOST_CHECK_EQUAL(d[i], 1u);
        uint64_t enc = (static_cast<uint64_t>(d[32]) << 8) | d[33];
        BOOST_CHECK_EQUAL(enc, 32u * 254u);  // 8128
    }

    // Checksum invariant: encoded checksum == computed checksum from message digits
    {
        auto d = WOTS39::BaseWEncode(FillUint256(0x37));
        uint64_t csum = 0;
        for (int i = 0; i < 32; ++i) csum += (255u - d[i]);
        uint64_t enc = (static_cast<uint64_t>(d[32]) << 8) | d[33];
        BOOST_CHECK_EQUAL(enc, csum);
    }
}

// ---------------------------------------------------------------------------
// GenerateAuthPath returns exactly WOTS_TREE_HEIGHT=6 nodes
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(auth_path_exact_length)
{
    uint256 msk        = FillUint256(0x22);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);

    // Sample every 7th slot to keep test fast
    for (uint64_t slot = 0; slot < WOTS39::WOTS_TREE_SLOTS; slot += 7) {
        auto path = WOTS39::GenerateAuthPath(msk, addr_nonce, slot);
        BOOST_CHECK_EQUAL(path.size(), static_cast<size_t>(WOTS39::WOTS_TREE_HEIGHT));
    }
}

// ---------------------------------------------------------------------------
// Slot 0 (all-left path) and slot 63 (all-right path) must both verify
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(slot_boundary_auth_paths)
{
    uint256 msk        = FillUint256(0x44);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 root       = WOTS39::ComputeMerkleRoot(msk, addr_nonce);

    // slot 0: all-left path in the tree
    {
        uint256 sn   = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
        uint256 pk   = WOTS39::GenerateWOTSPK(msk, sn);
        auto    path = WOTS39::GenerateAuthPath(msk, addr_nonce, 0);
        BOOST_CHECK( WOTS39::VerifyAuthPath(pk, 0, path, root));
        BOOST_CHECK(!WOTS39::VerifyAuthPath(pk, 1, path, root));  // wrong index
    }

    // slot 63: all-right path in the tree (0b111111)
    {
        uint64_t last = WOTS39::WOTS_TREE_SLOTS - 1;  // 63
        uint256 sn   = WOTS39::ComputeSlotNonce(msk, addr_nonce, last);
        uint256 pk   = WOTS39::GenerateWOTSPK(msk, sn);
        auto    path = WOTS39::GenerateAuthPath(msk, addr_nonce, last);
        BOOST_CHECK( WOTS39::VerifyAuthPath(pk, last, path, root));
        BOOST_CHECK(!WOTS39::VerifyAuthPath(pk, last - 1, path, root));  // wrong index
    }
}

// ---------------------------------------------------------------------------
// Two different messages signed with same key — cross-reuse must fail
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(sign_two_messages_cross_fail)
{
    uint256 msk        = FillUint256(0x33);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 pk         = WOTS39::GenerateWOTSPK(msk, nonce);

    uint256 msg1 = FillUint256(0xAA);
    uint256 msg2 = FillUint256(0xBB);

    auto sig1 = WOTS39::Sign(msk, nonce, msg1);
    auto sig2 = WOTS39::Sign(msk, nonce, msg2);

    // Each sig verifies for its own message
    BOOST_CHECK( WOTS39::Verify(pk, nonce, msg1, sig1));
    BOOST_CHECK( WOTS39::Verify(pk, nonce, msg2, sig2));

    // Cross-verification must fail
    BOOST_CHECK(!WOTS39::Verify(pk, nonce, msg2, sig1));
    BOOST_CHECK(!WOTS39::Verify(pk, nonce, msg1, sig2));

    // The two signatures must be distinct
    BOOST_CHECK(sig1 != sig2);
}

// ---------------------------------------------------------------------------
// Corrupting any single byte of any of the 34 sig elements must be detected
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(single_element_corruption_detected)
{
    uint256 msk        = FillUint256(0x55);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 pk         = WOTS39::GenerateWOTSPK(msk, nonce);
    uint256 msg        = FillUint256(0x77);

    auto sig = WOTS39::Sign(msk, nonce, msg);
    BOOST_CHECK(WOTS39::Verify(pk, nonce, msg, sig));  // baseline

    // Flip the first byte of each of the 34 sig elements in turn
    for (int elem = 0; elem < WOTS39::WOTS_L; ++elem) {
        auto bad = sig;
        bad[elem * 32] ^= 0x01;
        BOOST_CHECK_MESSAGE(!WOTS39::Verify(pk, nonce, msg, bad),
            "Corruption in element " << elem << " must be detected");
    }
}

// ---------------------------------------------------------------------------
// pk from address A cannot verify against root of address B (cross-address isolation)
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(cross_address_auth_path_isolation)
{
    uint256 msk = FillUint256(0x66);

    uint256 addr0 = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 addr1 = WOTS39::ComputeAddressNonce(msk, 1);
    uint256 root0 = WOTS39::ComputeMerkleRoot(msk, addr0);
    uint256 root1 = WOTS39::ComputeMerkleRoot(msk, addr1);

    BOOST_CHECK(root0 != root1);  // addresses produce distinct roots

    uint256 sn0   = WOTS39::ComputeSlotNonce(msk, addr0, 0);
    uint256 pk0   = WOTS39::GenerateWOTSPK(msk, sn0);
    auto    path0 = WOTS39::GenerateAuthPath(msk, addr0, 0);

    BOOST_CHECK( WOTS39::VerifyAuthPath(pk0, 0, path0, root0));  // correct
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk0, 0, path0, root1));  // wrong root

    uint256 sn1   = WOTS39::ComputeSlotNonce(msk, addr1, 0);
    uint256 pk1   = WOTS39::GenerateWOTSPK(msk, sn1);
    auto    path1 = WOTS39::GenerateAuthPath(msk, addr1, 0);

    BOOST_CHECK( WOTS39::VerifyAuthPath(pk1, 0, path1, root1));  // correct
    BOOST_CHECK(!WOTS39::VerifyAuthPath(pk1, 0, path1, root0));  // wrong root
}

// ---------------------------------------------------------------------------
// k-of-n degenerate case: k=1, n=1
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_degenerate_1of1)
{
    uint256 msk  = FillUint256(0x88);
    uint256 addr = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 root = WOTS39::ComputeMerkleRoot(msk, addr);

    uint256 c = WOTS39::ComputeMultiSigCommitment(1, 1, {root});

    BOOST_CHECK(c != uint256{});   // non-zero
    BOOST_CHECK(c != root);        // distinct from the raw root (domain separation)
    BOOST_CHECK(WOTS39::ComputeMultiSigCommitment(1, 1, {root}) == c);  // deterministic
}

// ---------------------------------------------------------------------------
// Multi-sig at maximum n=20 and k=20 works without crash or collision
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(multisig_max_n_20)
{
    uint256 msk = FillUint256(0x99);

    std::vector<uint256> roots20;
    for (int i = 0; i < WOTS39::WOTS_MULTISIG_MAX_N; ++i) {
        uint256 an = WOTS39::ComputeAddressNonce(msk, static_cast<uint64_t>(i));
        roots20.push_back(WOTS39::ComputeMerkleRoot(msk, an));
    }

    // k=n=20: all-must-sign commitment
    uint256 c_all = WOTS39::ComputeMultiSigCommitment(
        static_cast<uint8_t>(WOTS39::WOTS_MULTISIG_MAX_N),
        static_cast<uint8_t>(WOTS39::WOTS_MULTISIG_MAX_N),
        roots20);
    BOOST_CHECK(c_all != uint256{});

    // k=1, n=20: any-one-of-20 commitment must differ from all-must-sign
    uint256 c_any = WOTS39::ComputeMultiSigCommitment(
        1,
        static_cast<uint8_t>(WOTS39::WOTS_MULTISIG_MAX_N),
        roots20);
    BOOST_CHECK(c_any != c_all);
    BOOST_CHECK(c_any != uint256{});
}

// ---------------------------------------------------------------------------
// All-zero msk must still produce non-zero, non-degenerate outputs
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(zero_msk_nonces_nondegenerate)
{
    uint256 zero_msk{};  // all bytes 0x00

    uint256 an = WOTS39::ComputeAddressNonce(zero_msk, 0);
    BOOST_CHECK(an != uint256{});

    uint256 sn = WOTS39::ComputeSlotNonce(zero_msk, an, 0);
    BOOST_CHECK(sn != uint256{});
    BOOST_CHECK(sn != an);

    uint256 pk = WOTS39::GenerateWOTSPK(zero_msk, sn);
    BOOST_CHECK(pk != uint256{});

    // Must differ from a non-zero msk
    uint256 pk_nonzero = WOTS39::GenerateWOTSPK(
        FillUint256(0x01),
        WOTS39::ComputeSlotNonce(
            FillUint256(0x01),
            WOTS39::ComputeAddressNonce(FillUint256(0x01), 0), 0));
    BOOST_CHECK(pk != pk_nonzero);
}

// ---------------------------------------------------------------------------
// All 34 secret key elements for a slot must be mutually distinct
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(sk_independent_across_all_chains)
{
    uint256 msk        = FillUint256(0xAA);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);

    std::vector<uint256> sks;
    sks.reserve(WOTS39::WOTS_L);
    for (int j = 0; j < WOTS39::WOTS_L; ++j)
        sks.push_back(WOTS39::DeriveSK(msk, nonce, j));

    for (int i = 0; i < WOTS39::WOTS_L; ++i) {
        for (int j = i + 1; j < WOTS39::WOTS_L; ++j) {
            BOOST_CHECK_MESSAGE(sks[i] != sks[j],
                "sk[" << i << "] must differ from sk[" << j << "]");
        }
    }
}

// ---------------------------------------------------------------------------
// Sign output can be split into WOTS_L 32-byte elements and verify via stack form
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(sign_produces_valid_stack_form)
{
    uint256 msk        = FillUint256(0xBB);
    uint256 addr_nonce = WOTS39::ComputeAddressNonce(msk, 0);
    uint256 nonce      = WOTS39::ComputeSlotNonce(msk, addr_nonce, 0);
    uint256 msg        = FillUint256(0xCC);
    uint256 pk         = WOTS39::GenerateWOTSPK(msk, nonce);

    auto sig = WOTS39::Sign(msk, nonce, msg);
    BOOST_CHECK_EQUAL(sig.size(), static_cast<size_t>(WOTS39::WOTS_SIG_SIZE));  // 1088

    // Split into stack form
    std::vector<std::vector<uint8_t>> elems(WOTS39::WOTS_L);
    for (int i = 0; i < WOTS39::WOTS_L; ++i)
        elems[i].assign(sig.begin() + i * 32, sig.begin() + (i + 1) * 32);

    // Must verify via stack form
    BOOST_CHECK(WOTS39::VerifyFromStackElements(pk, nonce, msg, elems));

    // One extra element → fails
    elems.push_back(std::vector<uint8_t>(32, 0));
    BOOST_CHECK(!WOTS39::VerifyFromStackElements(pk, nonce, msg, elems));

    // One fewer element → fails
    elems.pop_back();
    elems.pop_back();
    BOOST_CHECK(!WOTS39::VerifyFromStackElements(pk, nonce, msg, elems));
}

BOOST_AUTO_TEST_SUITE_END()
