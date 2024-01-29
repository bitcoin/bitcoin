// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MUSIG_H
#define BITCOIN_MUSIG_H

#include <pubkey.h>

#include <vector>

struct secp256k1_musig_keyagg_cache;

class MuSig2SecNonceImpl;
struct secp256k1_musig_secnonce;

bool GetMuSig2KeyAggCache(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache);
std::optional<CPubKey> GetCPubKeyFromMuSig2KeyAggCache(secp256k1_musig_keyagg_cache& cache);
std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys);

/**
 * MuSig2SecNonce encapsulates a secret nonce in use in a MuSig2 signing session.
 * Since this nonce persists outside of libsecp256k1 signing code, we must handle
 * its construction and destruction ourselves.
 * The secret nonce must be kept a secret, otherwise the private key may be leaked.
 * As such, it needs to be treated in the same way that CKeys are treated.
 * So this class handles the secure allocation of the secp25k1_musig_secnonce object
 * that libsecp256k1 uses, and only gives out references to this object to avoid
 * any possibility of copies being made. Furthermore, objects of this class are not
 * copyable to avoid nonce reuse.
*/
class MuSig2SecNonce
{
private:
    //! The actual secnonce itself
    secure_unique_ptr<secp256k1_musig_secnonce> m_nonce;

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

std::optional<std::vector<uint8_t>> CreateMuSig2AggregateSig(const std::vector<CPubKey>& participants, const CPubKey& aggregate_pubkey, const std::vector<std::pair<uint256, bool>>& tweaks, const uint256& sighash, const std::map<CPubKey, std::vector<uint8_t>>& pubnonces, const std::map<CPubKey, uint256>& partial_sigs);

#endif // BITCOIN_MUSIG_H
