// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <musig.h>
#include <support/allocators/secure.h>

#include <secp256k1_musig.h>

static bool GetMuSig2KeyAggCache(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache)
{
    // Parse the pubkeys
    std::vector<secp256k1_pubkey> secp_pubkeys;
    std::vector<const secp256k1_pubkey*> pubkey_ptrs;
    for (const CPubKey& pubkey : pubkeys) {
        if (!secp256k1_ec_pubkey_parse(secp256k1_context_static, &secp_pubkeys.emplace_back(), pubkey.data(), pubkey.size())) {
            return false;
        }
    }
    pubkey_ptrs.reserve(secp_pubkeys.size());
    for (const secp256k1_pubkey& p : secp_pubkeys) {
        pubkey_ptrs.push_back(&p);
    }

    // Aggregate the pubkey
    if (!secp256k1_musig_pubkey_agg(secp256k1_context_static, nullptr, &keyagg_cache, pubkey_ptrs.data(), pubkey_ptrs.size())) {
        return false;
    }
    return true;
}

static std::optional<CPubKey> GetCPubKeyFromMuSig2KeyAggCache(secp256k1_musig_keyagg_cache& keyagg_cache)
{
    // Get the plain aggregated pubkey
    secp256k1_pubkey agg_pubkey;
    if (!secp256k1_musig_pubkey_get(secp256k1_context_static, &agg_pubkey, &keyagg_cache)) {
        return std::nullopt;
    }

    // Turn into CPubKey
    unsigned char ser_agg_pubkey[CPubKey::COMPRESSED_SIZE];
    size_t ser_agg_pubkey_len = CPubKey::COMPRESSED_SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_static, ser_agg_pubkey, &ser_agg_pubkey_len, &agg_pubkey, SECP256K1_EC_COMPRESSED);
    return CPubKey(ser_agg_pubkey, ser_agg_pubkey + ser_agg_pubkey_len);
}

std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache, const std::optional<CPubKey>& expected_aggregate)
{
    if (!GetMuSig2KeyAggCache(pubkeys, keyagg_cache)) {
        return std::nullopt;
    }
    std::optional<CPubKey> agg_key = GetCPubKeyFromMuSig2KeyAggCache(keyagg_cache);
    if (!agg_key.has_value()) return std::nullopt;
    if (expected_aggregate.has_value() && expected_aggregate != agg_key) return std::nullopt;
    return agg_key;
}

std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys)
{
    secp256k1_musig_keyagg_cache keyagg_cache;
    return MuSig2AggregatePubkeys(pubkeys, keyagg_cache, std::nullopt);
}

CExtPubKey CreateMuSig2SyntheticXpub(const CPubKey& pubkey)
{
    CExtPubKey extpub;
    extpub.nDepth = 0;
    std::memset(extpub.vchFingerprint, 0, 4);
    extpub.nChild = 0;
    extpub.chaincode = MUSIG_CHAINCODE;
    extpub.pubkey = pubkey;
    return extpub;
}

class MuSig2SecNonceImpl
{
private:
    //! The actual secnonce itself
    secure_unique_ptr<secp256k1_musig_secnonce> m_nonce;

public:
    MuSig2SecNonceImpl() : m_nonce{make_secure_unique<secp256k1_musig_secnonce>()} {}

    // Delete copy constructors
    MuSig2SecNonceImpl(const MuSig2SecNonceImpl&) = delete;
    MuSig2SecNonceImpl& operator=(const MuSig2SecNonceImpl&) = delete;

    secp256k1_musig_secnonce* Get() const { return m_nonce.get(); }
    void Invalidate() { m_nonce.reset(); }
    bool IsValid() { return m_nonce != nullptr; }
};

MuSig2SecNonce::MuSig2SecNonce() : m_impl{std::make_unique<MuSig2SecNonceImpl>()} {}

MuSig2SecNonce::MuSig2SecNonce(MuSig2SecNonce&&) noexcept = default;
MuSig2SecNonce& MuSig2SecNonce::operator=(MuSig2SecNonce&&) noexcept = default;

MuSig2SecNonce::~MuSig2SecNonce() = default;

secp256k1_musig_secnonce* MuSig2SecNonce::Get() const
{
    return m_impl->Get();
}

void MuSig2SecNonce::Invalidate()
{
    return m_impl->Invalidate();
}

bool MuSig2SecNonce::IsValid()
{
    return m_impl->IsValid();
}

uint256 MuSig2SessionID(const CPubKey& script_pubkey, const CPubKey& part_pubkey, const uint256& sighash)
{
    HashWriter hasher;
    hasher << script_pubkey << part_pubkey << sighash;
    return hasher.GetSHA256();
}
