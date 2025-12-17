// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MUSIG_H
#define BITCOIN_MUSIG_H

#include <pubkey.h>

#include <optional>
#include <vector>

struct secp256k1_musig_keyagg_cache;
class MuSig2SecNonceImpl;
struct secp256k1_musig_secnonce;

//! MuSig2 chaincode as defined by BIP 328
using namespace util::hex_literals;
constexpr uint256 MUSIG_CHAINCODE{
    // Use immediate lambda to work around GCC-14 bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117966
    []() consteval { return uint256{"868087ca02a6f974c4598924c36b57762d32cb45717167e300622c7167e38965"_hex_u8}; }(),
};



constexpr size_t MUSIG2_PUBNONCE_SIZE{66};

//! Compute the full aggregate pubkey from the given participant pubkeys in their current order.
//! Outputs the secp256k1_musig_keyagg_cache and validates that the computed aggregate pubkey matches an expected aggregate pubkey.
//! This is necessary for most MuSig2 operations.
std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache, const std::optional<CPubKey>& expected_aggregate);
std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys);

//! Construct the BIP 328 synthetic xpub for a pubkey
CExtPubKey CreateMuSig2SyntheticXpub(const CPubKey& pubkey);

/**
 * MuSig2SecNonce encapsulates a secret nonce in use in a MuSig2 signing session.
 * Since this nonce persists outside of libsecp256k1 signing code, we must handle
 * its construction and destruction ourselves.
 * The secret nonce must be kept a secret, otherwise the private key may be leaked.
 * As such, it needs to be treated in the same way that CKeys are treated.
 * So this class handles the secure allocation of the secp256k1_musig_secnonce object
 * that libsecp256k1 uses, and only gives out references to this object to avoid
 * any possibility of copies being made. Furthermore, objects of this class are not
 * copyable to avoid nonce reuse.
*/
class MuSig2SecNonce
{
private:
    std::unique_ptr<MuSig2SecNonceImpl> m_impl;

public:
    MuSig2SecNonce();
    MuSig2SecNonce(MuSig2SecNonce&&) noexcept;
    MuSig2SecNonce& operator=(MuSig2SecNonce&&) noexcept;
    ~MuSig2SecNonce();

    // Delete copy constructors
    MuSig2SecNonce(const MuSig2SecNonce&) = delete;
    MuSig2SecNonce& operator=(const MuSig2SecNonce&) = delete;

    secp256k1_musig_secnonce* Get() const;
    void Invalidate();
    bool IsValid();
};

uint256 MuSig2SessionID(const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256& sighash);

std::optional<std::vector<uint8_t>> CreateMuSig2AggregateSig(const std::vector<CPubKey>& participants, const CPubKey& aggregate_pubkey, const std::vector<std::pair<uint256, bool>>& tweaks, const uint256& sighash, const std::map<CPubKey, std::vector<uint8_t>>& pubnonces, const std::map<CPubKey, uint256>& partial_sigs);

#endif // BITCOIN_MUSIG_H
