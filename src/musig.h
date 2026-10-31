// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MUSIG_H
#define BITCOIN_MUSIG_H

#include <pubkey.h>
#include <uint256.h>

#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <vector>

class CKey;
struct secp256k1_musig_keyagg_cache;
class MuSig2SecNonceImpl;
struct secp256k1_musig_secnonce;

inline constexpr size_t MUSIG2_PUBNONCE_SIZE{66};

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

/**
 * The secret nonces of the MuSig2 signing sessions that have not finished yet, by session ID.
 * A session that is never finished would stay forever, so at most MAX_SESSIONS are kept:
 * adding one more drops the oldest. Dropping a nonce never allows it to be reused.
 * While our pubnonce is in the PSBT, no partial signature and no new nonce is made for a
 * dropped session, so it has to restart from a PSBT without our pubnonce.
 * The limit is above the number of key-path inputs of a standard transaction (about 1700),
 * and the unfinished sessions of concurrent PSBTs share it.
 */
class MuSig2SecNonces
{
private:
    std::map<uint256, MuSig2SecNonce> m_nonces;
    //! Session IDs in the map, oldest first
    std::deque<uint256> m_order;

public:
    static constexpr size_t MAX_SESSIONS{2048};

    //! Add the nonce of a new session. Returns false and keeps the nonce already stored if the session ID exists.
    bool Add(const uint256& session_id, MuSig2SecNonce&& nonce);
    //! The nonce of a session, or nullptr if there is none.
    MuSig2SecNonce* Get(const uint256& session_id);
    //! Remove the nonce of a session, if there is one.
    void Remove(const uint256& session_id);
    size_t Size() const { return m_nonces.size(); }
};

/**
 * Computes an arbitrary unique session ID to identify ongoing signing sessions.
 * It is the SHA256 of the signing (aggregate) pubkey, the participant pubkey, the sighash, and the pubnonce
 */
uint256 MuSig2SessionID(const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256& sighash, const std::vector<uint8_t>& pubnonce);

std::vector<uint8_t> CreateMuSig2Nonce(MuSig2SecNonce& secnonce, const uint256& sighash, const CKey& our_seckey, const CPubKey& aggregate_pubkey, const std::vector<CPubKey>& pubkeys);
std::optional<uint256> CreateMuSig2PartialSig(const uint256& hash, const CKey& our_seckey, const CPubKey& aggregate_pubkey, const std::vector<CPubKey>& pubkeys, const std::map<CPubKey, std::vector<uint8_t>>& pubnonces, MuSig2SecNonce& secnonce, const std::vector<std::pair<uint256, bool>>& tweaks);
std::optional<std::vector<uint8_t>> CreateMuSig2AggregateSig(const std::vector<CPubKey>& participants, const CPubKey& aggregate_pubkey, const std::vector<std::pair<uint256, bool>>& tweaks, const uint256& sighash, const std::map<CPubKey, std::vector<uint8_t>>& pubnonces, const std::map<CPubKey, uint256>& partial_sigs);

#endif // BITCOIN_MUSIG_H
