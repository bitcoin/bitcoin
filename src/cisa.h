// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CISA_H
#define BITCOIN_CISA_H

#include <pubkey.h>
#include <uint256.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class FullAggSecNonceImpl;
class KeyPair;
struct secp256k1_fullagg_secnonce;

inline constexpr size_t FULLAGG_PUBNONCE_SIZE{66};

/**
 * A BIP459 secret nonce of an ongoing full-aggregation signing session,
 * handled like MuSig2SecNonce: securely allocated and not copyable, since
 * reusing it leaks the private key.
 */
class FullAggSecNonce
{
private:
    std::unique_ptr<FullAggSecNonceImpl> m_impl;

public:
    FullAggSecNonce();
    FullAggSecNonce(FullAggSecNonce&&) noexcept;
    FullAggSecNonce& operator=(FullAggSecNonce&&) noexcept;
    ~FullAggSecNonce();

    FullAggSecNonce(const FullAggSecNonce&) = delete;
    FullAggSecNonce& operator=(const FullAggSecNonce&) = delete;

    secp256k1_fullagg_secnonce* Get() const;
    void Invalidate();
    bool IsValid();
};

/** Session id of a signer's full-aggregation session: SHA256 of the signing key and the pubnonce. */
uint256 CISASessionID(const XOnlyPubKey& pubkey, const std::vector<uint8_t>& pubnonce);

/** BIP459 NonceGen. The nonce commits to no message, so it can be generated before the
 *  transaction is known. Returns the serialized pubnonce, empty on failure. */
std::vector<uint8_t> CreateFullAggNonce(FullAggSecNonce& secnonce, const KeyPair& keypair);

/** BIP459 Sign for the signer at signer_index of the group. Invalidates secnonce. */
std::optional<uint256> CreateFullAggPartialSig(const KeyPair& keypair, FullAggSecNonce& secnonce, const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& pubnonces, size_t signer_index);

/** BIP459 PartialSigVerify for the signer at signer_index of the group. */
bool VerifyFullAggPartialSig(const uint256& partial_sig, const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& pubnonces, size_t signer_index);

/** BIP459 SigAgg, after verifying every partial signature. */
std::optional<std::vector<uint8_t>> AggregateFullAggSigs(const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& pubnonces, const std::vector<uint256>& partial_sigs);

/** BIP458 Aggregate of 64-byte BIP340 signatures, after verifying every signature. */
std::optional<std::vector<uint8_t>> AggregateHalfAggSigs(const std::vector<XOnlyPubKey>& pubkeys, const std::vector<uint256>& msgs, const std::vector<std::vector<uint8_t>>& sigs);

#endif // BITCOIN_CISA_H
