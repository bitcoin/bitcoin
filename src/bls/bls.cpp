// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>

#include <random.h>
#include <tinyformat.h>

#ifndef BUILD_BITCOIN_INTERNAL
#include <support/allocators/mt_pooled_secure.h>
#endif

#include <assert.h>
#include <string.h>

static std::unique_ptr<bls::CoreMPL> pSchemeLegacy(new bls::LegacySchemeMPL);
static std::unique_ptr<bls::CoreMPL> pScheme(new bls::BasicSchemeMPL);

static std::unique_ptr<bls::CoreMPL>& Scheme(const bool fLegacy)
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

CBLSSecretKey CBLSSecretKey::AggregateInsecure(const std::vector<CBLSSecretKey>& sks)
{
    if (sks.empty()) {
        return CBLSSecretKey();
    }

    std::vector<bls::PrivateKey> v;
    v.reserve(sks.size());
    for (auto& sk : sks) {
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
    unsigned char buf[32];
    while (true) {
        GetStrongRandBytes(buf, sizeof(buf));
        try {
            impl = bls::PrivateKey::FromBytes(bls::Bytes((const uint8_t*)buf, SerSize));
            break;
        } catch (...) {
        }
    }
    fValid = true;
    cachedHash.SetNull();
}
#endif

bool CBLSSecretKey::SecretKeyShare(const std::vector<CBLSSecretKey>& msk, const CBLSId& _id)
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
        return CBLSPublicKey();
    }

    CBLSPublicKey pubKey;
    pubKey.impl = impl.GetG1Element();
    pubKey.fValid = true;
    pubKey.cachedHash.SetNull();
    return pubKey;
}

CBLSSignature CBLSSecretKey::Sign(const uint256& hash) const
{
    if (!IsValid()) {
        return CBLSSignature();
    }

    CBLSSignature sigRet;
    sigRet.impl = Scheme(fLegacy)->Sign(impl, bls::Bytes(hash.begin(), hash.size()));

    sigRet.fValid = true;
    sigRet.cachedHash.SetNull();

    return sigRet;
}

void CBLSPublicKey::AggregateInsecure(const CBLSPublicKey& o)
{
    assert(IsValid() && o.IsValid());
    impl = Scheme(fLegacy)->Aggregate({impl, o.impl});
    cachedHash.SetNull();
}

CBLSPublicKey CBLSPublicKey::AggregateInsecure(const std::vector<CBLSPublicKey>& pks, const bool fLegacy)
{
    if (pks.empty()) {
        return CBLSPublicKey();
    }

    std::vector<bls::G1Element> vecPublicKeys;
    vecPublicKeys.reserve(pks.size());
    for (auto& pk : pks) {
        vecPublicKeys.emplace_back(pk.impl);
    }

    CBLSPublicKey ret;
    ret.impl = Scheme(fLegacy)->Aggregate(vecPublicKeys);
    ret.fValid = true;
    ret.cachedHash.SetNull();
    return ret;
}

bool CBLSPublicKey::PublicKeyShare(const std::vector<CBLSPublicKey>& mpk, const CBLSId& _id)
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
    impl = Scheme(fLegacy)->Aggregate({impl, o.impl});
    cachedHash.SetNull();
}

CBLSSignature CBLSSignature::AggregateInsecure(const std::vector<CBLSSignature>& sigs, const bool fLegacy)
{
    if (sigs.empty()) {
        return CBLSSignature();
    }

    std::vector<bls::G2Element> v;
    v.reserve(sigs.size());
    for (auto& pk : sigs) {
        v.emplace_back(pk.impl);
    }

    CBLSSignature ret;
    ret.impl = Scheme(fLegacy)->Aggregate(v);
    ret.fValid = true;
    ret.cachedHash.SetNull();
    return ret;
}

CBLSSignature CBLSSignature::AggregateSecure(const std::vector<CBLSSignature>& sigs,
                                             const std::vector<CBLSPublicKey>& pks,
                                             const uint256& hash,
                                             const bool fLegacy)
{
    if (sigs.size() != pks.size() || sigs.empty()) {
        return CBLSSignature();
    }

    std::vector<bls::G1Element> vecPublicKeys;
    vecPublicKeys.reserve(pks.size());
    for (auto& pk : pks) {
        vecPublicKeys.push_back(pk.impl);
    }

    std::vector<bls::G2Element> vecSignatures;
    vecSignatures.reserve(pks.size());
    for (auto& sig : sigs) {
        vecSignatures.push_back(sig.impl);
    }

    CBLSSignature ret;
    ret.impl = Scheme(fLegacy)->AggregateSecure(vecPublicKeys, vecSignatures, bls::Bytes(hash.begin(), hash.size()));
    ret.fValid = true;
    ret.cachedHash.SetNull();
    return ret;
}

void CBLSSignature::SubInsecure(const CBLSSignature& o)
{
    assert(IsValid() && o.IsValid());
    impl = impl + o.impl.Negate();
    cachedHash.SetNull();
}

bool CBLSSignature::VerifyInsecure(const CBLSPublicKey& pubKey, const uint256& hash) const
{
    if (!IsValid() || !pubKey.IsValid()) {
        return false;
    }

    try {
        return Scheme(fLegacy)->Verify(pubKey.impl, bls::Bytes(hash.begin(), hash.size()), impl);
    } catch (...) {
        return false;
    }
}

bool CBLSSignature::VerifyInsecureAggregated(const std::vector<CBLSPublicKey>& pubKeys, const std::vector<uint256>& hashes) const
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
        auto& p = pubKeys[i];
        if (!p.IsValid()) {
            return false;
        }
        pubKeyVec.push_back(p.impl);
        hashes2.emplace_back(hashes[i].begin(), hashes[i].size());
    }

    try {
        return Scheme(fLegacy)->AggregateVerify(pubKeyVec, hashes2, impl);
    } catch (...) {
        return false;
    }
}

bool CBLSSignature::VerifySecureAggregated(const std::vector<CBLSPublicKey>& pks, const uint256& hash) const
{
    if (pks.empty()) {
        return false;
    }

    std::vector<bls::G1Element> vecPublicKeys;
    vecPublicKeys.reserve(pks.size());
    for (const auto& pk : pks) {
        vecPublicKeys.push_back(pk.impl);
    }

    return Scheme(fLegacy)->VerifySecure(vecPublicKeys, impl, bls::Bytes(hash.begin(), hash.size()));
}

bool CBLSSignature::Recover(const std::vector<CBLSSignature>& sigs, const std::vector<CBLSId>& ids)
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
    *(size_t*)ptr = n;
    return ptr + sizeof(size_t);
}

static void secure_free(void* p)
{
    if (!p) {
        return;
    }

    uint8_t* ptr = (uint8_t*)p - sizeof(size_t);
    size_t n = *(size_t*)ptr;
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
