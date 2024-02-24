// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>

#include <random.h>

#ifndef BUILD_BITCOIN_INTERNAL
#include <support/allocators/mt_pooled_secure.h>
#endif

#include <cassert>
#include <cstring>

namespace bls {
    std::atomic<bool> bls_legacy_scheme = std::atomic<bool>(true);
}

static const std::unique_ptr<bls::CoreMPL> pSchemeLegacy{std::make_unique<bls::LegacySchemeMPL>()};
static const std::unique_ptr<bls::CoreMPL> pScheme(std::make_unique<bls::BasicSchemeMPL>());

static const std::unique_ptr<bls::CoreMPL>& Scheme(const bool fLegacy)
{
    return fLegacy ? pSchemeLegacy : pScheme;
}

CBLSId::CBLSId(const uint256& nHash) : CBLSWrapper<CBLSIdImplicit, BLS_CURVE_ID_SIZE, CBLSId>()
{
    impl = nHash;
    fValid = true;
    cachedHash.SetNull();
}

void CBLSSecretKey::AggregateInsecure(const CBLSSecretKey& o)
{
    assert(IsValid() && o.IsValid());
    impl = bls::PrivateKey::Aggregate({impl, o.impl});
    cachedHash.SetNull();
}

CBLSSecretKey CBLSSecretKey::AggregateInsecure(Span<CBLSSecretKey> sks)
{
    if (sks.empty()) {
        return {};
    }

    std::vector<bls::PrivateKey> v;
    v.reserve(sks.size());
    for (const auto& sk : sks) {
        v.emplace_back(sk.impl);
    }

    CBLSSecretKey ret;
    ret.impl = bls::PrivateKey::Aggregate(v);
    ret.fValid = true;
    ret.cachedHash.SetNull();
    return ret;
}

#ifndef BUILD_BITCOIN_INTERNAL
void CBLSSecretKey::MakeNewKey()
{
    unsigned char buf[SerSize];
    while (true) {
        GetStrongRandBytes(buf, sizeof(buf));
        try {
            impl = bls::PrivateKey::FromBytes(bls::Bytes(reinterpret_cast<const uint8_t*>(buf), SerSize));
            break;
        } catch (...) {
        }
    }
    fValid = true;
    cachedHash.SetNull();
}
#endif

bool CBLSSecretKey::SecretKeyShare(Span<CBLSSecretKey> msk, const CBLSId& _id)
{
    fValid = false;
    cachedHash.SetNull();

    if (!_id.IsValid()) {
        return false;
    }

    std::vector<bls::PrivateKey> mskVec;
    mskVec.reserve(msk.size());
    for (const CBLSSecretKey& sk : msk) {
        if (!sk.IsValid()) {
            return false;
        }
        mskVec.emplace_back(sk.impl);
    }

    try {
        impl = bls::Threshold::PrivateKeyShare(mskVec, bls::Bytes(_id.impl.begin(), _id.impl.size()));
    } catch (...) {
        return false;
    }

    fValid = true;
    cachedHash.SetNull();
    return true;
}

CBLSPublicKey CBLSSecretKey::GetPublicKey() const
{
    if (!IsValid()) {
        return {};
    }

    CBLSPublicKey pubKey;
    pubKey.impl = impl.GetG1Element();
    pubKey.fValid = true;
    pubKey.cachedHash.SetNull();
    return pubKey;
}

CBLSSignature CBLSSecretKey::Sign(const uint256& hash) const
{
    return Sign(hash, bls::bls_legacy_scheme.load());
}

CBLSSignature CBLSSecretKey::Sign(const uint256& hash, const bool specificLegacyScheme) const
{
    if (!IsValid()) {
        return {};
    }

    CBLSSignature sigRet;
    try {
        sigRet.impl = Scheme(specificLegacyScheme)->Sign(impl, bls::Bytes(hash.begin(), hash.size()));
        sigRet.fValid = true;
    } catch (...) {
        sigRet.fValid = false;
    }

    sigRet.cachedHash.SetNull();

    return sigRet;
}

void CBLSPublicKey::AggregateInsecure(const CBLSPublicKey& o)
{
    assert(IsValid() && o.IsValid());
    try {
        impl = Scheme(bls::bls_legacy_scheme.load())->Aggregate({impl, o.impl});
    } catch (...) {
        fValid = false;
    }
    cachedHash.SetNull();
}

CBLSPublicKey CBLSPublicKey::AggregateInsecure(Span<CBLSPublicKey> pks)
{
    if (pks.empty()) {
        return {};
    }

    std::vector<bls::G1Element> vecPublicKeys;
    vecPublicKeys.reserve(pks.size());
    for (const auto& pk : pks) {
        vecPublicKeys.emplace_back(pk.impl);
    }

    CBLSPublicKey ret;
    try {
        ret.impl = Scheme(bls::bls_legacy_scheme.load())->Aggregate(vecPublicKeys);
        ret.fValid = true;
    } catch (...) {
        ret.fValid = false;
    }

    ret.cachedHash.SetNull();
    return ret;
}

bool CBLSPublicKey::PublicKeyShare(Span<CBLSPublicKey> mpk, const CBLSId& _id)
{
    fValid = false;
    cachedHash.SetNull();

    if (!_id.IsValid()) {
        return false;
    }

    std::vector<bls::G1Element> mpkVec;
    mpkVec.reserve(mpk.size());
    for (const CBLSPublicKey& pk : mpk) {
        if (!pk.IsValid()) {
            return false;
        }
        mpkVec.emplace_back(pk.impl);
    }

    try {
        impl = bls::Threshold::PublicKeyShare(mpkVec, bls::Bytes(_id.impl.begin(), _id.impl.size()));
    } catch (...) {
        return false;
    }

    fValid = true;
    cachedHash.SetNull();
    return true;
}

bool CBLSPublicKey::DHKeyExchange(const CBLSSecretKey& sk, const CBLSPublicKey& pk)
{
    fValid = false;
    cachedHash.SetNull();

    if (!sk.IsValid() || !pk.IsValid()) {
        return false;
    }
    impl = sk.impl * pk.impl;
    fValid = true;
    cachedHash.SetNull();
    return true;
}

void CBLSSignature::AggregateInsecure(const CBLSSignature& o)
{
    assert(IsValid() && o.IsValid());
    try {
        impl = Scheme(bls::bls_legacy_scheme.load())->Aggregate({impl, o.impl});
    } catch (...) {
        fValid = false;
    }
    cachedHash.SetNull();
}

CBLSSignature CBLSSignature::AggregateInsecure(Span<CBLSSignature> sigs)
{
    if (sigs.empty()) {
        return {};
    }

    std::vector<bls::G2Element> v;
    v.reserve(sigs.size());
    for (const auto& pk : sigs) {
        v.emplace_back(pk.impl);
    }

    CBLSSignature ret;
    try {
        ret.impl = Scheme(bls::bls_legacy_scheme.load())->Aggregate(v);
        ret.fValid = true;
    } catch (...) {
        ret.fValid = false;
    }

    ret.cachedHash.SetNull();
    return ret;
}

CBLSSignature CBLSSignature::AggregateSecure(Span<CBLSSignature> sigs,
                                             Span<CBLSPublicKey> pks,
                                             const uint256& hash)
{
    if (sigs.size() != pks.size() || sigs.empty()) {
        return {};
    }

    std::vector<bls::G1Element> vecPublicKeys;
    vecPublicKeys.reserve(pks.size());
    for (const auto& pk : pks) {
        vecPublicKeys.push_back(pk.impl);
    }

    std::vector<bls::G2Element> vecSignatures;
    vecSignatures.reserve(pks.size());
    for (const auto& sig : sigs) {
        vecSignatures.push_back(sig.impl);
    }

    CBLSSignature ret;
    try {
        ret.impl = Scheme(bls::bls_legacy_scheme.load())->AggregateSecure(vecPublicKeys, vecSignatures, bls::Bytes(hash.begin(), hash.size()));
        ret.fValid = true;
    } catch (...) {
        ret.fValid = false;
    }

    ret.cachedHash.SetNull();
    return ret;
}

void CBLSSignature::SubInsecure(const CBLSSignature& o)
{
    assert(IsValid() && o.IsValid());
    impl = impl + o.impl.Negate();
    cachedHash.SetNull();
}

bool CBLSSignature::VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash, const bool specificLegacyScheme) const
{
    if (!IsValid() || !pubKey.IsValid()) {
        return false;
    }

    try {
        return Scheme(specificLegacyScheme)->Verify(pubKey.impl, bls::Bytes(hash.begin(), hash.size()), impl);
    } catch (...) {
        return false;
    }
}

bool CBLSSignature::VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash) const
{
    return VerifyInsecure(pubKey, hash, bls::bls_legacy_scheme.load());
}

bool CBLSSignature::VerifyInsecureAggregated(Span<CBLSPublicKey> pubKeys, Span<uint256> hashes) const
{
    if (!IsValid()) {
        return false;
    }
    assert(!pubKeys.empty() && !hashes.empty() && pubKeys.size() == hashes.size());

    std::vector<bls::G1Element> pubKeyVec;
    std::vector<bls::Bytes> hashes2;
    hashes2.reserve(hashes.size());
    pubKeyVec.reserve(pubKeys.size());
    for (size_t i = 0; i < pubKeys.size(); i++) {
        const auto& p = pubKeys[i];
        if (!p.IsValid()) {
            return false;
        }
        pubKeyVec.push_back(p.impl);
        hashes2.emplace_back(hashes[i].begin(), hashes[i].size());
    }

    try {
        return Scheme(bls::bls_legacy_scheme.load())->AggregateVerify(pubKeyVec, hashes2, impl);
    } catch (...) {
        return false;
    }
}

bool CBLSSignature::VerifySecureAggregated(Span<CBLSPublicKey> pks, const uint256& hash) const
{
    if (pks.empty()) {
        return false;
    }

    std::vector<bls::G1Element> vecPublicKeys;
    vecPublicKeys.reserve(pks.size());
    for (const auto& pk : pks) {
        vecPublicKeys.push_back(pk.impl);
    }

    try {
        return Scheme(bls::bls_legacy_scheme.load())->VerifySecure(vecPublicKeys, impl, bls::Bytes(hash.begin(), hash.size()));
    } catch (...) {
        return false;
    }
}

bool CBLSSignature::Recover(Span<CBLSSignature> sigs, Span<CBLSId> ids)
{
    fValid = false;
    cachedHash.SetNull();

    if (sigs.empty() || ids.empty() || sigs.size() != ids.size()) {
        return false;
    }

    std::vector<bls::G2Element> sigsVec;
    std::vector<bls::Bytes> idsVec;
    sigsVec.reserve(sigs.size());
    idsVec.reserve(sigs.size());

    for (size_t i = 0; i < sigs.size(); i++) {
        if (!sigs[i].IsValid() || !ids[i].IsValid()) {
            return false;
        }
        sigsVec.emplace_back(sigs[i].impl);
        idsVec.emplace_back(ids[i].impl.begin(), ids[i].impl.size());
    }

    try {
        impl = bls::Threshold::SignatureRecover(sigsVec, idsVec);
    } catch (...) {
        return false;
    }

    fValid = true;
    cachedHash.SetNull();
    return true;
}

#ifndef BUILD_BITCOIN_INTERNAL

static std::once_flag init_flag;
static mt_pooled_secure_allocator<uint8_t>* secure_allocator_instance;
static void create_secure_allocator()
{
    // make sure LockedPoolManager is initialized first (ensures destruction order)
    LockedPoolManager::Instance();

    // static variable in function scope ensures it's initialized when first accessed
    // and destroyed before LockedPoolManager
    static mt_pooled_secure_allocator<uint8_t> a(sizeof(bn_t) + sizeof(size_t));
    secure_allocator_instance = &a;
}

static mt_pooled_secure_allocator<uint8_t>& get_secure_allocator()
{
    std::call_once(init_flag, create_secure_allocator);
    return *secure_allocator_instance;
}

static void* secure_allocate(size_t n)
{
    uint8_t* ptr = get_secure_allocator().allocate(n + sizeof(size_t));
    *reinterpret_cast<size_t*>(ptr) = n;
    return ptr + sizeof(size_t);
}

static void secure_free(void* p)
{
    if (p == nullptr) {
        return;
    }

    uint8_t* ptr = reinterpret_cast<uint8_t*>(p) - sizeof(size_t);
    size_t n = *reinterpret_cast<size_t*>(ptr);
    return get_secure_allocator().deallocate(ptr, n);
}
#endif

bool BLSInit()
{
#ifndef BUILD_BITCOIN_INTERNAL
    bls::BLS::SetSecureAllocator(secure_allocate, secure_free);
#endif
    return true;
}
