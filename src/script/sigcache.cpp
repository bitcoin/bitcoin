// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/sigcache.h>

#include <hash.h>
#include <pubkey.h>
#include <random.h>
#include <script/interpreter.h>
#include <uint256.h>
#include <util/log.h>

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <utility>
#include <vector>

SignatureCache::SignatureCache(const size_t max_size_bytes) : SignatureCache{max_size_bytes, GetRandHash()} {}

SignatureCache::SignatureCache(const size_t max_size_bytes, const uint256& nonce)
    : m_salted_hasher_ecdsa{HashWriter{} << nonce << uint8_t{'E'}},
      m_salted_hasher_schnorr{HashWriter{} << nonce << uint8_t{'S'}}
{
    const auto [num_elems, approx_size_bytes] = setValid.setup_bytes(max_size_bytes);
    LogInfo("Using %zu MiB out of %zu MiB requested for signature cache, able to store %zu elements",
              approx_size_bytes >> 20, max_size_bytes >> 20, num_elems);
}

uint256 SignatureCache::ComputeEntryECDSA(const uint256& hash, const std::vector<uint8_t>& vchSig, const CPubKey& pubkey) const
{
    return (HashWriter{m_salted_hasher_ecdsa} << hash << std::span{pubkey} << std::span{vchSig}).GetSHA256();
}

uint256 SignatureCache::ComputeEntrySchnorr(const uint256& hash, std::span<const uint8_t> sig, const XOnlyPubKey& pubkey) const
{
    return (HashWriter{m_salted_hasher_schnorr} << hash << std::span{pubkey} << sig).GetSHA256();
}

bool SignatureCache::Get(const uint256& entry, const bool erase)
{
    std::shared_lock<std::shared_mutex> lock(cs_sigcache);
    return setValid.contains(entry, erase);
}

void SignatureCache::Set(const uint256& entry)
{
    std::unique_lock<std::shared_mutex> lock(cs_sigcache);
    setValid.insert(entry);
}

bool CachingTransactionSignatureChecker::VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const
{
    const uint256 entry{m_signature_cache.ComputeEntryECDSA(sighash, vchSig, pubkey)};
    if (m_signature_cache.Get(entry, !store))
        return true;
    if (!TransactionSignatureChecker::VerifyECDSASignature(vchSig, pubkey, sighash))
        return false;
    if (store)
        m_signature_cache.Set(entry);
    return true;
}

bool CachingTransactionSignatureChecker::VerifySchnorrSignature(std::span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const
{
    const uint256 entry{m_signature_cache.ComputeEntrySchnorr(sighash, sig, pubkey)};
    if (m_signature_cache.Get(entry, !store)) return true;
    if (!TransactionSignatureChecker::VerifySchnorrSignature(sig, pubkey, sighash)) return false;
    if (store) m_signature_cache.Set(entry);
    return true;
}
