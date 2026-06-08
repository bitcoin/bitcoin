// Copyright (c) 2026 The WOTS-39 Authors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_WOTS_SHA256_H
#define BITCOIN_CRYPTO_WOTS_SHA256_H

#include <array>
#include <cstdint>
#include <vector>

#include <uint256.h>

namespace WOTS39 {

static constexpr int WOTS_N        = 32;
static constexpr int WOTS_W        = 256;
static constexpr int WOTS_L1       = 32;
static constexpr int WOTS_L2       = 2;
static constexpr int WOTS_L        = 34;
static constexpr int WOTS_SIG_SIZE = 34 * 32;

static constexpr int WOTS_TREE_HEIGHT = 6;
static constexpr int WOTS_TREE_SLOTS  = 64;

static constexpr int WOTS_WITNESS_ITEMS = WOTS_L + 1 + 1 + WOTS_TREE_HEIGHT;

static constexpr int WOTS_MULTISIG_ITEMS_PER_SIGNER = 1 + WOTS_L + 1 + 1 + WOTS_TREE_HEIGHT;
static constexpr int WOTS_MULTISIG_MAX_N = 20;

uint256 ComputeAddressNonce(const uint256& mskWOTS, uint64_t address_index);
uint256 ComputeSlotNonce(const uint256& mskWOTS, const uint256& address_nonce, uint64_t slot_index);

uint256 WotsMerkleLeaf(const uint256& wots_pk);
uint256 WotsMerkleNode(const uint256& left, const uint256& right);
uint256 ComputeMerkleRoot(const uint256& mskWOTS, const uint256& address_nonce);

std::array<uint256, WOTS_TREE_HEIGHT> GenerateAuthPath(
    const uint256& mskWOTS,
    const uint256& address_nonce,
    uint64_t slot_index);

bool VerifyAuthPath(
    const uint256& wots_pk,
    uint64_t slot_index,
    const std::array<uint256, WOTS_TREE_HEIGHT>& auth_path,
    const uint256& merkle_root);

uint256 ComputeMultiSigCommitment(uint8_t k, uint8_t n, const std::vector<uint256>& merkle_roots);

uint256 ComputeMsgHash(const uint256& sighash);
uint256 DeriveSK(const uint256& mskWOTS, const uint256& nonce, uint64_t i);

uint256 ChainStep(const uint256& x, uint64_t j,
                  const uint256& nonce, uint64_t chainIdx);

uint256 ChainUp(const uint256& x, uint64_t from, uint64_t to,
                const uint256& nonce, uint64_t chainIdx);

std::array<uint8_t, WOTS_L> BaseWEncode(const uint256& msgHash);
uint256 GenerateWOTSPK(const uint256& mskWOTS, const uint256& nonce);

std::vector<uint8_t> Sign(const uint256& mskWOTS,
                           const uint256& nonce,
                           const uint256& msgHash);

bool Verify(const uint256& wotsPK,
            const uint256& nonce,
            const uint256& msgHash,
            const std::vector<uint8_t>& sig);

bool VerifyFromStackElements(
    const uint256& wotsPK,
    const uint256& nonce,
    const uint256& msgHash,
    const std::vector<std::vector<uint8_t>>& sigElements);

inline uint256 ComputeWotsNonce(const uint256& mskWOTS, uint64_t index) {
    return ComputeAddressNonce(mskWOTS, index);
}

} // namespace WOTS39

#endif // BITCOIN_CRYPTO_WOTS_SHA256_H
