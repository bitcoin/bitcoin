// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SIGCACHE_H
#define BITCOIN_SCRIPT_SIGCACHE_H

#include <consensus/amount.h>
#include <cuckoocache.h>
#include <hash.h>
#include <script/interpreter.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/hasher.h>

#include <cstddef>
#include <cstdint>
#include <shared_mutex>
#include <span>
#include <vector>

class CPubKey;
class CTransaction;
class XOnlyPubKey;

// DoS prevention: limit cache size to 32MiB (over 1000000 entries on 64-bit
// systems). Due to how we count cache size, actual memory usage is slightly
// more (~32.25 MiB)
inline constexpr size_t DEFAULT_VALIDATION_CACHE_BYTES{32_MiB};
inline constexpr size_t DEFAULT_SIGNATURE_CACHE_BYTES{DEFAULT_VALIDATION_CACHE_BYTES / 2};
inline constexpr size_t DEFAULT_SCRIPT_EXECUTION_CACHE_BYTES{DEFAULT_VALIDATION_CACHE_BYTES / 2};
static_assert(DEFAULT_VALIDATION_CACHE_BYTES == DEFAULT_SIGNATURE_CACHE_BYTES + DEFAULT_SCRIPT_EXECUTION_CACHE_BYTES);

/**
 * Valid signature cache, to avoid doing expensive ECDSA signature checking
 * twice for every transaction (once when accepted into memory pool, and
 * again when accepted into the block chain)
 */
class SignatureCache
{
private:
    /** Entries are SHA256(nonce [32 bytes] || 'E' or 'S' || signature hash [32 bytes] || public key || signature), where the signature excludes the sighash type byte.
     * ECDSA keys and signatures serialize with CompactSize prefixes and x-only keys are fixed-size, so the key/signature boundary is unambiguous for any input.
     */
    const HashWriter m_salted_hasher_ecdsa;
    const HashWriter m_salted_hasher_schnorr;
    typedef CuckooCache::cache<uint256, SignatureCacheHasher> map_type;
    map_type setValid;
    std::shared_mutex cs_sigcache;

public:
    SignatureCache(size_t max_size_bytes);
    //! Salts entries with the given nonce instead of a random one, so tests can recompute them
    SignatureCache(size_t max_size_bytes, const uint256& nonce);

    SignatureCache(const SignatureCache&) = delete;
    SignatureCache& operator=(const SignatureCache&) = delete;

    uint256 ComputeEntryECDSA(const uint256& hash, const std::vector<uint8_t>& vchSig, const CPubKey& pubkey) const;

    uint256 ComputeEntrySchnorr(const uint256& hash, std::span<const uint8_t> sig, const XOnlyPubKey& pubkey) const;

    bool Get(const uint256& entry, bool erase);

    void Set(const uint256& entry);
};

class CachingTransactionSignatureChecker : public TransactionSignatureChecker
{
private:
    bool store;
    SignatureCache& m_signature_cache;

public:
    CachingTransactionSignatureChecker(const CTransaction* txToIn, unsigned int nInIn, const CAmount& amountIn, bool storeIn, SignatureCache& signature_cache, PrecomputedTransactionData& txdataIn) : TransactionSignatureChecker(txToIn, nInIn, amountIn, txdataIn, MissingDataBehavior::ASSERT_FAIL), store(storeIn), m_signature_cache(signature_cache)  {}

    bool VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& vchPubKey, const uint256& sighash) const override;
    bool VerifySchnorrSignature(std::span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const override;
};

#endif // BITCOIN_SCRIPT_SIGCACHE_H
