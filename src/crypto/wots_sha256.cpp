// Copyright (c) 2026 The WOTS-39 Authors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/wots_sha256.h>
#include <crypto/sha256.h>

#include <array>
#include <cstring>

namespace WOTS39 {

static std::array<unsigned char, 8> Uint64BE(uint64_t n)
{
    std::array<unsigned char, 8> buf{};
    for (int i = 7; i >= 0; --i) {
        buf[i] = static_cast<unsigned char>(n & 0xFF);
        n >>= 8;
    }
    return buf;
}

static uint256 XorUint256(const uint256& a, const uint256& b)
{
    uint256 result;
    for (int i = 0; i < 32; ++i)
        result.begin()[i] = a.begin()[i] ^ b.begin()[i];
    return result;
}

uint256 ComputeAddressNonce(const uint256& mskWOTS, uint64_t address_index)
{
    static const char domain[] = "wots39-addr-v1";
    auto ibytes = Uint64BE(address_index);

    CSHA256 h;
    h.Write(reinterpret_cast<const unsigned char*>(domain), 14);
    h.Write(mskWOTS.begin(), 32);
    h.Write(ibytes.data(), 8);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 ComputeSlotNonce(const uint256& mskWOTS, const uint256& address_nonce, uint64_t slot_index)
{
    static const char domain[] = "wots39-slot-v1";
    auto ibytes = Uint64BE(slot_index);

    CSHA256 h;
    h.Write(reinterpret_cast<const unsigned char*>(domain), 14);
    h.Write(mskWOTS.begin(), 32);
    h.Write(address_nonce.begin(), 32);
    h.Write(ibytes.data(), 8);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 WotsMerkleLeaf(const uint256& wots_pk)
{
    static const char domain[] = "wots39-leaf-v1";

    CSHA256 h;
    h.Write(reinterpret_cast<const unsigned char*>(domain), 14);
    h.Write(wots_pk.begin(), 32);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 WotsMerkleNode(const uint256& left, const uint256& right)
{
    static const char domain[] = "wots39-node-v1";

    CSHA256 h;
    h.Write(reinterpret_cast<const unsigned char*>(domain), 14);
    h.Write(left.begin(), 32);
    h.Write(right.begin(), 32);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 ComputeMerkleRoot(const uint256& mskWOTS, const uint256& address_nonce)
{
    std::array<uint256, WOTS_TREE_SLOTS> layer;
    for (int i = 0; i < WOTS_TREE_SLOTS; ++i) {
        uint256 slot_nonce = ComputeSlotNonce(mskWOTS, address_nonce, static_cast<uint64_t>(i));
        uint256 pk = GenerateWOTSPK(mskWOTS, slot_nonce);
        layer[i] = WotsMerkleLeaf(pk);
    }

    int width = WOTS_TREE_SLOTS;
    while (width > 1) {
        for (int i = 0; i < width / 2; ++i)
            layer[i] = WotsMerkleNode(layer[i * 2], layer[i * 2 + 1]);
        width /= 2;
    }
    return layer[0];
}

std::array<uint256, WOTS_TREE_HEIGHT> GenerateAuthPath(
    const uint256& mskWOTS,
    const uint256& address_nonce,
    uint64_t slot_index)
{
    std::array<uint256, WOTS_TREE_SLOTS> layer;
    for (int i = 0; i < WOTS_TREE_SLOTS; ++i) {
        uint256 slot_nonce = ComputeSlotNonce(mskWOTS, address_nonce, static_cast<uint64_t>(i));
        uint256 pk = GenerateWOTSPK(mskWOTS, slot_nonce);
        layer[i] = WotsMerkleLeaf(pk);
    }

    std::array<uint256, WOTS_TREE_HEIGHT> auth_path;
    uint64_t idx = slot_index;
    int width = WOTS_TREE_SLOTS;

    for (int level = 0; level < WOTS_TREE_HEIGHT; ++level) {
        uint64_t sibling = idx ^ 1;
        auth_path[level] = layer[sibling];

        for (int i = 0; i < width / 2; ++i)
            layer[i] = WotsMerkleNode(layer[i * 2], layer[i * 2 + 1]);
        width /= 2;
        idx >>= 1;
    }
    return auth_path;
}

bool VerifyAuthPath(
    const uint256& wots_pk,
    uint64_t slot_index,
    const std::array<uint256, WOTS_TREE_HEIGHT>& auth_path,
    const uint256& merkle_root)
{
    uint256 node = WotsMerkleLeaf(wots_pk);
    uint64_t idx = slot_index;

    for (int level = 0; level < WOTS_TREE_HEIGHT; ++level) {
        if ((idx & 1) == 0)
            node = WotsMerkleNode(node, auth_path[level]);
        else
            node = WotsMerkleNode(auth_path[level], node);
        idx >>= 1;
    }
    return node == merkle_root;
}

uint256 ComputeMultiSigCommitment(uint8_t k, uint8_t n, const std::vector<uint256>& merkle_roots)
{
    static const char domain[] = "wots39-multisig-v1";

    CSHA256 h;
    h.Write(reinterpret_cast<const unsigned char*>(domain), 18);
    h.Write(&k, 1);
    h.Write(&n, 1);
    for (const auto& root : merkle_roots)
        h.Write(root.begin(), 32);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 ComputeMsgHash(const uint256& sighash)
{
    static const char domain[] = "wots39-msg-v1";

    CSHA256 h;
    h.Write(reinterpret_cast<const unsigned char*>(domain), 13);
    h.Write(sighash.begin(), 32);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 DeriveSK(const uint256& mskWOTS, const uint256& nonce, uint64_t i)
{
    static const char domain[] = "wots-sk-v1";
    auto ibytes = Uint64BE(i);

    CSHA256 h;
    h.Write(mskWOTS.begin(), 32);
    h.Write(reinterpret_cast<const unsigned char*>(domain), 10);
    h.Write(nonce.begin(), 32);
    h.Write(ibytes.data(), 8);
    uint256 result;
    h.Finalize(result.begin());
    return result;
}

uint256 ChainStep(const uint256& x, uint64_t j,
                  const uint256& nonce, uint64_t chainIdx)
{
    static const char bm_domain[]  = "wots-bm-v1--";
    static const char key_domain[] = "wots-key-v1";

    auto idxBytes = Uint64BE(chainIdx);
    auto jBytes   = Uint64BE(j);

    uint256 bitmask;
    {
        CSHA256 bmh;
        bmh.Write(reinterpret_cast<const unsigned char*>(bm_domain), 12);
        bmh.Write(nonce.begin(), 32);
        bmh.Write(idxBytes.data(), 8);
        bmh.Write(jBytes.data(), 8);
        bmh.Finalize(bitmask.begin());
    }

    uint256 key;
    {
        CSHA256 kh;
        kh.Write(reinterpret_cast<const unsigned char*>(key_domain), 11);
        kh.Write(nonce.begin(), 32);
        kh.Write(idxBytes.data(), 8);
        kh.Write(jBytes.data(), 8);
        kh.Finalize(key.begin());
    }

    uint256 xored = XorUint256(x, bitmask);
    CSHA256 rh;
    rh.Write(key.begin(), 32);
    rh.Write(xored.begin(), 32);
    uint256 result;
    rh.Finalize(result.begin());
    return result;
}

uint256 ChainUp(const uint256& x, uint64_t from, uint64_t to,
                const uint256& nonce, uint64_t chainIdx)
{
    uint256 current = x;
    for (uint64_t j = from; j < to; ++j) {
        current = ChainStep(current, j, nonce, chainIdx);
    }
    return current;
}

std::array<uint8_t, WOTS_L> BaseWEncode(const uint256& msgHash)
{
    std::array<uint8_t, WOTS_L> digits{};

    for (int i = 0; i < 32; ++i)
        digits[i] = msgHash.begin()[i];

    uint64_t csum = 0;
    for (int i = 0; i < 32; ++i) csum += (255 - digits[i]);

    digits[32] = static_cast<uint8_t>((csum >> 8) & 0xFF);
    digits[33] = static_cast<uint8_t>( csum        & 0xFF);

    return digits;
}

uint256 GenerateWOTSPK(const uint256& mskWOTS, const uint256& nonce)
{
    CSHA256 h;
    for (int i = 0; i < WOTS_L; ++i) {
        uint256 sk  = DeriveSK(mskWOTS, nonce, static_cast<uint64_t>(i));
        uint256 tip = ChainUp(sk, 0, WOTS_W - 1, nonce, static_cast<uint64_t>(i));
        h.Write(tip.begin(), 32);
    }
    uint256 wotsPK;
    h.Finalize(wotsPK.begin());
    return wotsPK;
}

std::vector<uint8_t> Sign(const uint256& mskWOTS,
                           const uint256& nonce,
                           const uint256& msgHash)
{
    auto digits = BaseWEncode(msgHash);
    std::vector<uint8_t> sig;
    sig.reserve(WOTS_SIG_SIZE);

    for (int i = 0; i < WOTS_L; ++i) {
        uint256 sk   = DeriveSK(mskWOTS, nonce, static_cast<uint64_t>(i));
        uint256 elem = ChainUp(sk, 0, digits[i], nonce, static_cast<uint64_t>(i));
        sig.insert(sig.end(), elem.begin(), elem.end());
    }
    return sig;
}

bool Verify(const uint256& wotsPK,
            const uint256& nonce,
            const uint256& msgHash,
            const std::vector<uint8_t>& sig)
{
    if (sig.size() != static_cast<size_t>(WOTS_SIG_SIZE)) return false;

    auto digits = BaseWEncode(msgHash);
    CSHA256 h;

    for (int i = 0; i < WOTS_L; ++i) {
        uint256 elem;
        memcpy(elem.begin(), sig.data() + i * 32, 32);
        uint256 tip = ChainUp(elem, digits[i], WOTS_W - 1, nonce, static_cast<uint64_t>(i));
        h.Write(tip.begin(), 32);
    }

    uint256 reconstructed;
    h.Finalize(reconstructed.begin());
    return reconstructed == wotsPK;
}

bool VerifyFromStackElements(
    const uint256& wotsPK,
    const uint256& nonce,
    const uint256& msgHash,
    const std::vector<std::vector<uint8_t>>& sigElements)
{
    if (sigElements.size() != static_cast<size_t>(WOTS_L)) return false;

    auto digits = BaseWEncode(msgHash);
    CSHA256 h;

    for (int i = 0; i < WOTS_L; ++i) {
        if (sigElements[i].size() != 32) return false;
        uint256 elem;
        memcpy(elem.begin(), sigElements[i].data(), 32);
        uint256 tip = ChainUp(elem, digits[i], WOTS_W - 1, nonce, static_cast<uint64_t>(i));
        h.Write(tip.begin(), 32);
    }

    uint256 reconstructed;
    h.Finalize(reconstructed.begin());
    return reconstructed == wotsPK;
}

} // namespace WOTS39
